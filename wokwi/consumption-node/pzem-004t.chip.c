// PZEM-004T v3.0: single-phase AC energy meter with a Modbus-RTU slave on UART (9600 8N1).
//
// Supported requests (CRC-16/Modbus, low byte first):
//   [addr] 0x04 0x0000 0x000A  read the 10 input registers
//   [addr] 0x42                reset the energy counter
// addr is 0x01 (own address) or 0xF8 (general address).
//
// Input registers (each register is sent high byte first):
//   0 voltage 0.1 V | 1-2 current 0.001 A | 3-4 power 0.1 W | 5-6 energy 1 Wh
//   7 frequency 0.1 Hz | 8 power factor 0.01 | 9 alarm (0xFFFF on, 0x0000 off)
// 32-bit values are sent low register first, as the real meter does.
// The "timeScale" control speeds up the energy counter for demos (x1 = real time).

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUD_RATE               9600
#define SLAVE_ADDRESS           0x01
#define GENERAL_ADDRESS         0xF8
#define READ_INPUT_REGISTERS    0x04
#define RESET_ENERGY            0x42
#define REGISTER_COUNT          10
#define GRID_FREQUENCY_HZ       60.0f
#define OVERLOAD_POWER_WATTS    4500.0f
#define ENERGY_PERIOD_MICROS    100000
#define FRAME_GAP_NANOS         4000000ULL
#define RX_BUFFER_SIZE          16
#define TX_BUFFER_SIZE          32

typedef struct {
  uart_dev_t uart;
  uint32_t voltage_attr;
  uint32_t current_attr;
  uint32_t power_factor_attr;
  uint32_t time_scale_attr;
  double energy_wh;
  uint8_t rx_buffer[RX_BUFFER_SIZE];
  uint8_t rx_length;
  uint64_t last_rx_nanos;
  bool tx_busy;
  uint8_t tx_buffer[TX_BUFFER_SIZE];   // Must stay valid until write_done fires
} chip_state_t;

static uint16_t modbus_crc(const uint8_t *data, int length) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < length; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
  }
  return crc;
}

static float active_power_watts(chip_state_t *chip) {
  return attr_read_float(chip->voltage_attr) * attr_read_float(chip->current_attr)
       * attr_read_float(chip->power_factor_attr);
}

static void send_frame(chip_state_t *chip, int length) {
  if (chip->tx_busy) {
    return;   // Previous reply still being transmitted
  }
  uint16_t crc = modbus_crc(chip->tx_buffer, length);
  chip->tx_buffer[length++] = crc & 0xFF;
  chip->tx_buffer[length++] = crc >> 8;
  if (uart_write(chip->uart, chip->tx_buffer, (uint32_t)length)) {
    chip->tx_busy = true;
  }
}

static void put_register(uint8_t *data, int index, uint16_t value) {
  data[index * 2] = value >> 8;
  data[index * 2 + 1] = value & 0xFF;
}

static void put_long(uint8_t *data, int index, uint32_t value) {
  put_register(data, index, value & 0xFFFF);
  put_register(data, index + 1, value >> 16);
}

static void send_registers(chip_state_t *chip, uint8_t address) {
  float voltage = attr_read_float(chip->voltage_attr);
  float current = attr_read_float(chip->current_attr);
  float power_factor = attr_read_float(chip->power_factor_attr);
  float power = active_power_watts(chip);

  chip->tx_buffer[0] = address;
  chip->tx_buffer[1] = READ_INPUT_REGISTERS;
  chip->tx_buffer[2] = REGISTER_COUNT * 2;
  uint8_t *data = &chip->tx_buffer[3];
  put_register(data, 0, (uint16_t)(voltage * 10.0f + 0.5f));
  put_long(data, 1, (uint32_t)(current * 1000.0f + 0.5f));
  put_long(data, 3, (uint32_t)(power * 10.0f + 0.5f));
  put_long(data, 5, (uint32_t)chip->energy_wh);
  put_register(data, 7, (uint16_t)(GRID_FREQUENCY_HZ * 10.0f));
  put_register(data, 8, (uint16_t)(power_factor * 100.0f + 0.5f));
  put_register(data, 9, power > OVERLOAD_POWER_WATTS ? 0xFFFF : 0x0000);
  send_frame(chip, 3 + REGISTER_COUNT * 2);
}

static void handle_frame(chip_state_t *chip) {
  uint8_t *frame = chip->rx_buffer;
  uint8_t address = frame[0];
  if (address != SLAVE_ADDRESS && address != GENERAL_ADDRESS) {
    return;
  }
  if (frame[1] == READ_INPUT_REGISTERS && chip->rx_length == 8) {
    if (modbus_crc(frame, 6) == (frame[6] | (frame[7] << 8))) {
      send_registers(chip, SLAVE_ADDRESS);
    }
    chip->rx_length = 0;
  } else if (frame[1] == RESET_ENERGY && chip->rx_length == 4) {
    if (modbus_crc(frame, 2) == (frame[2] | (frame[3] << 8))) {
      chip->energy_wh = 0.0;
      chip->tx_buffer[0] = SLAVE_ADDRESS;
      chip->tx_buffer[1] = RESET_ENERGY;
      send_frame(chip, 2);
      printf("[PZEM-004T] Energy counter reset\n");
    }
    chip->rx_length = 0;
  }
}

static void on_uart_rx_data(void *user_data, uint8_t byte) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint64_t now = get_sim_nanos();
  if (now - chip->last_rx_nanos > FRAME_GAP_NANOS || chip->rx_length == RX_BUFFER_SIZE) {
    chip->rx_length = 0;   // Silence between frames starts a new request
  }
  chip->last_rx_nanos = now;
  chip->rx_buffer[chip->rx_length++] = byte;
  handle_frame(chip);
}

static void on_uart_write_done(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  chip->tx_busy = false;
}

static void on_energy_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  float hours = (ENERGY_PERIOD_MICROS / 3600000000.0f) * attr_read_float(chip->time_scale_attr);
  chip->energy_wh += active_power_watts(chip) * hours;
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  memset(chip, 0, sizeof(chip_state_t));

  chip->voltage_attr = attr_init_float("voltage", 220.0f);
  chip->current_attr = attr_init_float("current", 5.0f);
  chip->power_factor_attr = attr_init_float("powerFactor", 0.95f);
  chip->time_scale_attr = attr_init_float("timeScale", 1.0f);

  const timer_config_t timer_config = {
    .callback = on_energy_timer,
    .user_data = chip,
  };
  timer_t energy_timer = timer_init(&timer_config);
  timer_start(energy_timer, ENERGY_PERIOD_MICROS, true);

  const uart_config_t uart_config = {
    .tx = pin_init("TX", INPUT_PULLUP),
    .rx = pin_init("RX", INPUT),
    .baud_rate = BAUD_RATE,
    .rx_data = on_uart_rx_data,
    .write_done = on_uart_write_done,
    .user_data = chip,
  };
  chip->uart = uart_init(&uart_config);

  printf("[PZEM-004T] Energy meter ready (Modbus-RTU, UART %d 8N1)\n", BAUD_RATE);
}
