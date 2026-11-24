#ifndef SmokeSensor_h
#define SmokeSensor_h

#include <Arduino.h>
#include <ModestIoT.h>

class SmokeSensor : public Sensor {
public:
    static constexpr int SENSOR_READY_EVENT_IDENTIFIER = 60;
    static constexpr int SMOKE_ALARM_CHANGED_EVENT_IDENTIFIER = 61;

    SmokeSensor(int analogInputGpioPin, unsigned long warmUpDurationInMilliseconds,
                float alertThresholdInPpm, float clearThresholdInPpm,
                unsigned long minimumAlarmDurationInMilliseconds, float ppmPerAdcCount,
                EventHandler* parentHandler = nullptr);

    [[nodiscard]] bool isWarmingUp() const;

    [[nodiscard]] unsigned long getRemainingWarmUpInMilliseconds() const;

    [[nodiscard]] bool isAlarmActive() const;

    [[nodiscard]] float getSmokeConcentrationInPpm() const;

protected:
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

private:
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 200;
    static constexpr int SAMPLES_PER_READING = 20;
    static constexpr int SAMPLE_SPACING_IN_MILLISECONDS = 5;

    unsigned long warmUpDurationInMilliseconds;
    float alertThresholdInPpm;
    float clearThresholdInPpm;
    unsigned long minimumAlarmDurationInMilliseconds;
    float ppmPerAdcCount;

    unsigned long startedAtInMilliseconds;
    unsigned long lastSmokeDetectedAtInMilliseconds;
    float smokeConcentrationInPpm;
    bool warmingUp;
    bool alarmActive;

    [[nodiscard]] float readAverageConcentrationInPpm() const;

    bool evaluateAlarm(float concentrationInPpm);
};

#endif
