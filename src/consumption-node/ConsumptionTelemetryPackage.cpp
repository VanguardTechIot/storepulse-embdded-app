/**
 * @file ConsumptionTelemetryPackage.cpp
 * @brief Implements the telemetry report of the consumption node.
 */

#include "ConsumptionTelemetryPackage.h"
#include <cstring>

namespace {
    /// Size of an ISO 8601 UTC time such as "2026-11-22T15:04:05Z", plus the terminator.
    constexpr size_t ISO_TIME_BUFFER_SIZE = 21;

    /**
     * @brief Formats a UTC time as ISO 8601.
     * @param time Seconds since the Unix epoch.
     * @param destination Buffer of ISO_TIME_BUFFER_SIZE characters.
     * @return `false` when the time is not set.
     */
    bool formatIsoUtcTime(const time_t time, char* destination) {
        if (time <= 0) {
            return false;
        }
        tm utc{};
        gmtime_r(&time, &utc);
        return strftime(destination, ISO_TIME_BUFFER_SIZE, "%Y-%m-%dT%H:%M:%SZ", &utc) > 0;
    }
}

ConsumptionTelemetryPackage::ConsumptionTelemetryPackage(const char* deviceIdentifier, const time_t recordedAt)
    : deviceIdentifier(deviceIdentifier),
      recordedAt(recordedAt),
      bufferedEvents(0),
      measurementCount(0)
{
}

void ConsumptionTelemetryPackage::addConsumption(const char* type, const double value) {
    if (type == nullptr) {
        return;
    }
    const double consumption = value > 0.0 ? value : 0.0;

    for (uint8_t i = 0; i < measurementCount; i++) {
        if (strcmp(measurements[i].type, type) == 0) {
            measurements[i].value += consumption;
            return;
        }
    }
    if (measurementCount < MAXIMUM_MEASUREMENTS) {
        measurements[measurementCount].type = type;
        measurements[measurementCount].value = consumption;
        measurementCount++;
    }
}

void ConsumptionTelemetryPackage::addConsumptionOf(const ConsumptionTelemetryPackage& other) {
    for (uint8_t i = 0; i < other.measurementCount; i++) {
        addConsumption(other.measurements[i].type, other.measurements[i].value);
    }
}

void ConsumptionTelemetryPackage::setBufferedEvents(const int count) {
    bufferedEvents = count > 0 ? count : 0;
}

bool ConsumptionTelemetryPackage::isEmpty() const {
    return measurementCount == 0;
}

time_t ConsumptionTelemetryPackage::getRecordedAt() const {
    return recordedAt;
}

void ConsumptionTelemetryPackage::serialize(JsonDocument& serializationDestination) const {
    char isoTime[ISO_TIME_BUFFER_SIZE];
    const bool hasTime = formatIsoUtcTime(recordedAt, isoTime);

    serializationDestination["deviceId"] = deviceIdentifier;
    if (hasTime) {
        // String() makes ArduinoJson copy the text: isoTime is a local buffer
        serializationDestination["receivedAt"] = String(isoTime);
    }
    serializationDestination["bufferedEvents"] = bufferedEvents;

    const JsonArray measurementArray = serializationDestination["measurements"].to<JsonArray>();
    for (uint8_t i = 0; i < measurementCount; i++) {
        const JsonObject measurement = measurementArray.add<JsonObject>();
        measurement["type"] = measurements[i].type;
        measurement["value"] = measurements[i].value;
        if (hasTime) {
            measurement["recordedAt"] = String(isoTime);
        }
    }
}
