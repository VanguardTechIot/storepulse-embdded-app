#ifndef RealTimeClockDriver_h
#define RealTimeClockDriver_h

#include <Arduino.h>
#include <RTClib.h>
#include <time.h>

/**
 * @brief DS3231 real-time clock that keeps the time without network.
 *
 * The clock stores UTC. At boot it sets the system clock, so time(nullptr) and the
 * telemetry timestamps are correct offline. When the network is up, NTP corrects the
 * system clock and the driver writes that time back to the DS3231.
 */
class RealTimeClockDriver {
public:
    bool initialize();

    [[nodiscard]] bool isAvailable() const;

    bool setSystemTimeFromClock();

    void writeUtcTime(time_t utcTime);

private:
    RTC_DS3231 clock;
    bool available = false;
};

#endif