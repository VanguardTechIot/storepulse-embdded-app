#pragma once

#include <Arduino.h>

const bool SEND_TO_EDGE = true;
const uint32_t REPORT_INTERVAL_MS = 30000;

const int PIN_MQ2 = 34;
const int PIN_ALERT_LED = 2;
const int PIN_BUZZER = -1;
const int PIN_BATTERY = -1;
const float BATTERY_DIVIDER_RATIO = 2.0;

const uint32_t MQ2_WARMUP_SECONDS = 60;
const float MQ2_PPM_PER_COUNT = 1000.0 / 4095.0;

const float SMOKE_ALERT_PPM = 300.0;
const float SMOKE_CLEAR_PPM = 270.0;
const uint32_t MIN_ALARM_DURATION_MS = 30000;
