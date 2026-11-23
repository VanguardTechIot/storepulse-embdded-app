/**
 * @file WaterConsumptionSensor.cpp
 * @brief Implements the water meter of the consumption node.
 */

#include "WaterConsumptionSensor.h"

WaterConsumptionSensor::WaterConsumptionSensor(const int signalGpioPin, EventHandler* parentHandler)
    : ConsumptionSensor(signalGpioPin, MEASUREMENT_TYPE, parentHandler)
{
    // GPIO 35 is input only and has no internal pull-up: the pull-up is external
    pinMode(pin, INPUT);
}

void WaterConsumptionSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        return;
    }

    // TODO(water): convert the pulses counted since the last reading to m³, add them with
    // accumulateConsumption() and call requestReport() when the flow stops or after
    // WATER_CONTINUOUS_FLOW_REPORT_INTERVAL_IN_MILLISECONDS of continuous flow.
    reportReadSuccess(event);
    suppressPropagation(event);
}

uint32_t WaterConsumptionSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t WaterConsumptionSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
