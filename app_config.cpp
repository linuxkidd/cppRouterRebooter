#include "app_config.h"

#include <Arduino.h>
#include <EEPROM.h>             // Save config settings

// Wifi Network Strings
String esid;
String epass;

// MQTT Settings
String mqtt_server;
String mqtt_topic_base;

// Test settings
String test_server;
uint32 test_port;

// Test every X seconds
uint32 test_interval;

// Require X fails before cycling power
uint32 test_fail_reset;

// Require X successful tests before resetting the failing state
uint32 test_success_clear;

// Require the power to be on for X seconds before power cycle allowed
uint32 relay_min_on;
uint32 relay_off_time;

// GPIO pin assignments
uint32 pin_button =  0;
uint32 pin_relay  = 12;
uint32 pin_led    = 13;


#define EEPROM_ESID_SIZE              32
#define EEPROM_EPASS_SIZE             64
#define EEPROM_MQTT_SERVER_SIZE       45
#define EEPROM_MQTT_TOPIC_SIZE        32
#define EEPROM_TEST_SERVER_SIZE       128

#define EEPROM_UINT_SIZE              4
#define EEPROM_SIZE                   1024

#define EEPROM_ESID_START             0
#define EEPROM_EPASS_START            (EEPROM_ESID_START + EEPROM_ESID_SIZE)
#define EEPROM_MQTT_SERVER_START      (EEPROM_EPASS_START + EEPROM_EPASS_SIZE)
#define EEPROM_MQTT_TOPIC_START       (EEPROM_MQTT_SERVER_START + EEPROM_MQTT_SERVER_SIZE)

#define EEPROM_TEST_SERVER_START      (EEPROM_MQTT_TOPIC_START + EEPROM_MQTT_TOPIC_SIZE)
#define EEPROM_TEST_PORT_START        (EEPROM_TEST_SERVER_START + EEPROM_TEST_SERVER_SIZE)

#define EEPROM_TEST_INTERVAL_START    (EEPROM_TEST_PORT_START + EEPROM_UINT_SIZE)
#define EEPROM_TEST_FAIL_START        (EEPROM_TEST_INTERVAL_START + EEPROM_UINT_SIZE)
#define EEPROM_TEST_SUCCESS_START     (EEPROM_TEST_FAIL_START + EEPROM_UINT_SIZE)
#define EEPROM_RELAY_MIN_START        (EEPROM_TEST_SUCCESS_START + EEPROM_UINT_SIZE)
#define EEPROM_RELAY_OFF_START        (EEPROM_RELAY_MIN_START + EEPROM_UINT_SIZE)

#define EEPROM_CONFIG_END             (EEPROM_RELAY_OFF_START + EEPROM_UINT_SIZE)

#if EEPROM_CONFIG_END > EEPROM_SIZE
#error EEPROM_SIZE too small
#endif

#define CHECKSUM_SEED 128

// -------------------------------------------------------------------
// Reset EEPROM, wipes all settings
// -------------------------------------------------------------------
void ResetEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  for (int i = 0; i < EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0xff);
  }
  EEPROM.end();
}

void EEPROM_read_string(uint start, uint count, String & val, String defaultVal = "") {
  val = "";
  byte checksum = CHECKSUM_SEED;
  for (uint i = 0; i < count - 1; ++i) {
    byte c = EEPROM.read(start + i);
    if (c != 0 && c != 255) {
      checksum ^= c;
      val += (char) c;
    } else {
      break;
    }
  }

  // Check the checksum
  byte c = EEPROM.read(start + (count - 1));
  if(c != checksum) {
    val = defaultVal;
  }
}

void EEPROM_write_string(uint start, uint count, String val) {
  byte checksum = CHECKSUM_SEED;
  for (uint i = 0; i < count - 1; ++i) {
    if (i < val.length()) {
      checksum ^= val[i];
      EEPROM.write(start + i, val[i]);
    } else {
      EEPROM.write(start + i, 0);
    }
  }
  EEPROM.write(start + (count - 1), checksum);
}

void EEPROM_read_uint32(int start, uint32_t & val, uint32_t defaultVal = 0) {
  byte checksum = CHECKSUM_SEED;
  val = 0;
  for (int i = 0; i < (EEPROM_UINT_SIZE - 1); ++i) {
    byte c = EEPROM.read(start + i);
    checksum ^= c;
    val = (val << 8) | c;
  }

  Serial.printf("Got %u from %d\n",val,start);
  // Check the checksum
  byte c = EEPROM.read(start + ( EEPROM_UINT_SIZE - 1 ));
  if(c != checksum) {
    Serial.printf("Checksum %02x did not match calculated %02x.  Using default %u\n",c,checksum,defaultVal);
    val = defaultVal;
  }
}

void EEPROM_write_uint32(int start, uint32_t value) {
  byte checksum = CHECKSUM_SEED;
  uint32_t val = value;
  for (int i = (EEPROM_UINT_SIZE - 2); i >= 0; --i) {
    byte c = val & 0xff;
    val = val >> 8;
    checksum ^= c;
    EEPROM.write(start + i, c);
  }
  EEPROM.write(start + (EEPROM_UINT_SIZE - 1), checksum);
  Serial.printf("Value %02x written with Checksum %02x.\n",value,checksum);
}

// -------------------------------------------------------------------
// Load saved settings from EEPROM
// -------------------------------------------------------------------
void config_load_settings() {
  Serial.println("CONFIG: Loading...");
  EEPROM.begin(EEPROM_SIZE);

  // Load WiFi values
  EEPROM_read_string(EEPROM_ESID_START, EEPROM_ESID_SIZE, esid);
  EEPROM_read_string(EEPROM_EPASS_START, EEPROM_EPASS_SIZE, epass);

  // MQTT settings
  EEPROM_read_string(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE,
                     mqtt_server);
  EEPROM_read_string(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE,
                     mqtt_topic_base, "RouterRebooter");

  // Test data
  EEPROM_read_string(EEPROM_TEST_SERVER_START, EEPROM_TEST_SERVER_SIZE,
                     test_server, "one.one.one.one");
  EEPROM_read_uint32(EEPROM_TEST_PORT_START,     test_port,         80);
  EEPROM_read_uint32(EEPROM_TEST_INTERVAL_START, test_interval,     60);
  EEPROM_read_uint32(EEPROM_TEST_FAIL_START,     test_fail_reset,    5);
  EEPROM_read_uint32(EEPROM_TEST_SUCCESS_START,  test_success_clear, 2);

  EEPROM_read_uint32(EEPROM_RELAY_MIN_START,     relay_min_on,     300);
  EEPROM_read_uint32(EEPROM_RELAY_OFF_START,     relay_off_time,    10);

  EEPROM.end();
  Serial.println("CONFIG: done");
}

void config_save_settings() {
  EEPROM.begin(EEPROM_SIZE);

  // MQTT settings
  EEPROM_write_string(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE,
                     mqtt_server);
  EEPROM_write_string(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE,
                     mqtt_topic_base);

  // Test data
  EEPROM_write_string(EEPROM_TEST_SERVER_START, EEPROM_TEST_SERVER_SIZE,
                     test_server);
  EEPROM_write_uint32(EEPROM_TEST_PORT_START,              test_port);
  EEPROM_write_uint32(EEPROM_TEST_INTERVAL_START,      test_interval);
  EEPROM_write_uint32(EEPROM_TEST_FAIL_START,        test_fail_reset);
  EEPROM_write_uint32(EEPROM_TEST_SUCCESS_START,  test_success_clear);

  EEPROM_write_uint32(EEPROM_RELAY_MIN_START,           relay_min_on);
  EEPROM_write_uint32(EEPROM_RELAY_OFF_START,         relay_off_time);

  EEPROM.end();
}

void config_save_wifi(String qsid, String qpass) {
  EEPROM.begin(EEPROM_SIZE);

  esid = qsid;
  epass = qpass;

  EEPROM_write_string(EEPROM_ESID_START, EEPROM_ESID_SIZE, qsid);
  EEPROM_write_string(EEPROM_EPASS_START, EEPROM_EPASS_SIZE, qpass);

  EEPROM.end();
}

void config_reset() {
  ResetEEPROM();
}
