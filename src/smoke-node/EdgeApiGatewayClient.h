#ifndef EdgeApiGatewayClient_h
#define EdgeApiGatewayClient_h

#include <Arduino.h>
#include <ModestIoT.h>

class EdgeApiGatewayClient : public GatewayClient {
public:
    EdgeApiGatewayClient(ConnectivityDriver& transportDriver, const char* edgeBaseUrl,
                         const char* deviceIdentifier, const char* apiKey,
                         uint16_t requestTimeoutInMilliseconds);

    bool authenticate();

    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) override;

    [[nodiscard]] bool wasLastPayloadRejected() const;

    [[nodiscard]] bool wasLastTelemetryEvaluatedAsAlert() const;

    [[nodiscard]] int getLastResponseStatusCode() const;

    [[nodiscard]] GatewayStatistics getStatistics() const override;

private:
    static constexpr auto AUTHENTICATION_PATH = "/iam/authentications";
    static constexpr auto TELEMETRY_PATH = "/monitoring/telemetry";
    static constexpr auto API_KEY_HEADER = "X-API-Key";
    static constexpr auto CONTENT_TYPE_HEADER = "Content-Type";
    static constexpr auto JSON_CONTENT_TYPE = "application/json";
    static constexpr auto ALERT_DEVICE_STATUS = "ALERT";
    static constexpr int NOT_REACHED_STATUS_CODE = -1;

    const char* deviceIdentifier;
    const char* apiKey;
    uint16_t requestTimeoutInMilliseconds;
    int lastResponseStatusCode;
    String lastResponseBody;
    bool lastTelemetryEvaluatedAsAlert;
    uint32_t sendsSucceededCount;
    uint32_t sendsFailedCount;

    int post(const char* path, const String& payload);

    [[nodiscard]] bool hasAlertEvaluation() const;
};

#endif
