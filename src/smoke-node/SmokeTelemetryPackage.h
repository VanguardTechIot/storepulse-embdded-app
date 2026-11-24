#ifndef SmokeTelemetryPackage_h
#define SmokeTelemetryPackage_h

#include <Arduino.h>
#include <ModestIoT.h>
#include <time.h>

class SmokeTelemetryPackage : public TelemetryPackage {
public:
    static constexpr auto MEASUREMENT_TYPE = "SMOKE";

    SmokeTelemetryPackage(const char* deviceIdentifier, time_t recordedAt, float smokeConcentrationInPpm);

    void setBufferedEvents(int count);

    [[nodiscard]] float getSmokeConcentrationInPpm() const;

    void serialize(JsonDocument& serializationDestination) const override;

private:
    const char* deviceIdentifier;
    time_t recordedAt;
    float smokeConcentrationInPpm;
    int bufferedEvents;
};

#endif
