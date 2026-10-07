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
 * @file logic.h Pure logic for 10_fan_control: RPM from tach timestamps,
 *       the PI(D) step, the feedforward table and the UART number helpers.
 * @author Thomas Reidemeister
 */
#ifndef FAN_PID_LOGIC_H
#define FAN_PID_LOGIC_H

#include <stdint.h>

// The three 32-bit routines keep their locals on the stack. As static data,
// SDCC's default, they would not fit in the 8051's 128 bytes next to
// everything else. CODE puts constant tables in flash.
#ifdef __SDCC
#define REENTRANT __reentrant
#define CODE __code
#else
#define REENTRANT
#define CODE
#endif

// Duty is kept in 1/1024 of full scale internally, so a duty times a PWM
// period is a multiply and a shift rather than a division. The UART speaks
// permille.
#define DUTY_MAX 1024u

// Tach falling edges per revolution. The datasheet's FG waveform shows two
// pulses per turn (T1..T4), which is the norm for a 4-pole fan motor.
// rpm_update() relies on this being 2.
#define TACH_PPR 2u

// With no full revolution for this long the fan counts as stopped (30 RPM).
#define STALL_US 2000000UL

// Readings are capped here; it is four times the fan's rated speed.
#define RPM_MAX 30000u

typedef struct {
  uint16_t ref_n;   // edge count at the reference edge
  uint32_t ref_t;   // timestamp of the reference edge, us
  uint16_t rpm;     // latest estimate
  uint8_t valid;    // 0 until an edge has set the reference
} rpm_est_t;

// n: tach edges seen so far, as counted by the INT1 handler.
void rpm_reset(rpm_est_t *r, uint16_t n);

// Call once per control tick. t_last and t_prev are the timestamps of the
// latest two edges and now is the current time, all from one free-running
// microsecond clock. The estimate only ever spans whole revolutions, so the
// two magnet poles' unequal half-periods cancel. With no full revolution
// since the last estimate it is capped at what the elapsed time allows,
// which makes a spin-down read as one rather than as a frozen value.
uint16_t rpm_update(rpm_est_t *r, uint16_t n, uint32_t t_last,
                    uint32_t t_prev, uint32_t now) REENTRANT;

// Gains are Q16 and at most 32767, i.e. below half a duty count per RPM.
typedef struct {
  int32_t kp;       // duty counts per RPM
  int32_t ki;       // duty counts per RPM, per control tick
  int32_t kd;       // duty counts per RPM of change, per control tick
  int32_t i_acc;    // integrator, Q16 duty counts
  uint16_t prev_rpm;
  uint16_t ff;      // feedforward duty for the current setpoint
  uint16_t floor;   // lowest duty the loop may command
} fan_pid_t;

// Zero the integrator and seed the derivative with the current speed, so a
// change of setpoint or mode does not kick the output.
void pid_reset(fan_pid_t *c, uint16_t rpm);

// One control step: feedforward plus PI(D), derivative on the measurement.
// The integrator stops when the output is saturated in the direction of the
// error (conditional integration), so it cannot wind up while the fan is at
// full speed or at the floor. Returns a duty in floor..DUTY_MAX. The floor
// exists because at 0% this fan stops driving its tach output while it
// coasts, so the loop would be steering by a speed it cannot see.
uint16_t pid_step(fan_pid_t *c, uint16_t sp, uint16_t rpm) REENTRANT;

// Median of three readings: drops a one-tick spike from a tach glitch that
// got past the edge filters, for one tick (20 ms) of extra delay.
uint16_t median3(uint16_t a, uint16_t b, uint16_t c);

// Duty for a target RPM, by linear interpolation in a table sorted by rpm.
// Clamps to the first and last entries outside it; 0 for an empty table.
uint16_t ff_lookup(const uint16_t *rpm_tab, const uint16_t *duty_tab,
                   uint8_t n, uint16_t rpm) REENTRANT;

// Permille <-> internal duty counts.
uint16_t permille_to_duty(uint16_t pm);
uint16_t duty_to_permille(uint16_t d);

// Skip spaces, then read a decimal number. Returns the character after it,
// or 0 (a null pointer) if there were no digits. Saturates at 65535.
const char *parse_uint(const char *s, uint16_t *v);

// Unsigned decimal, no leading zeros, no terminator, at most 5 characters.
// Returns the length. Repeated subtraction: SDCC's software division would
// cost more.
uint8_t fmt_uint(char *buf, uint16_t v);

#endif
