#include "AlarmBuzzer.h"

AlarmBuzzer::AlarmBuzzer(const int gpioPin, const int activeLevel)
    : gpioPin(gpioPin),
      activeLevel(activeLevel),
      idleLevel(activeLevel == HIGH ? LOW : HIGH),
      active(false),
      turnOffAtInMilliseconds(0)
{
    pinMode(gpioPin, OUTPUT);
    digitalWrite(gpioPin, idleLevel);
}

void AlarmBuzzer::activateFor(const unsigned long durationInMilliseconds) {
    active = true;
    turnOffAtInMilliseconds = millis() + durationInMilliseconds;
    digitalWrite(gpioPin, activeLevel);
}

void AlarmBuzzer::update() {
    if (active && static_cast<long>(millis() - turnOffAtInMilliseconds) >= 0) {
        active = false;
        digitalWrite(gpioPin, idleLevel);
    }
}

bool AlarmBuzzer::isActive() const {
    return active;
}