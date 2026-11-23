/**
 * @file ConsumptionSensor.cpp
 * @brief Implements the base class of the meters of the consumption node.
 */

#include "ConsumptionSensor.h"

ConsumptionSensor::ConsumptionSensor(const int signalGpioPin, const char* measurementType,
                                     EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      measurementType(measurementType),
      pendingConsumption(0.0)
{
}

const char* ConsumptionSensor::getMeasurementType() const {
    return measurementType;
}

double ConsumptionSensor::getPendingConsumption() const {
    return pendingConsumption;
}

double ConsumptionSensor::takePendingConsumption() {
    const double consumption = pendingConsumption;
    pendingConsumption = 0.0;
    return consumption;
}

void ConsumptionSensor::accumulateConsumption(const double amount) {
    // Also discards NaN, because every comparison with NaN is false
    if (!(amount > 0.0)) {
        return;
    }
    pendingConsumption += amount;
}

void ConsumptionSensor::requestReport(Event& event) const {
    // A fault or recovery event set by reportReadFailure() or reportReadSuccess() wins:
    // the pending consumption stays and goes in the next report
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        event = Event(CONSUMPTION_REPORT_REQUESTED_EVENT_IDENTIFIER, pin);
    }
}

void ConsumptionSensor::suppressPropagation(Event& event) {
    // A fault or recovery event set by reportReadFailure() or reportReadSuccess() must reach the node
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
    }
}
