/**
 * @file ConsumptionSensor.h
 * @brief Base class of the meters of the consumption node.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Sensor` subclass that reads locally and only propagates an event when there is
 *   something to report (propagation suppression)
 * - Sketch-level class: it is specific to StorePulse and is not part of the framework
 */

#ifndef ConsumptionSensor_h
#define ConsumptionSensor_h

#include <Arduino.h>
#include <ModestIoT.h>

/**
 * @brief Meter that accumulates a consumption until the node reports it.
 *
 * Each subclass reads its meter, adds the consumed amount with `accumulateConsumption()`
 * and decides when the node must report: it turns the event into
 * `CONSUMPTION_REPORT_REQUESTED_EVENT_IDENTIFIER` with `requestReport()`, or suppresses
 * it with `suppressPropagation()`. The node takes the pending amount with
 * `takePendingConsumption()`, so every report carries the consumption since the previous
 * one, in the unit the REST API expects (m³ for water, kWh for electricity).
 *
 * Subclasses must call `reportReadSuccess()` or `reportReadFailure()` on every reading,
 * before `requestReport()` or `suppressPropagation()`, so the framework can emit the sensor
 * fault and recovery events. Those two helpers leave a fault or recovery event untouched.
 */
class ConsumptionSensor : public Sensor {
public:
    /// Emitted when the meter has a consumption that must be reported now.
    static constexpr int CONSUMPTION_REPORT_REQUESTED_EVENT_IDENTIFIER = 80;

    /**
     * @brief Constructs a consumption meter.
     * @param signalGpioPin GPIO pin that identifies the meter as the source of its events.
     * @param measurementType Measurement type of the Edge API (`WATER` or `ELECTRICITY`).
     * @param parentHandler Node that receives the propagated events.
     */
    ConsumptionSensor(int signalGpioPin, const char* measurementType, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the measurement type of the Edge API.
     * @return `WATER` or `ELECTRICITY`.
     */
    [[nodiscard]] const char* getMeasurementType() const;

    /**
     * @brief Returns the consumption accumulated since the last report, without taking it.
     * @return Pending consumption (m³ or kWh).
     */
    [[nodiscard]] double getPendingConsumption() const;

    /**
     * @brief Takes the consumption accumulated since the last report and resets it.
     * @return Pending consumption (m³ or kWh), never negative.
     * @runs_on{sampler}
     */
    double takePendingConsumption();

protected:
    /// Event identifier that stops the propagation of an event.
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;

    /**
     * @brief Adds a consumed amount to the pending consumption.
     * @param amount Consumed amount (m³ or kWh). Negative or invalid amounts are discarded.
     */
    void accumulateConsumption(double amount);

    /**
     * @brief Turns the event into a report request for the node.
     * @param event Event being processed by the meter.
     */
    void requestReport(Event& event) const;

    /**
     * @brief Stops the propagation of the event: there is nothing to report yet.
     * @param event Event being processed by the meter.
     */
    static void suppressPropagation(Event& event);

private:
    const char* measurementType;
    double pendingConsumption;
};

#endif /* ConsumptionSensor_h */
