#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AsyncMqttClient.h>

void setupWiFiAP();
void onTestWiFiConnect(const WiFiEventStationModeGotIP& event);
void testWiFi(String testssid, String testpass);
void connectToWifi();
void onWiFiConnect(const WiFiEventStationModeGotIP& event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);

bool setupMqtt();
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void mqttSendStatus();

void onTCPConnect(void* arg, AsyncClient* tcpClient);
void onTCPError(void* arg, AsyncClient* tcpClient, err_t error);

void ledSetToggleRate();
void ledToggle();

void setup();
void loop();
void checkButton();
void tcpTest();
void tcpCancelTest();
void routerResetRoutine();
void espRestartRoutine();

unsigned long countFail = 0;
unsigned long countSuccess = 0;
unsigned long healthLed = 0;
unsigned long lastms = 0;
unsigned long prevTime = 0;
unsigned int rebootESP = 0;
unsigned int routerReset = 0;

unsigned int buttonPresses = 0;
unsigned int firstPress = 0;
unsigned int wifiConnected = 0;

String testssid = "";
String testpass = "";
