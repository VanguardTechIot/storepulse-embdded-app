/**
 * @file WaterConsumptionSensor.h
 * @brief Water meter of the consumption node (YF-S201 flow sensor).
 *
 * @note Skeleton: the pulse counting and the reporting rules are still to be written.
 * The framework `WaterFlowSensor` is not used: it never counts the pulses and assumes
 * 7 pulses per milliliter instead of about 450 pulses per liter.
 */

#ifndef WaterConsumptionSensor_h
#define WaterConsumptionSensor_h

#include "ConsumptionSensor.h"

/**
 * @brief YF-S201 water meter that reports the water consumed in m³.
 *
 * Reporting rules (README, consumption node behavior):
 * - The sensor is read every second.
 * - It requests a report when a use ends (the flow stops).
 * - While the water keeps running, it requests a report every 5 minutes.
 */
class WaterConsumptionSensor : public ConsumptionSensor {
public:
    /// Measurement type of the Edge API.
    static constexpr auto MEASUREMENT_TYPE = "WATER";

    /**
     * @brief Constructs the water meter.
     * @param signalGpioPin GPIO pin connected to the YF-S201 signal output.
     * @param parentHandler Node that receives the propagated events.
     */
    explicit WaterConsumptionSensor(int signalGpioPin, EventHandler* parentHandler = nullptr);

protected:
    /**
     * @brief Reads the meter on every sampling request.
     * @param event Mutable event request/response payload.
     * @runs_on{sampler}
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

private:
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 200;
};

#endif /* WaterConsumptionSensor_h */
