#include "SmokeSensor.h"

SmokeSensor::SmokeSensor(const int analogInputGpioPin, const unsigned long warmUpDurationInMilliseconds,
                         const float alertThresholdInPpm, const float clearThresholdInPpm,
                         const unsigned long minimumAlarmDurationInMilliseconds, const float ppmPerAdcCount,
                         EventHandler* parentHandler)
    : Sensor(analogInputGpioPin, parentHandler),
      warmUpDurationInMilliseconds(warmUpDurationInMilliseconds),
      alertThresholdInPpm(alertThresholdInPpm),
      clearThresholdInPpm(clearThresholdInPpm),
      minimumAlarmDurationInMilliseconds(minimumAlarmDurationInMilliseconds),
      ppmPerAdcCount(ppmPerAdcCount),
      startedAtInMilliseconds(millis()),
      lastSmokeDetectedAtInMilliseconds(0),
      smokeConcentrationInPpm(0.0f),
      warmingUp(true),
      alarmActive(false)
{
    pinMode(pin, INPUT);
    analogReadResolution(12);
}

bool SmokeSensor::isWarmingUp() const {
    return warmingUp;
}

unsigned long SmokeSensor::getRemainingWarmUpInMilliseconds() const {
    if (!warmingUp) {
        return 0;
    }
    const unsigned long elapsed = millis() - startedAtInMilliseconds;
    return elapsed >= warmUpDurationInMilliseconds ? 0 : warmUpDurationInMilliseconds - elapsed;
}

bool SmokeSensor::isAlarmActive() const {
    return alarmActive;
}

float SmokeSensor::getSmokeConcentrationInPpm() const {
    return smokeConcentrationInPpm;
}

void SmokeSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        return;
    }

    if (warmingUp) {
        if (millis() - startedAtInMilliseconds < warmUpDurationInMilliseconds) {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }
        warmingUp = false;
        smokeConcentrationInPpm = readAverageConcentrationInPpm();
        alarmActive = evaluateAlarm(smokeConcentrationInPpm);
        event = Event(SENSOR_READY_EVENT_IDENTIFIER, pin);
        return;
    }

    smokeConcentrationInPpm = readAverageConcentrationInPpm();
    const bool previousAlarm = alarmActive;
    alarmActive = evaluateAlarm(smokeConcentrationInPpm);

    if (alarmActive != previousAlarm) {
        event = Event(SMOKE_ALARM_CHANGED_EVENT_IDENTIFIER, pin);
    } else {
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
    }
}

uint32_t SmokeSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t SmokeSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

float SmokeSensor::readAverageConcentrationInPpm() const {
    long sum = 0;
    for (int i = 0; i < SAMPLES_PER_READING; i++) {
        sum += analogRead(pin);
        delay(SAMPLE_SPACING_IN_MILLISECONDS);
    }
    return static_cast<float>(sum) / SAMPLES_PER_READING * ppmPerAdcCount;
}

bool SmokeSensor::evaluateAlarm(const float concentrationInPpm) {
    const unsigned long now = millis();
    const float limit = alarmActive ? clearThresholdInPpm : alertThresholdInPpm;
    if (concentrationInPpm > limit) {
        lastSmokeDetectedAtInMilliseconds = now;
        return true;
    }
    return alarmActive && now - lastSmokeDetectedAtInMilliseconds < minimumAlarmDurationInMilliseconds;
}
