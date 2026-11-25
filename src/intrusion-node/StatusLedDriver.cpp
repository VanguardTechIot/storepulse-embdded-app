#include "StatusLedDriver.h"

namespace {
    constexpr LedColor OFF_COLOR{false, false, false};
    constexpr LedColor RED_COLOR{true, false, false};
    constexpr LedColor AMBER_COLOR{true, true, false};
    constexpr LedColor PURPLE_COLOR{true, false, true};
    constexpr LedColor BLUE_COLOR{false, false, true};
    constexpr LedColor GREEN_COLOR{false, true, false};
}

StatusLedDriver::StatusLedDriver(const int redPin, const int greenPin, const int bluePin, const bool commonAnode)
    : redPin(redPin),
      greenPin(greenPin),
      bluePin(bluePin),
      commonAnode(commonAnode),
      eventDetected(false),
      errorFault(false),
      networkLost(false),
      backupPower(false),
      blinkOn(false)
{
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    show(OFF_COLOR);
}

void StatusLedDriver::setEventDetected(const bool active) {
    eventDetected = active;
}

void StatusLedDriver::setErrorFault(const bool active) {
    errorFault = active;
}

void StatusLedDriver::setNetworkLost(const bool active) {
    networkLost = active;
}

void StatusLedDriver::setBackupPower(const bool active) {
    backupPower = active;
}

void StatusLedDriver::refresh() {
    blinkOn = !blinkOn;
    if (eventDetected) {
        show(blinkOn ? RED_COLOR : OFF_COLOR);
        return;
    }
    if (errorFault) {
        show(PURPLE_COLOR);
        return;
    }
    if (networkLost) {
        show(blinkOn ? AMBER_COLOR : OFF_COLOR);
        return;
    }
    if (backupPower) {
        show(BLUE_COLOR);
        return;
    }
    show(GREEN_COLOR);
}

void StatusLedDriver::show(const LedColor color) const {
    digitalWrite(redPin, commonAnode ? !color.red : color.red);
    digitalWrite(greenPin, commonAnode ? !color.green : color.green);
    digitalWrite(bluePin, commonAnode ? !color.blue : color.blue);
}