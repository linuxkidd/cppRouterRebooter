#include <Arduino.h>

// -------------------------------------------------------------------
// Load and save the RouterRebooter config.
//
// This implementation saves the config to the EEPROM area of flash
// -------------------------------------------------------------------

// Global config varables

// Wifi Network Strings
extern String esid;
extern String epass;

// MQTT Settings
extern String mqtt_server;
extern String mqtt_topic_base;

// Test settings
extern String test_server;
extern uint32 test_port;

// Test every X seconds
extern uint32 test_interval;

// Require X fails before cycling power
extern uint32 test_fail_reset;

// Require X successful tests before resetting the failing state
extern uint32 test_success_clear;

// Require the power to be on for X seconds before power cycle allowed
extern uint32 relay_min_on;
extern uint32 relay_off_time;

// GPIO pin assignments
extern uint32 pin_button;
extern uint32 pin_relay;
extern uint32 pin_led;

// -------------------------------------------------------------------
// Load saved settings
// -------------------------------------------------------------------
extern void config_load_settings();

// -------------------------------------------------------------------
// Save the All but WiFi details
// -------------------------------------------------------------------
extern void config_save_settings(String qmqtt_server, String qmqtt_topic,
  String qtest_server, uint32_t qtest_port, uint32_t qtest_interval,
  uint32_t qtest_fail_reset, uint32_t qtest_success_clear, uint32_t qrelay_min_on,
  uint32_t qrelay_off_time, uint32_t qpin_button, uint32_t qpin_relay, uint32_t qpin_led);

// -------------------------------------------------------------------
// Save the Wifi details
// -------------------------------------------------------------------
extern void config_save_wifi(String qsid, String qpass);

// -------------------------------------------------------------------
// Reset the config back to defaults
// -------------------------------------------------------------------
extern void config_reset();
