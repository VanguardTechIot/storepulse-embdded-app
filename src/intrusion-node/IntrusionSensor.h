#ifndef IntrusionSensor_h
#define IntrusionSensor_h

#include <Arduino.h>
#include <ModestIoT.h>

/**
 * @brief PIR sensor that confirms an intrusion after consecutive readings.
 *
 * Emits INTRUSION_CONFIRMED once when a movement is confirmed and INTRUSION_ENDED
 * when the PIR stays low for the configured time. Reads the pin on every sampling tick.
 */
class IntrusionSensor : public Sensor {
public:
    static constexpr int INTRUSION_CONFIRMED_EVENT_IDENTIFIER = 70;
    static constexpr int INTRUSION_ENDED_EVENT_IDENTIFIER = 71;

    IntrusionSensor(int motionInputGpioPin, unsigned long warmUpDurationInMilliseconds,
                    uint8_t confirmationReadings, unsigned long endedAfterInMilliseconds,
                    EventHandler* parentHandler = nullptr);

    [[nodiscard]] bool isWarmingUp() const;

    [[nodiscard]] bool isIntrusionActive() const;

protected:
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

private:
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;

    unsigned long warmUpDurationInMilliseconds;
    uint8_t confirmationReadings;
    unsigned long endedAfterInMilliseconds;
    unsigned long startedAtInMilliseconds;
    unsigned long lastMotionAtInMilliseconds;
    uint8_t consecutiveMotionReadings;
    bool warmingUp;
    bool intrusionActive;
};

#endif