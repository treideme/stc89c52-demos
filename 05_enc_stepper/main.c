/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file main.c Rotary knob drives a 28BYJ-48 stepper 1:1 on the HC6800-ES.
 *
 * Wiring (see doc/HC6800-ES Schematic.pdf for the board):
 *   KY-040 encoder on ISP1: + -> 5V, GND -> GND, CLK -> P1.7, DT -> P1.6,
 *                           SW -> P1.5 (shared with the buzzer: it clicks).
 *   28BYJ-48 on header P3 ("+ A B C D"): ULN2003 IN1..IN4 are P1.0..P1.3,
 *                           OUT1..OUT4 are A..D, and pin 1 is VCC. The JST
 *                           plug goes straight on, red on "+": its wire order
 *                           (orange, yellow, pink, blue) is the firing order.
 *   P1.0..P1.7 are also the keypad, so leave the keypad alone while running.
 *
 * One detent turns the shaft by one detent of the knob: 4096 half-steps per
 * output revolution, 30 detents per knob revolution (the module used here), so
 * 136.53 half-steps per detent. It is computed exactly from the detent
 * count, so nothing drifts.
 *
 * Everything time-critical runs in a 1 ms Timer0 tick at 12 MHz:
 *   - sample the encoder every tick (the sampling interval is the debounce),
 *   - take one half-step toward the target every STEP_TICKS ticks,
 *   - de-energise the coils after IDLE_TICKS at the target. Each 5 V coil is
 *     about 50 ohm, so a held half-step draws 100-200 mA and warms the motor
 *     for nothing, while the 64:1 gearbox holds position by itself.
 * The main loop only mirrors the detent count onto LEDs D1..D8.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>
#include <stdint.h>

#include "logic.h"

#define SW  P1_5
#define DT  P1_6
#define CLK P1_7
#define LED P2

// 12 MHz crystal, 12T core: 1 machine cycle = 1 us. The HC6800-ES ships
// with a 12 MHz crystal, not the 11.0592 MHz that UART examples assume.
#define TICK_US      1000u
#define STEP_TICKS   2u      // 2 ms per half-step = 500 half-steps/s
#define IDLE_TICKS   200u    // coils off after 200 ms at the target
#define SW_TICKS     20u     // switch must read low for 20 ms to count
#define HALF_STEPS_PER_REV 4096L
// Clicks per knob revolution, for the 1:1 mapping. Count them on the actual
// knob: KY-040-style modules ship with 20 or 30. The module used here has 30;
// at 20 the shaft overturned the knob by half again.
#define DETENTS_PER_REV    30L
// 1 if the knob clicks at every half quadrature cycle (rests at both 00 and
// 11), 0 if it clicks once per full cycle. The module used here is
// half-cycle: decoded as full-cycle, it moved once per two clicks.
#define ENC_HALF_CYCLE     1u
// The half-step target is an int16_t: 150 detents = 20480 half-steps, which
// is 5 output turns either way and leaves room below 32767 for a 20-detent
// knob too (30720). Past that, further detents are ignored rather than
// wrapping the target round to the other end.
#define DETENT_LIMIT       150

// P1.4..P1.7 are inputs (encoder, unused keypad row): keep their latches at 1
// on every write so the quasi-bidirectional pins stay pulled up.
#define P1_INPUTS 0xF0u

volatile int16_t detents;       // knob position, in detents
static int16_t position;        // motor position, in half-steps
static int16_t target;          // motor target, in half-steps
static enc_quad_t quad;
static uint8_t step_div;
static uint8_t idle;
static uint8_t sw_low;
static __bit sw_latched;

static void timer0_reload(void) {
  TH0 = (uint8_t)((uint16_t)(0u - TICK_US) >> 8);
  TL0 = (uint8_t)(0u - TICK_US);
}

void timer0_isr(void) __interrupt(TF0_VECTOR) {
  int8_t d;
  timer0_reload();

  // Encoder: one sample per tick.
  d = quad_detent(&quad, AB(CLK, DT));
  if (d && detents + d <= DETENT_LIMIT && detents + d >= -DETENT_LIMIT) {
    detents += d;
    target = (int16_t)(((int32_t)detents * HALF_STEPS_PER_REV) / DETENTS_PER_REV);
  }

  // Push button: re-zero once per press, after SW_TICKS of stable low.
  if (!SW) {
    if (sw_low < SW_TICKS) {
      sw_low++;
    } else if (!sw_latched) {
      sw_latched = 1;
      detents = 0;
      target = 0;
      position = 0;
    }
  } else {
    sw_low = 0;
    sw_latched = 0;
  }

  // Stepper: one half-step toward the target every STEP_TICKS.
  if (++step_div < STEP_TICKS) {
    return;
  }
  step_div = 0;
  if (position != target) {
    position += (target > position) ? 1 : -1;
    P1 = P1_INPUTS | stepper_phase((uint8_t)position);
    idle = 0;
  } else if (idle < IDLE_TICKS / STEP_TICKS) {
    if (++idle == IDLE_TICKS / STEP_TICKS) {
      P1 = P1_INPUTS;           // all coils off
    }
  }
}

void main(void) {
  int16_t shown;

  LED = 0xFF;                   // LEDs off (active low)
  P1 = P1_INPUTS;               // coils off, inputs pulled up
  quad.ab = AB(CLK, DT);
  quad.acc = 0;
  quad.half = ENC_HALF_CYCLE;

  TMOD = (TMOD & 0xF0u) | 0x01u;  // Timer0 mode 1, 16-bit
  timer0_reload();
  ET0 = 1;
  EA = 1;
  TR0 = 1;

  for (;;) {
    EA = 0;                     // 16-bit read of an ISR-owned value
    shown = detents;
    EA = 1;
    LED = (uint8_t)~(uint8_t)shown;   // binary count of detents, active low
  }
}
