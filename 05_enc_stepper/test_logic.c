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
 * @file test_logic.c Host-side tests for 05_enc_stepper/logic.c, built with
 *       the native compiler by `meson test`.
 * @author Thomas Reidemeister
 */
#include <stdio.h>
#include "logic.h"

static int failures;

#define CHECK(cond, ...) do { if (!(cond)) { failures++; \
  printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int feed(enc_quad_t *q, const uint8_t *seq, int n) {
  int total = 0, i;
  for (i = 0; i < n; i++) {
    total += quad_detent(q, seq[i]);
  }
  return total;
}

int main(void) {
  // Clockwise per the post's table: 11 -> 10 -> 00 -> 01 -> 11 is one detent.
  static const uint8_t cw[] = {2, 0, 1, 3};
  static const uint8_t ccw[] = {1, 0, 2, 3};
  // Bounce on the first edge, then a clean clockwise detent.
  static const uint8_t bouncy_cw[] = {2, 3, 2, 3, 2, 0, 1, 0, 1, 3};
  // A half turn and back: no detent.
  static const uint8_t back[] = {2, 0, 2, 3};
  // A two-line jump (missed sample): ignored, no detent.
  static const uint8_t jump[] = {0, 3};
  enc_quad_t q;
  int i, ones, prev, d;

  q.ab = 3; q.acc = 0; q.half = 0;
  CHECK(feed(&q, cw, 4) == 1, "one clockwise detent");
  CHECK(feed(&q, cw, 4) == 1, "a second clockwise detent");
  CHECK(feed(&q, ccw, 4) == -1, "one counter-clockwise detent");
  CHECK(feed(&q, bouncy_cw, 10) == 1, "bounce nets out to one detent");
  CHECK(feed(&q, back, 4) == 0, "half a cycle and back is no detent");
  CHECK(feed(&q, jump, 2) == 0, "an invalid jump is not a detent");

  // Half-cycle encoder: every rest position (00 and 11) is a detent, so a full
  // quadrature cycle is two clicks. Decoded as full-cycle, the same cycle is
  // one click, which is the bug the first hardware run showed.
  {
    static const uint8_t cw_half1[] = {2, 0};      // 11 -> 10 -> 00
    static const uint8_t cw_half2[] = {1, 3};      // 00 -> 01 -> 11
    static const uint8_t ccw_half[] = {1, 0};      // 11 -> 01 -> 00
    static const uint8_t bouncy_half[] = {2, 3, 2, 0, 2, 0};  // bounce, then settle at 00
    enc_quad_t h;
    h.ab = 3; h.acc = 0; h.half = 1;
    CHECK(feed(&h, cw_half1, 2) == 1, "half-cycle: 11 -> 00 clockwise is one click");
    CHECK(feed(&h, cw_half2, 2) == 1, "half-cycle: 00 -> 11 clockwise is the next click");
    CHECK(feed(&h, ccw_half, 2) == -1, "half-cycle: counter-clockwise click");
    h.ab = 3; h.acc = 0;
    CHECK(feed(&h, bouncy_half, 6) == 1, "half-cycle: bounce nets out to one click");
    h.ab = 3; h.acc = 0; h.half = 0;
    CHECK(feed(&h, cw, 4) == 1, "the same full cycle is one click in full-cycle mode");
    h.ab = 3; h.acc = 0; h.half = 1;
    CHECK(feed(&h, cw, 4) == 2, "and two clicks in half-cycle mode");
  }

  // Matches the published demo: a CLK falling edge (10 -> 00) with DT low is
  // clockwise.
  CHECK(quad_transition(2, 0) == 1, "10 -> 00 is clockwise");
  CHECK(quad_transition(3, 0) == 0, "11 -> 00 is invalid");

  // Half-step table: every entry energises one or two adjacent coils, and
  // consecutive entries differ by exactly one coil (no skipped phase).
  for (i = 0; i < 8; i++) {
    uint8_t p = stepper_phase((uint8_t)i), n = stepper_phase((uint8_t)(i + 1));
    uint8_t diff = (uint8_t)(p ^ n);
    ones = (p & 1) + ((p >> 1) & 1) + ((p >> 2) & 1) + ((p >> 3) & 1);
    CHECK(ones == 1 || ones == 2, "phase %d energises %d coils", i, ones);
    CHECK(diff && !(diff & (diff - 1)), "phase %d -> %d changes one coil", i, i + 1);
    CHECK((p & 0xF0u) == 0, "phase %d stays in IN1..IN4", i);
  }
  CHECK(stepper_phase(8) == stepper_phase(0), "index wraps modulo 8");
  CHECK(stepper_phase((uint8_t)-1) == stepper_phase(7), "negative positions wrap");

  // Detent -> half-step mapping used in main.c: one knob revolution is
  // exactly one output revolution, for both common knobs, and the per-detent
  // rounding never accumulates.
  {
    static const int knobs[] = {20, 30};
    int k;
    for (k = 0; k < 2; k++) {
      int n = knobs[k], lo = 4096 / n;
      prev = 0;
      for (d = 1; d <= n; d++) {
        int t = (int)(((long)d * 4096L) / (long)n);
        CHECK(t - prev == lo || t - prev == lo + 1, "%d-click knob: detent %d moves %d", n, d, t - prev);
        prev = t;
      }
      CHECK(prev == 4096, "%d detents = 4096 half-steps, got %d", n, prev);
      // The clamp in main.c keeps the int16_t target in range.
      CHECK(((long)150 * 4096L) / (long)n <= 32767L, "%d-click knob: 150 detents fits int16", n);
    }
  }

  if (failures) {
    printf("%d failure(s)\n", failures);
    return 1;
  }
  printf("all logic checks passed\n");
  return 0;
}
