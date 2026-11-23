/**
 * @file ElectricityConsumptionSensor.cpp
 * @brief Implements the electricity meter of the consumption node.
 */

#include "ElectricityConsumptionSensor.h"

ElectricityConsumptionSensor::ElectricityConsumptionSensor(const int receivePin, const int transmitPin,
                                                           EventHandler* parentHandler)
    : ConsumptionSensor(receivePin, MEASUREMENT_TYPE, parentHandler),
      transmitPin(transmitPin)
{
}

void ElectricityConsumptionSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        return;
    }

    // TODO(electricity): read the energy counter of the PZEM-004T through UART2
    // (pin = RX, transmitPin = TX), add the difference with accumulateConsumption() and call
    // requestReport() when the pending consumption reaches
    // ELECTRICITY_REPORT_THRESHOLD_IN_KILOWATT_HOURS.
    reportReadSuccess(event);
    suppressPropagation(event);
}

uint32_t ElectricityConsumptionSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t ElectricityConsumptionSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
