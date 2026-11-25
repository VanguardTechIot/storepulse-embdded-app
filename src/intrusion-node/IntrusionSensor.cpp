#include "IntrusionSensor.h"
#include "Config.h"

IntrusionSensor::IntrusionSensor(const int motionInputGpioPin,
                                 const unsigned long warmUpDurationInMilliseconds,
                                 const uint8_t confirmationReadings,
                                 const unsigned long endedAfterInMilliseconds,
                                 EventHandler* parentHandler)
    : Sensor(motionInputGpioPin, parentHandler),
      warmUpDurationInMilliseconds(warmUpDurationInMilliseconds),
      confirmationReadings(confirmationReadings),
      endedAfterInMilliseconds(endedAfterInMilliseconds),
      startedAtInMilliseconds(millis()),
      lastMotionAtInMilliseconds(0),
      consecutiveMotionReadings(0),
      warmingUp(true),
      intrusionActive(false)
{
    pinMode(pin, INPUT);
}

bool IntrusionSensor::isWarmingUp() const {
    return warmingUp;
}

bool IntrusionSensor::isIntrusionActive() const {
    return intrusionActive;
}

void IntrusionSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        return;
    }

    const unsigned long now = millis();
    if (warmingUp) {
        if (now - startedAtInMilliseconds < warmUpDurationInMilliseconds) {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }
        warmingUp = false;
    }

    if (digitalRead(pin) == HIGH) {
        lastMotionAtInMilliseconds = now;
        if (consecutiveMotionReadings < 255) {
            consecutiveMotionReadings++;
        }
        if (!intrusionActive && consecutiveMotionReadings >= confirmationReadings) {
            intrusionActive = true;
            event = Event(INTRUSION_CONFIRMED_EVENT_IDENTIFIER, pin);
            return;
        }
    } else {
        consecutiveMotionReadings = 0;
        if (intrusionActive && now - lastMotionAtInMilliseconds >= endedAfterInMilliseconds) {
            intrusionActive = false;
            event = Event(INTRUSION_ENDED_EVENT_IDENTIFIER, pin);
            return;
        }
    }

    event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
}

uint32_t IntrusionSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return MOTION_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t IntrusionSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MOTION_SAMPLING_INTERVAL_IN_MILLISECONDS;
}