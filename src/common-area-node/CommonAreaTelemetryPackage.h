#ifndef CommonAreaTelemetryPackage_h
#define CommonAreaTelemetryPackage_h

#include <Arduino.h>
#include <ModestIoT.h>
#include <time.h>

/** @brief Telemetry of a motion change in a common area: a MOTION measurement with value 1 or 0. */

class CommonAreaTelemetryPackage : public TelemetryPackage {
public:
    static constexpr auto MEASUREMENT_TYPE = "MOTION";
    static constexpr int MOTION_DETECTED_VALUE = 1;
    static constexpr int MOTION_STOPPED_VALUE = 0;

    CommonAreaTelemetryPackage(const char *deviceIdentifier, time_t recordedAt, bool motionDetected);

    void setBufferedEvents(int count);

    void serialize(JsonDocument& serializationDestination) const override;

private:
    const char* deviceIdentifier;
    time_t recordedAt;
    bool motionDetected;
    int bufferedEvents;
};

#endif
