#include "CommonAreaTelemetryPackage.h"

namespace {
    constexpr size_t ISO_TIME_BUFFER_SIZE = 21;

    bool formatIsoUtcTime(const time_t time, char *destination) {
        if (time <= 0) {
            return false;
        }
        tm utc{};
        gmtime_r(&time, &utc);
        return strftime(destination, ISO_TIME_BUFFER_SIZE, "%Y-%m-%dT%H:%M:%SZ", &utc) > 0;
    }
}

CommonAreaTelemetryPackage::CommonAreaTelemetryPackage(const char *deviceIdentifier, const time_t recordedAt,
                                                       const bool motionDetected)
    : deviceIdentifier(deviceIdentifier),
      recordedAt(recordedAt),
      motionDetected(motionDetected),
      bufferedEvents(0) {
}

void CommonAreaTelemetryPackage::setBufferedEvents(const int count) {
    bufferedEvents = count > 0 ? count : 0;
}

void CommonAreaTelemetryPackage::serialize(JsonDocument &serializationDestination) const {
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
    measurement["value"] = motionDetected ? MOTION_DETECTED_VALUE : MOTION_STOPPED_VALUE;
    if (hasTime) {
        measurement["recordedAt"] = String(isoTime);
    }
}
