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
 * @file logic.h Pure logic for 05_enc_stepper: quadrature decode and the
 *       28BYJ-48 half-step table.
 * @author Thomas Reidemeister
 */
#ifndef ENC_STEPPER_LOGIC_H
#define ENC_STEPPER_LOGIC_H

#include <stdint.h>

// Encoder lines packed as AB: A = CLK in bit 1, B = DT in bit 0.
#define AB(a, b) ((uint8_t)(((a) ? 2u : 0u) | ((b) ? 1u : 0u)))

/* Encoders come in two detent styles, and they look identical from outside:
 *  - full-cycle: rests only at AB = 11, four transitions per detent (the
 *    common KY-040);
 *  - half-cycle: rests at AB = 11 AND AB = 00, two transitions per detent.
 *    Decoding one of these as full-cycle counts every second click only,
 *    which is exactly what the first hardware run showed.
 */
#define DETENT_REST 3u
#define DETENT_REST_HALF 0u

typedef struct {
  uint8_t ab;   // last sampled AB
  int8_t acc;   // transitions accumulated since the last rest position
  uint8_t half; // 1: half-cycle detents (rest at 00 and 11), 0: full-cycle
} enc_quad_t;

// One quadrature transition: +1 clockwise, -1 counter-clockwise, 0 for no
// change or an invalid jump (both lines at once -- a missed sample or bounce).
int8_t quad_transition(uint8_t prev_ab, uint8_t ab);

// Feed every sample; returns +1/-1 once per detent and 0 otherwise. Counting
// only on arrival at a rest position, with at least two transitions
// accumulated in one direction, means contact bounce (+1 -1 +1 -1 ...) nets
// out instead of registering steps. Two transitions is half a cycle, which is
// a whole detent for a half-cycle encoder and the minimum believable move for
// a full-cycle one.
int8_t quad_detent(enc_quad_t *q, uint8_t ab);

// Half-step coil pattern for IN1..IN4 (bits 0..3), index 0..7:
// A, AB, B, BC, C, CD, D, DA.
uint8_t stepper_phase(uint8_t index);

#endif
