#pragma once

#include <Arduino.h>

/** @name Pin Definitions */
const int MOTION_SENSOR_PIN = 13;
const int STATUS_LED_PIN = 4;

/** @name Storage Configuration */
const char* const SD_MOUNT_PATH = "/sdcard";
const bool SD_ONE_LINE_MODE = true;
const bool SD_FORMAT_IF_MOUNT_FAILS = false;
const char* const CAPTURE_FILENAME_FORMAT = "/capture_%04u.jpg";
const size_t CAPTURE_FILENAME_BUFFER_SIZE = 32;
const unsigned int MAX_CAPTURE_INDEX = 9999;

/** @name Device Configuration */
const bool STATUS_LED_INITIAL_STATE = false;
const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 200;
const uint8_t HARDWARE_TIMER_INDEX = 0;
const uint32_t SAMPLER_STACK_BYTES = 16384;
const uint8_t ASYNC_ENGINE_QUEUE_DEPTH = 8;
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 1000;

/** @name Edge API Configuration */
const unsigned long WIFI_CONNECTION_TIMEOUT_IN_MILLISECONDS = 15000;
const uint16_t EDGE_REQUEST_TIMEOUT_IN_MILLISECONDS = 5000;
const unsigned long REPORT_RETRY_INTERVAL_IN_MILLISECONDS = 30000;
const size_t MAXIMUM_UNDELIVERED_REPORTS = 20;
const uint32_t NETWORK_STACK_BYTES = 12288;
const char* const TIME_ZONE = "UTC0";
const char* const NTP_SERVER = "pool.ntp.org";
const time_t MINIMUM_VALID_UTC_TIME = 1700000000;
const char* const FIRMWARE_VERSION = "SP-CMN FW 0.2.0";
const unsigned long HEARTBEAT_INTERVAL_IN_MILLISECONDS = 600000;
const unsigned long HEARTBEAT_CHECK_INTERVAL_IN_MILLISECONDS = 60000;
