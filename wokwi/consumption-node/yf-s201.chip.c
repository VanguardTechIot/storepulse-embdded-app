// Wokwi Custom Chip - YF-S201 water flow sensor
// For information and examples see: https://link.wokwi.com/custom-chips-alpha
//
// SPDX-License-Identifier: MIT
// Created by VanguardTech on 2026/11/22, based on the course BH1750 custom chip
//
// The YF-S201 outputs a square wave whose frequency is F = 7.5 x Q, with Q the flow rate
// in liters per minute (about 450 pulses per liter). The "Flow rate" control of the chip
// sets Q while the simulation runs; at 0 L/min the output stays low.

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

const double PULSE_FREQUENCY_IN_HERTZ_PER_LITER_PER_MINUTE = 7.5;
const double NANOSECONDS_PER_SECOND                        = 1e9;
const uint32_t IDLE_CHECK_INTERVAL_IN_MICROSECONDS         = 100000;
const float DEFAULT_FLOW_RATE_IN_LITERS_PER_MINUTE         = 0.0f;

typedef struct {
  pin_t output_pin;
  uint32_t flow_rate_attr;
  timer_t timer;
  bool output_high;
} chip_state_t;

static void on_timer(void *user_data);

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->output_pin = pin_init("OUT", OUTPUT_LOW);
  chip->output_high = false;

  // This attribute can be edited by the user. It's defined in yf-s201.chip.json:
  chip->flow_rate_attr = attr_init_float("flowRate", DEFAULT_FLOW_RATE_IN_LITERS_PER_MINUTE);

  const timer_config_t timer_config = {
    .user_data = chip,
    .callback = on_timer,
  };
  chip->timer = timer_init(&timer_config);
  timer_start(chip->timer, IDLE_CHECK_INTERVAL_IN_MICROSECONDS, false);

  printf("YF-S201 flow sensor ready\n");
}

// Toggles the output every half period. The flow rate is read again on every edge, so a
// change of the control takes effect within one period.
static void on_timer(void *user_data) {
  chip_state_t *chip = user_data;
  const float flow_rate = attr_read_float(chip->flow_rate_attr);

  if (flow_rate <= 0.0f) {
    chip->output_high = false;
    pin_write(chip->output_pin, LOW);
    timer_start(chip->timer, IDLE_CHECK_INTERVAL_IN_MICROSECONDS, false);
    return;
  }

  chip->output_high = !chip->output_high;
  pin_write(chip->output_pin, chip->output_high ? HIGH : LOW);

  const double frequency_in_hertz = PULSE_FREQUENCY_IN_HERTZ_PER_LITER_PER_MINUTE * flow_rate;
  timer_start_ns_d(chip->timer, NANOSECONDS_PER_SECOND / (2.0 * frequency_in_hertz), false);
}
