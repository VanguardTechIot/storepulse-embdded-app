#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "Secrets.h"

namespace edge {

const int MAX_BUFFERED = 20;
String buffered[MAX_BUFFERED];
int bufferedCount = 0;
bool authenticated = false;
bool lastAlert = false;
bool sendToEdge = true;
int batteryPin = -1;
float batteryDividerRatio = 2.0;

void connectWifi() {
  if (!sendToEdge || WiFi.status() == WL_CONNECTED) return;
  Serial.printf("[WiFi] connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf(" connected. Node IP: %s\n", WiFi.localIP().toString().c_str());
    configTime(0, 0, "pool.ntp.org");
  } else {
    Serial.println(" not connected (check the network name, password and that it is a 2.4 GHz network)");
  }
}

bool utcTime(struct tm* t) {
  return getLocalTime(t, 50);
}

String nowIso() {
  struct tm t;
  if (!utcTime(&t)) return "";
  char text[25];
  strftime(text, sizeof text, "%Y-%m-%dT%H:%M:%SZ", &t);
  return String(text);
}

int post(const char* path, const String& body, String* response) {
  if (WiFi.status() != WL_CONNECTED) return -1;
  String url = String(EDGE_BASE_URL) + path;
  WiFiClient plain;
  WiFiClientSecure secure;
  secure.setInsecure();
  HTTPClient http;
  if (url.startsWith("https")) http.begin(secure, url); else http.begin(plain, url);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);
  int code = http.POST(body);
  if (response) *response = http.getString();
  http.end();
  return code;
}

bool authenticate() {
  String response;
  int code = post("/iam/authentications", String("{\"device_id\":\"") + DEVICE_ID + "\"}", &response);
  Serial.printf("[IAM] authentication -> %d %s\n", code, response.c_str());
  return code == 200;
}

float batteryVolts() {
  if (batteryPin < 0) return NAN;
  return analogRead(batteryPin) * 3.3 / 4095.0 * batteryDividerRatio;
}

void begin(bool enabled, int batteryInputPin, float dividerRatio) {
  sendToEdge = enabled;
  batteryPin = batteryInputPin;
  batteryDividerRatio = dividerRatio;
  analogReadResolution(12);
  if (!sendToEdge) {
    Serial.println("[Edge] sending disabled: telemetry is only printed here.");
    return;
  }
  connectWifi();
  authenticated = authenticate();
}

void beginTelemetry(JsonDocument& doc) {
  doc.clear();
  String now = nowIso();
  doc["deviceId"] = DEVICE_ID;
  if (now.length()) doc["receivedAt"] = now;
  float volts = batteryVolts();
  if (!isnan(volts)) doc["supplyVoltage"] = round(volts * 100) / 100.0;
  doc["bufferedEvents"] = bufferedCount;
  doc["measurements"].to<JsonArray>();
}

void addMeasurement(JsonDocument& doc, const char* type, float value) {
  JsonObject m = doc["measurements"].as<JsonArray>().add<JsonObject>();
  m["type"] = type;
  m["value"] = round(value * 1000) / 1000.0;
  if (doc["receivedAt"].is<const char*>()) m["recordedAt"] = doc["receivedAt"];
}

int deliver(const String& body, bool* alert) {
  String response;
  int code = post("/monitoring/telemetry", body, &response);
  Serial.printf("[Edge] telemetry -> %d %s\n", code, response.c_str());
  if (code == 201 || code == 202) {
    JsonDocument result;
    if (alert && !deserializeJson(result, response)) {
      for (JsonObject e : result["evaluations"].as<JsonArray>())
        if (e["deviceStatus"] == "ALERT") *alert = true;
    }
    return 0;
  }
  if (code == 401) authenticated = false;
  return code == 400 ? 2 : 1;
}

void bufferPacket(const String& body) {
  if (bufferedCount == MAX_BUFFERED) {
    for (int i = 1; i < MAX_BUFFERED; i++) buffered[i - 1] = buffered[i];
    bufferedCount--;
  }
  buffered[bufferedCount++] = body;
  Serial.printf("[Buffer] %d pending packet(s)\n", bufferedCount);
}

void send(JsonDocument& doc) {
  String body;
  serializeJson(doc, body);
  Serial.println("[Node] " + body);
  if (!sendToEdge) return;

  connectWifi();
  if (!authenticated) authenticated = authenticate();
  while (bufferedCount > 0) {
    if (deliver(buffered[0], nullptr) == 1) break;
    for (int i = 1; i < bufferedCount; i++) buffered[i - 1] = buffered[i];
    bufferedCount--;
  }
  bool alert = false;
  int outcome = bufferedCount > 0 ? 1 : deliver(body, &alert);
  if (outcome == 1) bufferPacket(body); else lastAlert = alert;
}

bool alert() {
  return lastAlert;
}

}
