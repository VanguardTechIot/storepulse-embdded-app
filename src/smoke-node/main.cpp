#include <Arduino.h>
#include "Config.h"
#include <EdgeGatewayClient.h>

void setupSensor() {
  Serial.printf("[MQ-2] warming up for %lu s, keep it in clean air...\n", (unsigned long)MQ2_WARMUP_SECONDS);
  for (uint32_t s = MQ2_WARMUP_SECONDS; s > 0; s--) {
    if (s % 10 == 0) Serial.printf("[MQ-2] %lu s left\n", (unsigned long)s);
    delay(1000);
  }
  Serial.println("[MQ-2] ready");
}

float readSmokePpm() {
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(PIN_MQ2);
    delay(5);
  }
  return (sum / 20.0) * MQ2_PPM_PER_COUNT;
}

void setupActions() {
  if (PIN_ALERT_LED >= 0) pinMode(PIN_ALERT_LED, OUTPUT);
  if (PIN_BUZZER >= 0) pinMode(PIN_BUZZER, OUTPUT);
}

void applyActions(bool alarm) {
  if (PIN_ALERT_LED >= 0) digitalWrite(PIN_ALERT_LED, alarm);
  if (PIN_BUZZER >= 0) {
    if (alarm) tone(PIN_BUZZER, 2000); else noTone(PIN_BUZZER);
  }
}

uint32_t lastReport = 0;
uint32_t lastSmokeDetected = 0;
bool lastLocalAlarm = false;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== StorePulse - Smoke node ===");
  setupActions();
  edge::begin(SEND_TO_EDGE, PIN_BATTERY, BATTERY_DIVIDER_RATIO);
  setupSensor();
  lastReport = millis() - REPORT_INTERVAL_MS;
}

void loop() {
  float smokePpm = readSmokePpm();

  float smokeLimit = lastLocalAlarm ? SMOKE_CLEAR_PPM : SMOKE_ALERT_PPM;
  bool smokeDetected = smokePpm > smokeLimit;
  if (smokeDetected) lastSmokeDetected = millis();
  bool localAlarm = smokeDetected ||
                    (lastLocalAlarm && millis() - lastSmokeDetected < MIN_ALARM_DURATION_MS);

  if (millis() - lastReport >= REPORT_INTERVAL_MS || localAlarm != lastLocalAlarm) {
    lastReport = millis();
    lastLocalAlarm = localAlarm;

    JsonDocument doc;
    edge::beginTelemetry(doc);
    edge::addMeasurement(doc, "SMOKE", smokePpm);
    edge::send(doc);
  }

  applyActions(localAlarm || edge::alert());
  delay(200);
}
