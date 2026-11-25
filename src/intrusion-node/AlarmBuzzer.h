#ifndef AlarmBuzzer_h
#define AlarmBuzzer_h

#include <Arduino.h>

/** @brief Active buzzer that sounds for a fixed time, without waiting for the network. */
class AlarmBuzzer {
public:
    AlarmBuzzer(int gpioPin, int activeLevel);

    void activateFor(unsigned long durationInMilliseconds);

    void update();

    [[nodiscard]] bool isActive() const;

private:
    int gpioPin;
    int activeLevel;
    int idleLevel;
    bool active;
    unsigned long turnOffAtInMilliseconds;
};

#endif