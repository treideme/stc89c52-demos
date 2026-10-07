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
 * @file logic.c Quadrature decode and half-step table for 05_enc_stepper.
 * @author Thomas Reidemeister
 */
#include "logic.h"

// Indexed by (prev << 2) | cur. Clockwise is 00 -> 01 -> 11 -> 10 -> 00,
// the sequence in the post's state table: on CLK's falling edge (10 -> 00)
// DT is low, which is the published demo's "DT == 0 means clockwise".
static const int8_t quad_table[16] = {
  /* prev 00 */  0, +1, -1,  0,
  /* prev 01 */ -1,  0,  0, +1,
  /* prev 10 */ +1,  0,  0, -1,
  /* prev 11 */  0, -1, +1,  0,
};

static const uint8_t half_step[8] = {
  0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09
};

int8_t quad_transition(uint8_t prev_ab, uint8_t ab) {
  return quad_table[((prev_ab & 3u) << 2) | (ab & 3u)];
}

int8_t quad_detent(enc_quad_t *q, uint8_t ab) {
  int8_t d = 0;
  ab &= 3u;
  q->acc += quad_transition(q->ab, ab);
  q->ab = ab;
  if (ab == DETENT_REST || (q->half && ab == DETENT_REST_HALF)) {
    if (q->acc >= 2) {
      d = 1;
    } else if (q->acc <= -2) {
      d = -1;
    }
    q->acc = 0;
  }
  return d;
}

uint8_t stepper_phase(uint8_t index) {
  return half_step[index & 7u];
}
