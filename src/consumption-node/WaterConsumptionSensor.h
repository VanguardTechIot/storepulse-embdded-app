/**
 * @file WaterConsumptionSensor.h
 * @brief Water meter of the consumption node (YF-S201 flow sensor).
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - Hybrid operation: a GPIO interrupt counts every pulse, and the periodic sampling turns
 *   the pulses into a consumption
 * - Propagation suppression: the node only receives an event when a use of water ends, or
 *   every few minutes while the water keeps running
 *
 * @note The framework `WaterFlowSensor` is not used: its interrupt never counts the pulses
 * (the framework interrupt only marks a flag) and it assumes 7 pulses per milliliter instead
 * of about 450 pulses per liter.
 */

#ifndef WaterConsumptionSensor_h
#define WaterConsumptionSensor_h

#include "ConsumptionSensor.h"
#include <atomic>

/**
 * @brief YF-S201 water meter that reports the water consumed in m³.
 *
 * The YF-S201 gives a pulse frequency F = 7.5 × Q, with Q in L/min, which is about
 * 450 pulses per liter. Reporting rules (README, consumption node behavior):
 * - The sensor is read every second.
 * - It requests a report when a use ends: pulses arrived and then stopped for
 *   `useEndTimeoutInMilliseconds`.
 * - While the water keeps running, it requests a report every
 *   `continuousFlowReportIntervalInMilliseconds`, so a leak is reported in time.
 */
class WaterConsumptionSensor : public ConsumptionSensor {
public:
    /// Measurement type of the Edge API.
    static constexpr auto MEASUREMENT_TYPE = "WATER";

    /// Pulses per liter of the YF-S201 (7.5 Hz per L/min × 60 s).
    static constexpr double PULSES_PER_LITER = 450.0;

    /// Pulse frequency per L/min of the YF-S201.
    static constexpr double PULSE_FREQUENCY_IN_HERTZ_PER_LITER_PER_MINUTE = 7.5;

    /// Liters in a cubic meter, the unit of the REST API.
    static constexpr double LITERS_PER_CUBIC_METER = 1000.0;

    /**
     * @brief Constructs the water meter and attaches its pulse interrupt.
     * @param signalGpioPin GPIO pin connected to the YF-S201 signal output.
     * @param useEndTimeoutInMilliseconds Time without pulses that ends a use of water.
     * @param continuousFlowReportIntervalInMilliseconds Report interval while the water keeps running.
     * @param parentHandler Node that receives the propagated events.
     */
    WaterConsumptionSensor(int signalGpioPin, uint32_t useEndTimeoutInMilliseconds,
                           uint32_t continuousFlowReportIntervalInMilliseconds,
                           EventHandler* parentHandler = nullptr);

    ~WaterConsumptionSensor() override;

    /**
     * @brief Returns the flow rate of the last reading.
     * @return Flow rate in liters per minute.
     */
    [[nodiscard]] double getFlowRateInLitersPerMinute() const;

protected:
    /**
     * @brief Turns the pulses counted since the last reading into a consumption.
     * @param event Mutable event request/response payload.
     * @runs_on{sampler}
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

private:
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 200;
    static constexpr double MILLISECONDS_PER_SECOND = 1000.0;
    static constexpr double SECONDS_PER_MINUTE = 60.0;

    const uint32_t useEndTimeoutInMilliseconds;
    const uint32_t continuousFlowReportIntervalInMilliseconds;

    /// Pulses counted by the interrupt since boot (written by the interrupt only).
    std::atomic<uint32_t> pulseCount{0};

    // Sampler task state
    uint32_t pulseCountAtLastReading = 0;
    unsigned long lastReadingAtInMilliseconds = 0;
    unsigned long lastPulseAtInMilliseconds = 0;
    unsigned long lastFlowReportAtInMilliseconds = 0;
    bool waterRunning = false;
    uint32_t pulsesOfCurrentUse = 0;
    double flowRateInLitersPerMinute = 0.0;

    /**
     * @brief Counts one pulse of the YF-S201.
     * @param sensorInstance The WaterConsumptionSensor that owns the interrupt.
     */
    static void IRAM_ATTR onPulse(void* sensorInstance);

    static double pulsesToLiters(uint32_t pulses);
};

#endif /* WaterConsumptionSensor_h */
