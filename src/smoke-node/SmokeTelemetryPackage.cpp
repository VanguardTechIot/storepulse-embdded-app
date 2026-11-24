#include "SmokeTelemetryPackage.h"
#include <cmath>

namespace {
    constexpr size_t ISO_TIME_BUFFER_SIZE = 21;
    constexpr double VALUE_DECIMAL_SCALE = 1000.0;

    bool formatIsoUtcTime(const time_t time, char* destination) {
        if (time <= 0) {
            return false;
        }
        tm utc{};
        gmtime_r(&time, &utc);
        return strftime(destination, ISO_TIME_BUFFER_SIZE, "%Y-%m-%dT%H:%M:%SZ", &utc) > 0;
    }
}

SmokeTelemetryPackage::SmokeTelemetryPackage(const char* deviceIdentifier, const time_t recordedAt,
                                             const float smokeConcentrationInPpm)
    : deviceIdentifier(deviceIdentifier),
      recordedAt(recordedAt),
      smokeConcentrationInPpm(smokeConcentrationInPpm > 0.0f ? smokeConcentrationInPpm : 0.0f),
      bufferedEvents(0)
{
}

void SmokeTelemetryPackage::setBufferedEvents(const int count) {
    bufferedEvents = count > 0 ? count : 0;
}

float SmokeTelemetryPackage::getSmokeConcentrationInPpm() const {
    return smokeConcentrationInPpm;
}

void SmokeTelemetryPackage::serialize(JsonDocument& serializationDestination) const {
    char isoTime[ISO_TIME_BUFFER_SIZE];
    const bool hasTime = formatIsoUtcTime(recordedAt, isoTime);

    serializationDestination["deviceId"] = deviceIdentifier;
    if (hasTime) {
        serializationDestination["receivedAt"] = String(isoTime);
    }
    serializationDestination["bufferedEvents"] = bufferedEvents;

    const JsonArray measurements = serializationDestination["measurements"].to<JsonArray>();
    const JsonObject measurement = measurements.add<JsonObject>();
    measurement["type"] = MEASUREMENT_TYPE;
    measurement["value"] = std::round(smokeConcentrationInPpm * VALUE_DECIMAL_SCALE) / VALUE_DECIMAL_SCALE;
    if (hasTime) {
        measurement["recordedAt"] = String(isoTime);
    }
}
