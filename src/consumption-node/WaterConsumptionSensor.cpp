/**
 * @file WaterConsumptionSensor.cpp
 * @brief Implements the water meter of the consumption node.
 */

#include "WaterConsumptionSensor.h"

WaterConsumptionSensor::WaterConsumptionSensor(const int signalGpioPin,
                                               const uint32_t useEndTimeoutInMilliseconds,
                                               const uint32_t continuousFlowReportIntervalInMilliseconds,
                                               EventHandler* parentHandler)
    : ConsumptionSensor(signalGpioPin, MEASUREMENT_TYPE, parentHandler),
      useEndTimeoutInMilliseconds(useEndTimeoutInMilliseconds),
      continuousFlowReportIntervalInMilliseconds(continuousFlowReportIntervalInMilliseconds)
{
    // GPIO 35 is input only and has no internal pull-up: in Wokwi the chip drives the line,
    // and a physical YF-S201 would need an external pull-up and a 5 V to 3.3 V divider
    pinMode(pin, INPUT);
    lastReadingAtInMilliseconds = millis();
    attachInterruptArg(digitalPinToInterrupt(pin), onPulse, this, RISING);
}

WaterConsumptionSensor::~WaterConsumptionSensor() {
    detachInterrupt(digitalPinToInterrupt(pin));
}

void IRAM_ATTR WaterConsumptionSensor::onPulse(void* sensorInstance) {
    static_cast<WaterConsumptionSensor*>(sensorInstance)->pulseCount.fetch_add(1, std::memory_order_relaxed);
}

double WaterConsumptionSensor::pulsesToLiters(const uint32_t pulses) {
    return static_cast<double>(pulses) / PULSES_PER_LITER;
}

double WaterConsumptionSensor::getFlowRateInLitersPerMinute() const {
    return flowRateInLitersPerMinute;
}

void WaterConsumptionSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        return;
    }

    const unsigned long now = millis();
    const uint32_t currentPulseCount = pulseCount.load(std::memory_order_relaxed);
    // Unsigned subtraction also works when the counter wraps around
    const uint32_t newPulses = currentPulseCount - pulseCountAtLastReading;
    const unsigned long elapsedInMilliseconds = now - lastReadingAtInMilliseconds;
    pulseCountAtLastReading = currentPulseCount;
    lastReadingAtInMilliseconds = now;

    // The YF-S201 has no failed readings: without flow there are simply no pulses
    reportReadSuccess(event);

    flowRateInLitersPerMinute = elapsedInMilliseconds > 0
        ? static_cast<double>(newPulses) / PULSE_FREQUENCY_IN_HERTZ_PER_LITER_PER_MINUTE
              / (static_cast<double>(elapsedInMilliseconds) / MILLISECONDS_PER_SECOND)
        : 0.0;

    if (newPulses > 0) {
        accumulateConsumption(pulsesToLiters(newPulses) / LITERS_PER_CUBIC_METER);
        lastPulseAtInMilliseconds = now;
        pulsesOfCurrentUse += newPulses;

        if (!waterRunning) {
            waterRunning = true;
            lastFlowReportAtInMilliseconds = now;
            Serial.println(F("[WaterMeter] Water use started"));
        }
        Serial.printf("[WaterMeter] Flow %.2f L/min, %.2f L in this use\n",
                      flowRateInLitersPerMinute, pulsesToLiters(pulsesOfCurrentUse));

        if (now - lastFlowReportAtInMilliseconds >= continuousFlowReportIntervalInMilliseconds) {
            lastFlowReportAtInMilliseconds = now;
            Serial.println(F("[WaterMeter] Water still running: report requested"));
            requestReport(event);
            return;
        }
        suppressPropagation(event);
        return;
    }

    if (waterRunning && now - lastPulseAtInMilliseconds >= useEndTimeoutInMilliseconds) {
        waterRunning = false;
        Serial.printf("[WaterMeter] Water use ended: %.2f L, report requested\n", pulsesToLiters(pulsesOfCurrentUse));
        pulsesOfCurrentUse = 0;
        requestReport(event);
        return;
    }

    suppressPropagation(event);
}

uint32_t WaterConsumptionSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t WaterConsumptionSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
