/**
 * @file ElectricityConsumptionSensor.h
 * @brief Electricity meter of the consumption node (PZEM-004T v3).
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - Non-blocking reading: each sampling sends a Modbus-RTU request and the next one
 *   parses the answer, so the sampler task never waits for the meter
 * - Sensor fault and recovery events when the meter stops answering
 *
 * @note The framework has no Modbus sensor, so the PZEM-004T protocol lives in this class.
 */

#ifndef ElectricityConsumptionSensor_h
#define ElectricityConsumptionSensor_h

#include "ConsumptionSensor.h"

/**
 * @brief PZEM-004T energy meter that reports the electricity consumed in kWh.
 *
 * The meter is read over UART (Modbus-RTU, 9600 8N1) with the "read input registers"
 * request. Reporting rules (README, consumption node behavior):
 * - The first answer only sets the reference of the energy counter.
 * - Each reading adds the difference between the energy counter of the meter and the
 *   previous reading. A negative difference (counter reset) is discarded.
 * - It requests a report when the accumulated energy reaches `reportThresholdInKilowattHours`.
 */
class ElectricityConsumptionSensor : public ConsumptionSensor {
public:
    /// Measurement type of the Edge API.
    static constexpr auto MEASUREMENT_TYPE = "ELECTRICITY";

    /// Watt-hours in a kilowatt-hour, the unit of the REST API.
    static constexpr double WATT_HOURS_PER_KILOWATT_HOUR = 1000.0;

    /**
     * @brief Constructs the electricity meter and opens its serial port.
     * @param serialPort Hardware serial port wired to the PZEM-004T.
     * @param receivePin GPIO pin connected to the TX output of the PZEM-004T (UART RX).
     *        It also identifies the meter as the source of its events.
     * @param transmitPin GPIO pin connected to the RX input of the PZEM-004T (UART TX).
     * @param reportThresholdInKilowattHours Accumulated energy that requests a report.
     * @param parentHandler Node that receives the propagated events.
     */
    ElectricityConsumptionSensor(HardwareSerial& serialPort, int receivePin, int transmitPin,
                                 double reportThresholdInKilowattHours, EventHandler* parentHandler = nullptr);

protected:
    /**
     * @brief Parses the previous answer of the meter and sends the next request.
     * @param event Mutable event request/response payload.
     * @runs_on{sampler}
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

private:
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 200;
    static constexpr unsigned long METER_BAUD_RATE = 9600;
    static constexpr uint8_t GENERAL_ADDRESS = 0xF8;
    static constexpr uint8_t READ_INPUT_REGISTERS_FUNCTION = 0x04;
    static constexpr uint8_t REGISTER_COUNT = 10;
    static constexpr size_t REQUEST_LENGTH = 8;
    /// Address, function, byte count, 10 registers and CRC.
    static constexpr size_t RESPONSE_LENGTH = 3 + REGISTER_COUNT * 2 + 2;

    HardwareSerial& serialPort;
    const double reportThresholdInKilowattHours;

    // Sampler task state
    uint8_t response[RESPONSE_LENGTH] = {};
    bool requestPending = false;
    bool hasEnergyReference = false;
    uint32_t lastEnergyInWattHours = 0;

    /**
     * @brief Sends the "read input registers" request to the meter.
     */
    void sendReadRequest();

    /**
     * @brief Reads the answer received since the last request and logs the reading.
     * @param energyInWattHours Energy counter of the meter, set when the answer is valid.
     * @return `true` when a complete answer with a valid CRC arrived.
     */
    bool readResponse(uint32_t& energyInWattHours);

    /**
     * @brief Computes the CRC-16/Modbus of a frame.
     * @param data Frame bytes.
     * @param length Number of bytes.
     * @return CRC, sent low byte first.
     */
    static uint16_t modbusCrc(const uint8_t* data, size_t length);
};

#endif /* ElectricityConsumptionSensor_h */
