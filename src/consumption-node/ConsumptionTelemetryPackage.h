/**
 * @file ConsumptionTelemetryPackage.h
 * @brief Telemetry report of the consumption node, in the format of the Edge API.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `TelemetryPackage` subclass with a custom data schema
 * - Created in the sampler task and sent by the network task through the telemetry queue
 */

#ifndef ConsumptionTelemetryPackage_h
#define ConsumptionTelemetryPackage_h

#include <Arduino.h>
#include <ModestIoT.h>
#include <time.h>

/**
 * @brief One measurement of a report: a consumption since the previous report.
 */
struct ConsumptionMeasurement {
    const char* type = nullptr; ///< `WATER` or `ELECTRICITY`; points to a string constant.
    double value = 0.0;         ///< Consumption in m³ (water) or kWh (electricity).
};

/**
 * @brief Report sent to `POST /monitoring/telemetry` of the Edge API.
 *
 * Serialized as:
 * @code{.json}
 * {"deviceId": "...", "receivedAt": "2026-11-22T15:04:05Z", "bufferedEvents": 0,
 *  "measurements": [{"type": "WATER", "value": 0.0125, "recordedAt": "2026-11-22T15:04:05Z"}]}
 * @endcode
 * The times are left out while the node has no NTP time; the Edge API then uses the time
 * it receives the report.
 */
class ConsumptionTelemetryPackage : public TelemetryPackage {
public:
    /// The node has two meters, so a report carries at most two measurements.
    static constexpr uint8_t MAXIMUM_MEASUREMENTS = 2;

    /**
     * @brief Constructs an empty report.
     * @param deviceIdentifier Identifier of the node in the Edge API.
     * @param recordedAt UTC time of the report, or 0 when the node has no NTP time.
     */
    ConsumptionTelemetryPackage(const char* deviceIdentifier, time_t recordedAt);

    /**
     * @brief Adds a consumption, or adds it to the measurement of the same type.
     * @param type Measurement type; must point to a string constant.
     * @param value Consumption since the previous report. Negative values count as 0.
     */
    void addConsumption(const char* type, double value);

    /**
     * @brief Adds all the measurements of another report to this one.
     * @param other Report whose consumption has not been delivered yet.
     */
    void addConsumptionOf(const ConsumptionTelemetryPackage& other);

    /**
     * @brief Sets the number of reports kept on the node while the Edge API is unreachable.
     * @param count Reports waiting to be delivered.
     */
    void setBufferedEvents(int count);

    /**
     * @brief Returns whether the report carries no measurement.
     * @return `true` when the report is empty.
     */
    [[nodiscard]] bool isEmpty() const;

    /**
     * @brief Returns the UTC time of the report.
     * @return Seconds since the Unix epoch, or 0 when the node has no NTP time.
     */
    [[nodiscard]] time_t getRecordedAt() const;

    void serialize(JsonDocument& serializationDestination) const override;

private:
    const char* deviceIdentifier;
    time_t recordedAt;
    int bufferedEvents;
    ConsumptionMeasurement measurements[MAXIMUM_MEASUREMENTS];
    uint8_t measurementCount;
};

#endif /* ConsumptionTelemetryPackage_h */
