#pragma once

#include <Arduino.h>

/** @name Pin Definitions (section 5.6 of the report) */
const int WATER_FLOW_SENSOR_PIN = 35;
const int ELECTRICITY_METER_RX_PIN = 16;
const int ELECTRICITY_METER_TX_PIN = 17;
const int STATUS_LED_RED_PIN = 25;
const int STATUS_LED_GREEN_PIN = 33;
const int STATUS_LED_BLUE_PIN = 32;
const bool STATUS_LED_COMMON_ANODE = false;

/** @name Reporting Rules (README, consumption node behavior) */
const unsigned long WATER_CONTINUOUS_FLOW_REPORT_INTERVAL_IN_MILLISECONDS = 5UL * 60UL * 1000UL;
const double ELECTRICITY_REPORT_THRESHOLD_IN_KILOWATT_HOURS = 0.01;
const unsigned long HEARTBEAT_INTERVAL_IN_MILLISECONDS = 10UL * 60UL * 1000UL;
const unsigned long STATUS_CHECK_INTERVAL_IN_MILLISECONDS = 5000;
const unsigned long REPORT_RETRY_INTERVAL_IN_MILLISECONDS = 30000;

/** @name Network Configuration */
const unsigned long WIFI_CONNECTION_TIMEOUT_IN_MILLISECONDS = 15000;
const uint16_t EDGE_REQUEST_TIMEOUT_IN_MILLISECONDS = 5000;
const char* const TIME_ZONE = "UTC0";
const char* const NTP_SERVER = "pool.ntp.org";

/** @name Sensor Health */
const uint8_t SENSOR_FAULT_THRESHOLD = 3;

/** @name Device Configuration */
const char* const FIRMWARE_VERSION = "SP-CON FW 0.1.0";
const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
const uint8_t HARDWARE_TIMER_INDEX = 0;
const uint32_t SAMPLER_STACK_BYTES = 8192;
const uint32_t NETWORK_STACK_BYTES = 12288;
const uint8_t ASYNC_ENGINE_QUEUE_DEPTH = 8;
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 1000;
