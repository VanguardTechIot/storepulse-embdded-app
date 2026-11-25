#include "RealTimeClockDriver.h"
#include <Wire.h>
#include <sys/time.h>
#include "Config.h"

bool RealTimeClockDriver::initialize() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    available = clock.begin(&Wire);
    if (!available) {
        Serial.println(F("[RTC] DS3231 not found"));
        return false;
    }
    if (clock.lostPower()) {
        Serial.println(F("[RTC] Lost power; the time is not trusted until NTP sets it"));
        return false;
    }
    return setSystemTimeFromClock();
}

bool RealTimeClockDriver::isAvailable() const {
    return available;
}

bool RealTimeClockDriver::setSystemTimeFromClock() {
    if (!available || clock.lostPower()) {
        return false;
    }
    timeval systemTime{};
    systemTime.tv_sec = static_cast<time_t>(clock.now().unixtime());
    return settimeofday(&systemTime, nullptr) == 0;
}

void RealTimeClockDriver::writeUtcTime(const time_t utcTime) {
    if (available) {
        clock.adjust(DateTime(static_cast<uint32_t>(utcTime)));
    }
}