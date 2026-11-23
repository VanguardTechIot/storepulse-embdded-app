/**
 * @file ElectricityConsumptionSensor.h
 * @brief Electricity meter of the consumption node (PZEM-004T v3).
 *
 * @note Skeleton: the Modbus-RTU reading and the reporting rule are still to be written.
 */

#ifndef ElectricityConsumptionSensor_h
#define ElectricityConsumptionSensor_h

#include "ConsumptionSensor.h"

/**
 * @brief PZEM-004T energy meter that reports the electricity consumed in kWh.
 *
 * Reporting rules (README, consumption node behavior):
 * - Each reading adds the difference between the energy counter of the meter and the
 *   previous reading. A negative difference (counter reset) is discarded.
 * - It requests a report when the accumulated energy reaches 10 Wh (0.01 kWh).
 */
class ElectricityConsumptionSensor : public ConsumptionSensor {
public:
    /// Measurement type of the Edge API.
    static constexpr auto MEASUREMENT_TYPE = "ELECTRICITY";

    /**
     * @brief Constructs the electricity meter.
     * @param receivePin GPIO pin connected to the TX output of the PZEM-004T (UART2 RX).
     *        It also identifies the meter as the source of its events.
     * @param transmitPin GPIO pin connected to the RX input of the PZEM-004T (UART2 TX).
     * @param parentHandler Node that receives the propagated events.
     */
    ElectricityConsumptionSensor(int receivePin, int transmitPin, EventHandler* parentHandler = nullptr);

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
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;

    int transmitPin;
};

#endif /* ElectricityConsumptionSensor_h */
