#include "EdgeApiGatewayClient.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

EdgeApiGatewayClient::EdgeApiGatewayClient(ConnectivityDriver& transportDriver, const char* edgeBaseUrl,
                                           const char* deviceIdentifier, const char* apiKey,
                                           const uint16_t requestTimeoutInMilliseconds)
    : GatewayClient(transportDriver, edgeBaseUrl),
      deviceIdentifier(deviceIdentifier),
      apiKey(apiKey),
      requestTimeoutInMilliseconds(requestTimeoutInMilliseconds),
      lastResponseStatusCode(NOT_REACHED_STATUS_CODE),
      lastTelemetryEvaluatedAsAlert(false),
      sendsSucceededCount(0),
      sendsFailedCount(0)
{
}

bool EdgeApiGatewayClient::authenticate() {
    JsonDocument body;
    body["device_id"] = deviceIdentifier;
    String payload;
    serializeJson(body, payload);

    const int statusCode = post(AUTHENTICATION_PATH, payload);
    Serial.printf("[EdgeApi] Authentication -> %d\n", statusCode);
    return statusCode == 200;
}

bool EdgeApiGatewayClient::sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
    const String payload = abstractTelemetryPayload.toJsonString();
    const int statusCode = post(TELEMETRY_PATH, payload);
    Serial.printf("[EdgeApi] Telemetry %s -> %d\n", payload.c_str(), statusCode);

    const bool delivered = statusCode == 201 || statusCode == 202;
    if (delivered) {
        sendsSucceededCount++;
        lastTelemetryEvaluatedAsAlert = hasAlertEvaluation();
    } else {
        sendsFailedCount++;
    }
    return delivered;
}

bool EdgeApiGatewayClient::wasLastPayloadRejected() const {
    return lastResponseStatusCode == 400;
}

bool EdgeApiGatewayClient::wasLastTelemetryEvaluatedAsAlert() const {
    return lastTelemetryEvaluatedAsAlert;
}

int EdgeApiGatewayClient::getLastResponseStatusCode() const {
    return lastResponseStatusCode;
}

GatewayStatistics EdgeApiGatewayClient::getStatistics() const {
    GatewayStatistics statistics;
    statistics.sendsSucceeded = sendsSucceededCount;
    statistics.sendsFailed = sendsFailedCount;
    return statistics;
}

bool EdgeApiGatewayClient::hasAlertEvaluation() const {
    JsonDocument response;
    if (deserializeJson(response, lastResponseBody)) {
        return false;
    }
    for (JsonObject evaluation : response["evaluations"].as<JsonArray>()) {
        if (evaluation["deviceStatus"] == ALERT_DEVICE_STATUS) {
            return true;
        }
    }
    return false;
}

int EdgeApiGatewayClient::post(const char* path, const String& payload) {
    lastResponseStatusCode = NOT_REACHED_STATUS_CODE;
    lastResponseBody = "";
    if (!communicationTransportDriver.isConnected()) {
        return lastResponseStatusCode;
    }

    const String url = String(serviceEndpointUrl) + path;
    WiFiClient plainSocket;
    WiFiClientSecure secureSocket;
    HTTPClient httpClient;

    bool started;
    if (url.startsWith("https")) {
        secureSocket.setInsecure();
        started = httpClient.begin(secureSocket, url);
    } else {
        started = httpClient.begin(plainSocket, url);
    }
    if (!started) {
        return lastResponseStatusCode;
    }

    httpClient.setTimeout(requestTimeoutInMilliseconds);
    httpClient.addHeader(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    httpClient.addHeader(API_KEY_HEADER, apiKey);
    lastResponseStatusCode = httpClient.POST(payload);
    if (lastResponseStatusCode > 0) {
        lastResponseBody = httpClient.getString();
    }
    httpClient.end();
    return lastResponseStatusCode;
}
