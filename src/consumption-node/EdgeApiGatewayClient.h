/**
 * @file EdgeApiGatewayClient.h
 * @brief HTTP gateway client for the StorePulse Edge API.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `GatewayClient` subclass used by the network task of the `Device`
 * - Sketch-level class: the framework `HttpGatewayClient` only sends the `Authorization`
 *   header, and the Edge API authenticates the nodes with the `X-API-Key` header
 */

#ifndef EdgeApiGatewayClient_h
#define EdgeApiGatewayClient_h

#include <Arduino.h>
#include <ModestIoT.h>

/**
 * @brief Sends the telemetry of a node to the Edge API.
 *
 * - `POST <base URL>/iam/authentications` checks the credentials of the node (MS-02).
 * - `POST <base URL>/monitoring/telemetry` delivers a report. The Edge API answers `201`
 *   when it also reached the REST API and `202` when it keeps the report to synchronize it
 *   later; both mean that the report was delivered.
 */
class EdgeApiGatewayClient : public GatewayClient {
public:
    /**
     * @brief Constructs the client.
     * @param transportDriver WiFi connection of the node.
     * @param edgeBaseUrl Base URL of the Edge API, for example `http://192.168.1.10:5000/api/v1`.
     * @param deviceIdentifier Identifier of the node in the Edge API.
     * @param apiKey API key issued by the Edge API for the node.
     * @param requestTimeoutInMilliseconds Maximum time to wait for each answer.
     */
    EdgeApiGatewayClient(ConnectivityDriver& transportDriver, const char* edgeBaseUrl,
                         const char* deviceIdentifier, const char* apiKey,
                         uint16_t requestTimeoutInMilliseconds);

    /**
     * @brief Checks the credentials of the node against the Edge API.
     * @return `true` when the Edge API accepts the node.
     */
    bool authenticate();

    /**
     * @brief Delivers a report to the Edge API.
     * @param abstractTelemetryPayload Report to deliver.
     * @return `true` when the Edge API answers `201` or `202`.
     * @runs_on{network}
     */
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) override;

    /**
     * @brief Returns whether the Edge API rejected the content of the last request (`400`).
     *
     * A report with an invalid content is not sent again, because the Edge API would reject
     * it every time. Other errors, such as `401` for credentials that are not registered yet,
     * keep the report to send it again.
     * @return `true` for a `400` answer.
     */
    [[nodiscard]] bool wasLastPayloadRejected() const;

    /**
     * @brief Returns the HTTP status of the last request.
     * @return HTTP status, or a negative value when the Edge API could not be reached.
     */
    [[nodiscard]] int getLastResponseStatusCode() const;

    [[nodiscard]] GatewayStatistics getStatistics() const override;

private:
    static constexpr auto AUTHENTICATION_PATH = "/iam/authentications";
    static constexpr auto TELEMETRY_PATH = "/monitoring/telemetry";
    static constexpr auto API_KEY_HEADER = "X-API-Key";
    static constexpr auto CONTENT_TYPE_HEADER = "Content-Type";
    static constexpr auto JSON_CONTENT_TYPE = "application/json";
    static constexpr int NOT_REACHED_STATUS_CODE = -1;

    const char* deviceIdentifier;
    const char* apiKey;
    uint16_t requestTimeoutInMilliseconds;
    int lastResponseStatusCode;
    uint32_t sendsSucceededCount;
    uint32_t sendsFailedCount;

    /**
     * @brief Sends a JSON body to a path of the Edge API.
     * @param path Path after the base URL.
     * @param payload JSON body.
     * @return HTTP status, or a negative value when the Edge API could not be reached.
     */
    int post(const char* path, const String& payload);
};

#endif /* EdgeApiGatewayClient_h */
