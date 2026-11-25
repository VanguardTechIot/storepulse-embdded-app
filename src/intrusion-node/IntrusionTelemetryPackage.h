#ifndef IntrusionTelemetryPackage_h
#define IntrusionTelemetryPackage_h

#include <Arduino.h>
#include <ModestIoT.h>
#include <time.h>

/** @brief Telemetry of one confirmed intrusion: a MOTION measurement with value 1. */
class IntrusionTelemetryPackage : public TelemetryPackage {
public:
    static constexpr auto MEASUREMENT_TYPE = "MOTION";
    static constexpr int MOTION_DETECTED_VALUE = 1;

    IntrusionTelemetryPackage(const char* deviceIdentifier, time_t recordedAt);

    void setBufferedEvents(int count);

    void serialize(JsonDocument& serializationDestination) const override;

private:
    const char* deviceIdentifier;
    time_t recordedAt;
    int bufferedEvents;
};

#endif