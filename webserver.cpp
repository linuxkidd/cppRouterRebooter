#include "webserver.h"
#include "webserver_static.h"

extern unsigned long countFail;
extern unsigned long countSuccess;
extern unsigned long lastms;
extern unsigned long prevTime;
extern unsigned int rebootESP;
extern unsigned int routerReset;
extern NTPClient timeClient;
extern WiFiClient wifiClient;

// WiFi Client settings
extern String esid;
extern String epass;
extern String testssid;
extern String testpass;

// MQTT Settings
extern String mqtt_server;
extern String mqtt_topic_base;
extern String test_server;
extern uint32 test_port;
extern uint32 test_interval;
extern uint32 test_fail_reset;
extern uint32 test_success_clear;
extern uint32 relay_min_on;
extern uint32 relay_off_time;

extern void testWiFi();
extern void config_reset();
extern void config_save_settings();

AsyncWebServer webServer(80);

void setup_webServer() {
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response;
    if(esid=="") {
      response = request->beginResponse(200, "text/html", serverAPIndex);
    } else {
      response = request->beginResponse(200, "text/html", serverIndex);
    }
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  webServer.on("/powercycle.svg", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(200, "image/svg+xml", powercycleicon);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  webServer.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    char payload[512];
    char mqtt_server_buf[45];
    mqtt_server.toCharArray(mqtt_server_buf,45);
    char mqtt_topic_base_buf[32];
    mqtt_topic_base.toCharArray(mqtt_topic_base_buf,32);
    char test_server_buf[32];
    test_server.toCharArray(test_server_buf,32);
    char wifi_name[32];
    esid.toCharArray(wifi_name,32);
    snprintf(payload,512,
      "{\"lastms\": %lu, "
      "\"countFail\": %lu, "
      "\"countSuccess\": %lu, "
      "\"routerReset\": %u, "
      "\"epoch\": %lu, "
      "\"ip\": \"%s\", "
      "\"mqtt_server\": \"%s\", "
      "\"mqtt_topic_base\": \"%s\", "
      "\"test_server\": \"%s\", "
      "\"test_port\": %u, "
      "\"test_interval\": %u, "
      "\"test_fail_reset\": %u, "
      "\"test_success_clear\": %u, "
      "\"relay_min_on\": %u, "
      "\"relay_off_time\": %u, "
      "\"wifi_rssi\": \"%d dBm\", "
      "\"wifi_name\": \"%s\" "
      "}", lastms, countFail, countSuccess, routerReset, timeClient.getEpochTime(),
      WiFi.localIP().toString().c_str(), mqtt_server_buf, mqtt_topic_base_buf, 
      test_server_buf, test_port, test_interval, test_fail_reset, test_success_clear,
      relay_min_on, relay_off_time, WiFi.RSSI(), wifi_name);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", payload);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  webServer.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    config_reset();
    char payload[512];
    snprintf(payload,512,redirectIndex,5,"Factory reset complete.  Rebooting.",5);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", payload);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    rebootESP = 1;
  });

  webServer.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
    char payload[512];
    snprintf(payload,512,redirectIndex,5,"Restarting",5);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", payload);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    rebootESP = 1;
  });

  webServer.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    char payload[512];
    snprintf(payload,512,redirectIndex,relay_off_time,"Rebooting Router.",relay_off_time);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", payload);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    routerReset = 1;
  });

  webServer.on("/savewifi", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("testssid", true)) {
      AsyncWebParameter* p = request->getParam("testssid", true);
      testssid = p->value().c_str();
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
      if(request->hasParam("testpass", true)) {
        AsyncWebParameter* p = request->getParam("testpass", true);
        testpass = p->value().c_str();
        Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
      }

      char payload[512];
      snprintf(payload,512,redirectIndex,15,"Testing.  If successful, the new settings will be saved. Otherwise, the system will restart so you can try again.",15);
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", payload);
      response->addHeader("Connection", "close");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
      delay(2000);
      testWiFi();
    } else {
      char payload[512];
      snprintf(payload,512,redirectIndex,5,"SSID was blank.",5);
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", payload);
      response->addHeader("Connection", "close");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    }
  });


  webServer.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    String oldmqtt_server = mqtt_server;
    if(request->hasParam("mqtt_server", true)) {
      AsyncWebParameter* p = request->getParam("mqtt_server", true);
      mqtt_server = p->value().c_str();
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
    }
    if(request->hasParam("mqtt_topic_base", true)) {
      AsyncWebParameter* p = request->getParam("mqtt_topic_base", true);
      mqtt_topic_base = p->value().c_str();
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
    }
    if(request->hasParam("test_server", true)) {
      AsyncWebParameter* p = request->getParam("test_server", true);
      test_server = p->value().c_str();
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
    }
    if(request->hasParam("test_port", true)) {
      AsyncWebParameter* p = request->getParam("test_port", true);
      test_port = atoi(p->value().c_str());
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
    }
    if(request->hasParam("test_interval", true)) {
      AsyncWebParameter* p = request->getParam("test_interval", true);
      test_interval = atoi(p->value().c_str());
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
    }
    if(request->hasParam("test_fail_reset", true)) {
      AsyncWebParameter* p = request->getParam("test_fail_reset", true);
      test_fail_reset = atoi(p->value().c_str());
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
    }
    if(request->hasParam("test_success_clear", true)) {
      AsyncWebParameter* p = request->getParam("test_success_clear", true);
      test_success_clear = atoi(p->value().c_str());
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
    }
    if(request->hasParam("relay_min_on", true)) {
      AsyncWebParameter* p = request->getParam("relay_min_on", true);
      relay_min_on = atoi(p->value().c_str());
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
    }
    if(request->hasParam("relay_off_time", true)) {
      AsyncWebParameter* p = request->getParam("relay_off_time", true);
      relay_off_time = atoi(p->value().c_str());
      Serial.printf("POST: %s: %s\n", p->name().c_str(), p->value().c_str());
    }

    config_save_settings();
    char payload[512];
    snprintf(payload,512,redirectIndex,8,"Saved",8);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", payload);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    rebootESP = 1;
  });

  webServer.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    // the request handler is triggered after the upload has finished... 
    // create the response, add header, and send response
    char payload[512];
    snprintf(payload,512,redirectIndex,5,(Update.hasError())?"FAIL":"OK",5);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", payload);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    rebootESP = 1;  // Tell the main loop to restart the ESP
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    //Upload handler chunks in data

    if(!index){ // if index == 0 then this is the first frame of data
      Serial.printf("UploadStart: %s\n", filename.c_str());
      Serial.setDebugOutput(true);

      // calculate sketch space required for the update
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if(!Update.begin(maxSketchSpace)){//start with max available size
        Update.printError(Serial);
      }
      Update.runAsync(true); // tell the updaterClass to run in async mode
    }

    //Write chunked data to the free sketch space
    if(Update.write(data, len) != len){
        Update.printError(Serial);
    }

    if(final){ // if the final flag is set then this is the last frame of data
      if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u B\nRebooting...\n", index+len);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
    }
  });
  
  webServer.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());
  
    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    if(esid=="") {
      AsyncWebServerResponse *response = request->beginResponse(302, "text/html", "<head><meta http-equiv=\"refresh\" content=\"0; url=/\" /></head><body><p>redirecting...</p></body>");
      response->addHeader("Location", "/");
      request->send(response);
    } else
      request->send(404);
  });

  webServer.begin();
}
