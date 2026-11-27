/**
 * @file ElectricityConsumptionSensor.cpp
 * @brief Implements the electricity meter of the consumption node.
 */

#include "ElectricityConsumptionSensor.h"

namespace {
    /**
     * @brief Reads a 16-bit register, sent high byte first.
     * @param data First byte of the register block.
     * @param index Register index.
     */
    uint16_t registerAt(const uint8_t* data, const int index) {
        return static_cast<uint16_t>((data[index * 2] << 8) | data[index * 2 + 1]);
    }

    /**
     * @brief Reads a 32-bit value, sent low register first.
     * @param data First byte of the register block.
     * @param index Index of the low register.
     */
    uint32_t longAt(const uint8_t* data, const int index) {
        return registerAt(data, index) | (static_cast<uint32_t>(registerAt(data, index + 1)) << 16);
    }
}

ElectricityConsumptionSensor::ElectricityConsumptionSensor(HardwareSerial& serialPort, const int receivePin,
                                                           const int transmitPin,
                                                           const double reportThresholdInKilowattHours,
                                                           EventHandler* parentHandler)
    : ConsumptionSensor(receivePin, MEASUREMENT_TYPE, parentHandler),
      serialPort(serialPort),
      reportThresholdInKilowattHours(reportThresholdInKilowattHours)
{
    serialPort.begin(METER_BAUD_RATE, SERIAL_8N1, receivePin, transmitPin);
}

uint16_t ElectricityConsumptionSensor::modbusCrc(const uint8_t* data, const size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
    }
    return crc;
}

void ElectricityConsumptionSensor::sendReadRequest() {
    uint8_t request[REQUEST_LENGTH] = {GENERAL_ADDRESS, READ_INPUT_REGISTERS_FUNCTION, 0x00, 0x00, 0x00, REGISTER_COUNT, 0, 0};
    const uint16_t crc = modbusCrc(request, REQUEST_LENGTH - 2);
    request[REQUEST_LENGTH - 2] = crc & 0xFF;
    request[REQUEST_LENGTH - 1] = crc >> 8;
    serialPort.write(request, REQUEST_LENGTH);
}

bool ElectricityConsumptionSensor::readResponse(uint32_t& energyInWattHours) {
    size_t length = 0;
    while (serialPort.available() > 0) {
        const auto received = static_cast<uint8_t>(serialPort.read());
        if (length < RESPONSE_LENGTH) {
            response[length++] = received;
        }
    }
    if (length < RESPONSE_LENGTH || response[1] != READ_INPUT_REGISTERS_FUNCTION || response[2] != REGISTER_COUNT * 2) {
        return false;
    }
    const uint16_t receivedCrc = response[RESPONSE_LENGTH - 2] | (response[RESPONSE_LENGTH - 1] << 8);
    if (modbusCrc(response, RESPONSE_LENGTH - 2) != receivedCrc) {
        return false;
    }

    const uint8_t* registers = &response[3];
    energyInWattHours = longAt(registers, 5);
    if (energyInWattHours != lastEnergyInWattHours || !hasEnergyReference) {
        Serial.printf("[ElectricityMeter] %.1f V, %.3f A, %.1f W, counter %lu Wh\n",
                      registerAt(registers, 0) / 10.0, longAt(registers, 1) / 1000.0,
                      longAt(registers, 3) / 10.0, static_cast<unsigned long>(energyInWattHours));
    }
    return true;
}

void ElectricityConsumptionSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        return;
    }

    uint32_t energyInWattHours = 0;
    const bool answered = readResponse(energyInWattHours);
    const bool answerMissing = requestPending && !answered;
    sendReadRequest();
    requestPending = true;

    if (answerMissing) {
        reportReadFailure(event);
        suppressPropagation(event);
        return;
    }
    if (!answered) {
        // First sampling: the answer of this request is parsed on the next one
        suppressPropagation(event);
        return;
    }
    reportReadSuccess(event);

    if (!hasEnergyReference) {
        hasEnergyReference = true;
    } else if (energyInWattHours >= lastEnergyInWattHours) {
        accumulateConsumption((energyInWattHours - lastEnergyInWattHours) / WATT_HOURS_PER_KILOWATT_HOUR);
    } else {
        Serial.println(F("[ElectricityMeter] Energy counter reset; difference discarded"));
    }
    lastEnergyInWattHours = energyInWattHours;

    if (getPendingConsumption() >= reportThresholdInKilowattHours) {
        Serial.printf("[ElectricityMeter] %.3f kWh accumulated, report requested\n", getPendingConsumption());
        requestReport(event);
        return;
    }
    suppressPropagation(event);
}

uint32_t ElectricityConsumptionSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t ElectricityConsumptionSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
