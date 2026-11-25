#pragma once

#include <Arduino.h>

/** @name Pin Definitions (section 5.6, pin assignment of the intrusion node) */
const int MOTION_SENSOR_PIN = 27;
const int ALARM_PIN = 26;
const int ALARM_ACTIVE_LEVEL = HIGH;   // Physical build drives the alarm through an NPN transistor
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;
const int STATUS_LED_RED_PIN = 25;
const int STATUS_LED_GREEN_PIN = 33;
const int STATUS_LED_BLUE_PIN = 32;
const bool STATUS_LED_COMMON_ANODE = false;

/** @name Motion Detection (section 5.6, algorithms 2 and 4 of step 10) */
#ifdef WOKWI_SIMULATION
const unsigned long PIR_WARM_UP_IN_MILLISECONDS = 5000;   // The simulated PIR is stable at once
#else
const unsigned long PIR_WARM_UP_IN_MILLISECONDS = 60000;
#endif
const unsigned long MOTION_SAMPLING_INTERVAL_IN_MILLISECONDS = 100;   // 10 Hz
const uint8_t MOTION_CONFIRMATION_READINGS = 3;
const unsigned long MOTION_ENDED_AFTER_IN_MILLISECONDS = 5000;
const unsigned long ALARM_DURATION_IN_MILLISECONDS = 60000;

/** @name Attention Hours (same window as BusinessHours in the Edge API) */
const long ATTENTION_UTC_OFFSET_IN_SECONDS = -5L * 3600L;   // Lima, no daylight saving time
const int ATTENTION_OPENING_HOUR = 10;
const int ATTENTION_CLOSING_HOUR = 22;

/** @name Real-Time Clock and Indicators */
const unsigned long RTC_SYNC_INTERVAL_IN_MILLISECONDS = 3600000UL;
const unsigned long STATUS_CHECK_INTERVAL_IN_MILLISECONDS = 1000;
const unsigned long INDICATOR_REFRESH_INTERVAL_IN_MILLISECONDS = 500;

/** @name Device Configuration */
const char* const FIRMWARE_VERSION = "SP-INT FW 0.1.0";
const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = MOTION_SAMPLING_INTERVAL_IN_MILLISECONDS;
const uint8_t HARDWARE_TIMER_INDEX = 0;
const uint32_t SAMPLER_STACK_BYTES = 8192;
const uint32_t NETWORK_STACK_BYTES = 12288;
const uint8_t ASYNC_ENGINE_QUEUE_DEPTH = 8;
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 1000;
const size_t MAXIMUM_UNDELIVERED_REPORTS = 20;
const unsigned long REPORT_RETRY_INTERVAL_IN_MILLISECONDS = 30000;

/** @name Network */
const unsigned long WIFI_CONNECTION_TIMEOUT_IN_MILLISECONDS = 15000;
const uint16_t EDGE_REQUEST_TIMEOUT_IN_MILLISECONDS = 5000;
const char* const TIME_ZONE = "UTC0";
const char* const NTP_SERVER = "pool.ntp.org";