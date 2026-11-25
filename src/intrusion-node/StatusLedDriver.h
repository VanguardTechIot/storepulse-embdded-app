#ifndef StatusLedDriver_h
#define StatusLedDriver_h

#include <Arduino.h>

struct LedColor {
    bool red;
    bool green;
    bool blue;
};

/**
 * @brief RGB status LED of the node (section 5.6, LED states).
 *
 * Priority: event detected (red, blinking) > error (purple) > no network (amber, blinking)
 * > backup power (blue) > normal (green). Calling refresh() every 500 ms gives a 1 Hz blink.
 */
class StatusLedDriver {
public:
    StatusLedDriver(int redPin, int greenPin, int bluePin, bool commonAnode);

    void setEventDetected(bool active);
    void setErrorFault(bool active);
    void setNetworkLost(bool active);
    void setBackupPower(bool active);

    void refresh();

private:
    int redPin;
    int greenPin;
    int bluePin;
    bool commonAnode;
    bool eventDetected;
    bool errorFault;
    bool networkLost;
    bool backupPower;
    bool blinkOn;

    void show(LedColor color) const;
};

#endif