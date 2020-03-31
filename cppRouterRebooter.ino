#include <Arduino.h>
#include "app_config.h"
#include "main.h"
//#include "ota.h"
#include "webserver.h"

WiFiClient wifiClient;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

AsyncClient* tcpClient = new AsyncClient;
AsyncMqttClient mqttClient;

DNSServer dnsServer;                  // Create class DNS server, captive portal re-direct
const byte DNS_PORT = 53;

// Access Point SSID, password & IP address. SSID will be softAP_ssid + chipID to make SSID unique
const char *softAP_ssid = "RouterRebooter";
const char *softAP_password = "routerrebooter";
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0);

Ticker buttonTimer;
Ticker ledTimer;
Ticker mqttReconnectTimer;
Ticker ntpTimer;
Ticker relayTimer;
Ticker serverTimer;
Ticker tcpTimer;
Ticker testTimer;
Ticker wifiReconnectTimer;

void setupWiFiAP() {
  delay(2000);
  WiFi.disconnect();
  WiFi.enableSTA(false);
  WiFi.enableAP(true);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  String softAP_ssid_ID = softAP_ssid + String("_") + String(ESP.getChipId());

  // Pick a random channel out of 1, 6 or 11
  int channel = (random(3) * 5) + 1;
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password, channel);

  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  Serial.printf("AP: IP Address: %s\n", tmpStr);
  setup_webServer();
}

void onTestWifiConnect(const WiFiEventStationModeGotIP& event) {
  serverTimer.detach();
  Serial.println("TestWIFI: Success.  Saving and restarting.");
  config_save_wifi(testssid, testpass);
  delay(500);
  espRestartRoutine();
}

void testWiFi() {
  if(testssid!="") {
    WiFi.enableAP(false);
    WiFi.enableSTA(true);
    wifiConnectHandler = WiFi.onStationModeGotIP(onTestWifiConnect);
    ledTimer.detach();
    ledTimer.attach(0.1,ledToggle);
    Serial.println("");
    Serial.print("WiFi: Testing connection to ");
    Serial.println(testssid);
    WiFi.mode(WIFI_STA);
    WiFi.hostname("routerrebooter");
    WiFi.begin(testssid, testpass);
    serverTimer.attach(30,espRestartRoutine);
  } else {
    Serial.println("ERROR: Testssid was empty?");
  }
}

void connectToWifi() {
  ledTimer.detach();
  ledTimer.attach(0.1,ledToggle);
  Serial.println("");
  Serial.print("WiFi: Connecting to ");
  Serial.println(esid);
  WiFi.enableAP(false);
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("routerrebooter");
  WiFi.begin(esid, epass);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("WiFi: Connected");
  wifiReconnectTimer.detach();
  ledSetToggleRate();
  connectToMqtt();
  timeClient.begin();
  ntpTimer.attach(60,[]() { timeClient.update(); });
  relayTimer.detach();
  routerReset = 0;
  relayTimer.attach(0.1,routerResetRoutine);
  testTimer.attach(test_interval,tcpTest);
  setup_webServer();
  tcpTest();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("WiFi: Disconnected");
  // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  mqttReconnectTimer.detach();
  ntpTimer.detach();
  // Set fast blink on LED
  ledTimer.detach();
  ledTimer.attach_ms(300,ledToggle);
  // disable TCP test
  testTimer.detach();
  if(!wifiReconnectTimer.active())
    wifiReconnectTimer.attach(30, connectToWifi);
  relayTimer.detach();
  routerReset = 1;
  relayTimer.attach(relay_min_on,routerResetRoutine);
}

bool setupMqtt() {
  if(mqtt_server != "") {
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    char topicbuf[32];
    mqtt_topic_base.toCharArray(topicbuf,32);
    char buf[32];
    snprintf(buf,32,"%s/availability", topicbuf);
    mqttClient.setWill(buf,0,1,"offline");
    IPAddress ip;
    if(ip.fromString(mqtt_server))
      mqttClient.setServer(ip, 1883);
    return 1;
  } else
    return 0;  
}

void connectToMqtt() {
  if(mqtt_server != "") {
    Serial.print("MQTT: Connecting to ");
    Serial.println(mqtt_server);
    mqttClient.connect();
  } else {
    Serial.println("MQTT: Server not defined.");
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("MQTT: Connected");
  Serial.print("MQTT: Session present: ");
  Serial.println(sessionPresent);
  mqttReconnectTimer.detach();
  char topicbuf[32];
  mqtt_topic_base.toCharArray(topicbuf,32);
  char buf[32];
  snprintf(buf,32,"%s/cmd", topicbuf);
  uint16_t packetIdSub = mqttClient.subscribe(buf, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
  snprintf(buf,32,"%s/availability", topicbuf);
  mqttClient.publish(buf,0,1,"online");
  mqttSendStatus();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.printf("MQTT: Disconnected -> %d\n", static_cast<typename std::underlying_type<AsyncMqttClientDisconnectReason>::type>(reason));
  if (WiFi.isConnected()){
    if(!mqttReconnectTimer.active()) 
      mqttReconnectTimer.attach(30, connectToMqtt);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.print("MQTT: topic: ");
  Serial.print(topic);
  Serial.print("  ");
  Serial.println(payload[0], HEX);

  if(payload[0] == 0x31) {
    relayTimer.detach();
    routerReset = 1;
    routerResetRoutine();
  }
}

void mqttSendStatus() {
  if(mqttClient.connected()) {
    char topicbuf[32];
    mqtt_topic_base.toCharArray(topicbuf,32);
    char buf[32];
    snprintf(buf,32,"%s/status", topicbuf);
    char payload[256];
    snprintf(payload,256,"{\"lastms\": %lu, \"countFail\": %lu, \"countSuccess\": %lu, \"routerReset\": %u, \"epoch\": %lu, \"ip\": \"%s\", \"rssi\": %l }", lastms, countFail, countSuccess, routerReset, timeClient.getEpochTime(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
    mqttClient.publish(buf,0,1,payload);
  }
}

void onTCPConnect(void* arg, AsyncClient* tcpClient) {
  lastms = millis() - prevTime;
  tcpTimer.detach();
  tcpClient->close();
	Serial.printf("CONNECT: OK ( %lu ms )\n", lastms);
  if(countFail > 0) {
    if(countSuccess >= test_success_clear) {
      countFail = 0;
      countSuccess = 0;
    } else {
      countFail --;
      countSuccess ++;
    }
    ledSetToggleRate();
  } else
    countSuccess = 0;
  mqttSendStatus();
//  update_me();
}

void onTCPError(void* arg, AsyncClient* tcpClient, err_t error) {
  tcpTimer.detach();
  // tcpTimer set to 1.5 seconds, so any error after that is timeout
  if( (millis() - prevTime) < 1500 ) {
    lastms = millis() - prevTime;
    tcpClient->close();
    Serial.printf("CONNECT: Error ( %lu ms )\n", lastms);
    countFail ++;
    countSuccess = 0;
    ledSetToggleRate();
    mqttSendStatus();
  }
}

void ledSetToggleRate() {
  ledTimer.detach();
  ledTimer.attach_ms(2000 / ( 1 + countFail),ledToggle);
}

void ledToggle() {
  digitalWrite(pin_led, !digitalRead(pin_led));
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  config_load_settings();
  routerReset = 0;
  pinMode(pin_relay, OUTPUT);
  pinMode(pin_led, OUTPUT);
  pinMode(pin_button, INPUT_PULLUP);

  // Turn the relay on
  digitalWrite(pin_relay, HIGH);

  // Turn the LED on
  digitalWrite(pin_led, LOW);

  buttonTimer.attach_ms(10,checkButton);

  if(esid != "") {
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
    connectToWifi();
    setupMqtt();
  } else {
    setupWiFiAP();
  }
}

void loop() {
  if(rebootESP) {
    delay(2000);
    espRestartRoutine();
  }
}

void checkButton() {
  bool buttonState = digitalRead(pin_button);
  unsigned int buttonMillis = millis();
  unsigned int debounce = 100;
  if(buttonState == 0) {
    while(( digitalRead(pin_button) == 0 ) && ( millis() - buttonMillis < debounce ))
      delay(5);
    if(digitalRead(pin_button) == 0) {
      routerReset = 1;
      buttonPresses++;
      Serial.printf("SYS: Button pressed %u times\n",buttonPresses);
      if(firstPress==0) {
        firstPress=millis();
      } else {
        if(millis() - firstPress < 5000 && buttonPresses > 4) {
          Serial.println("SYS: Performing factory reset");
          config_reset();
          delay(1000);
          espRestartRoutine();
        }
      }
    } else {
      if(firstPress > 0 && millis() - firstPress > 5000) {
        firstPress = 0;
        buttonPresses = 0;
      }
    }
  }
}

void tcpTest() {
  char serverbuf[32];
  test_server.toCharArray(serverbuf,32);
  if(tcpClient->connecting()) {
    Serial.printf("CONNECT: already in progress, skipping\n");
  } else {
    Serial.printf("CONNECT: Trying %s:%u\n", serverbuf, test_port);
    prevTime = millis();
    tcpClient->onConnect(&onTCPConnect, tcpClient);
    tcpClient->onError(&onTCPError, tcpClient);
	  tcpClient->connect(serverbuf, test_port);
    tcpTimer.once_ms(1500,tcpCancelTest);
  }
}

void tcpCancelTest() {
  tcpClient->abort();
  lastms = millis() - prevTime;
  Serial.printf("CONNECT: Abort ( %lu ms )\n", lastms);
  countFail ++;
  countSuccess = 0;
  ledSetToggleRate();
  mqttSendStatus();
}

void routerResetRoutine() {
  if (
    ((countFail >= test_fail_reset) && millis() > (relay_min_on * 1000))
      || routerReset == 1) {
    Serial.println("ROUTER: Switching off");
    mqttSendStatus();
    digitalWrite(pin_relay, LOW);
    relayTimer.detach();
    relayTimer.once(relay_off_time,espRestartRoutine);
  }
}

void espRestartRoutine() {
    Serial.println("ESP: Restarting...");
    ESP.restart();
}
