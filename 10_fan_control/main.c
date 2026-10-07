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
 * @file main.c 4-wire PWM fan, speed set and read back in RPM over UART, on
 *       the HC6800-ES. Wiring and the UART commands are in the README.
 * @author Thomas Reidemeister
 */
#include <mcs51/8052.h>
#include <stdint.h>

#include "logic.h"

#define PWM_OUT P1_6
#define TACH_IN P3_3            // INT1

// 12 MHz crystal, 12T core: Timer0 and Timer1 count in 1 us steps.
#define TICK_US     20000UL     // control and telemetry period
// Shortest PWM high or low segment. The handler takes about 50 us; a
// shorter segment would only be stretched to that.
#define MIN_SEG_US  60u
// Machine cycles between the PWM handler reading TL0 and writing it back,
// which the reload loses; counted from the handler's listing.
#define T0_FIX_US   11u
// Tach edges closer than this are glitches (30000 RPM at 2 per turn).
#define TACH_MIN_US 500UL
// An edge sooner than half the last interval is a glitch too: the fan
// cannot double its speed within one edge. Capped, so that the first edges
// after a stop (one interval of seconds) are not all rejected.
#define TACH_RATIO_CAP_US 50000UL

// Duty from reset until a command changes it: off.
#define BOOT_DUTY 0u

#define FF_MAX 12u
#define RX_LEN 24u
#define TX_LEN 64u              // power of two

// --- time base: Timer1 free-running, extended to 32 bits ------------------

volatile uint16_t t1_hi;

// Valid while Timer1's overflow handler cannot run: same priority level, or
// interrupts off. An overflow that is pending but not yet counted shows as
// TF1 set with the counter just past zero.
#define NOW_US(dst) do {                                              \
    uint8_t h_, l_;                                                   \
    uint16_t hi_;                                                     \
    do { h_ = TH1; l_ = TL1; } while (h_ != TH1);                     \
    hi_ = t1_hi;                                                      \
    if (TF1 && !(h_ & 0x80u)) { hi_++; }                              \
    (dst) = ((uint32_t)hi_ << 16) | ((uint16_t)h_ << 8) | l_;         \
  } while (0)

// The three high-priority handlers cannot interrupt each other, so they
// share register bank 1.
void timer1_isr(void) __interrupt(TF1_VECTOR) __using(1) {
  t1_hi++;
}

static uint32_t now_us(void) {
  uint32_t t;
  EA = 0;
  NOW_US(t);
  EA = 1;
  return t;
}

// --- tach: INT1 falling edges, timestamped ---------------------------------

volatile uint16_t tach_n;
volatile uint32_t tach_t1;      // latest edge
volatile uint32_t tach_t0;      // the edge before it
static uint32_t tach_dt;        // the last accepted interval

// The tach line is the fan's open collector against the 8051's weak
// pull-up, and switching the PWM while the motor draws full current puts
// glitches on it. Two filters: the pin must still be low when the handler
// reads it, some 130 machine cycles after the edge (a real low phase lasts
// milliseconds), and the edge must not come implausibly soon.
void int1_isr(void) __interrupt(IE1_VECTOR) __using(1) {
  uint32_t t, dt, min_dt;
  NOW_US(t);
  dt = t - tach_t1;
  min_dt = tach_dt >> 1;
  if (min_dt > TACH_RATIO_CAP_US) {
    min_dt = TACH_RATIO_CAP_US;
  }
  if (dt < TACH_MIN_US || dt < min_dt || TACH_IN) {
    return;
  }
  tach_dt = dt;
  tach_t0 = tach_t1;
  tach_t1 = t;
  tach_n++;
}

// --- PWM: Timer0, one interrupt per edge -----------------------------------

volatile uint16_t pwm_on_us;    // 0: constant low
volatile uint16_t pwm_off_us;   // 0: constant high
static __bit pwm_high;

void timer0_isr(void) __interrupt(TF0_VECTOR) __using(1) {
  uint16_t seg;
  uint8_t lo, hi;
  if (pwm_high) {
    if (pwm_off_us) {
      PWM_OUT = 0;
      pwm_high = 0;
      seg = pwm_off_us;
    } else {
      seg = pwm_on_us;
    }
  } else {
    if (pwm_on_us) {
      PWM_OUT = 1;
      pwm_high = 1;
      seg = pwm_on_us;
    } else {
      seg = pwm_off_us;
    }
  }
  // Schedule the next edge seg us after the overflow that got us here, not
  // after now, so interrupt latency does not add up. TH0 is still 0 and TL0
  // holds the microseconds since that overflow, since a high-priority
  // handler always runs well within 256 us of it. TH0 is written first:
  // TL0 cannot wrap before it is written, so no carry is lost.
  seg = (uint16_t)(T0_FIX_US - seg);
  lo = TL0 + (uint8_t)seg;
  hi = (uint8_t)(seg >> 8);
  if (lo < (uint8_t)seg && ++hi == 0) {
    // A short segment, entered late: the edge is already due. Take it now
    // instead of letting the counter wrap and holding the pin for 65 ms.
    hi = 0xFFu;
    lo = 0xFFu;
  }
  TH0 = hi;
  TL0 = lo;
}

// 500 Hz. Of 500 Hz, 1 kHz and 2 kHz, this fan's speed is closest to
// linear in duty at 500 Hz, and it turns from the lowest duty (sweeps made
// with host/). Its datasheet asks for 12 to 50 kHz.
static uint16_t pwm_period_us = 2000u;
static uint16_t duty;           // 0..DUTY_MAX

static void set_duty(uint16_t d) {
  uint16_t on, off;
  if (d > DUTY_MAX) {
    d = DUTY_MAX;
  }
  duty = d;
  on = (uint16_t)(((uint32_t)d * pwm_period_us) >> 10);
  off = pwm_period_us - on;
  // Segments the handler cannot time are rounded to fully off or fully on.
  if (on < MIN_SEG_US) {
    on = 0;
    off = pwm_period_us;
  } else if (off < MIN_SEG_US) {
    on = pwm_period_us;
    off = 0;
  }
  ET0 = 0;
  pwm_on_us = on;
  pwm_off_us = off;
  ET0 = 1;
}

// --- UART: Timer2 baud, interrupt driven both ways -------------------------

static __xdata char rx_buf[RX_LEN];
static uint8_t rx_len;
static volatile __bit line_ready;
static __xdata char tx_buf[TX_LEN];
static volatile uint8_t tx_head, tx_tail;
static volatile __bit tx_busy;

void uart_isr(void) __interrupt(SI0_VECTOR) {
  char c;
  if (RI) {
    c = SBUF;
    RI = 0;
    if (!line_ready) {
      if (c == '\r' || c == '\n') {
        if (rx_len) {
          rx_buf[rx_len] = 0;
          line_ready = 1;
        }
      } else if (rx_len < RX_LEN - 1u) {
        rx_buf[rx_len++] = c;
      }
    }
  }
  if (TI) {
    TI = 0;
    if (tx_tail != tx_head) {
      SBUF = tx_buf[tx_tail];
      tx_tail = (tx_tail + 1u) & (TX_LEN - 1u);
    } else {
      tx_busy = 0;
    }
  }
}

static uint8_t tx_free(void) {
  return (uint8_t)((tx_tail - tx_head - 1u) & (TX_LEN - 1u));
}

static void tx_put(char c) {
  uint8_t next = (tx_head + 1u) & (TX_LEN - 1u);
  while (next == tx_tail) {
    ;                           // full: wait for the handler to drain it
  }
  ES = 0;
  tx_buf[tx_head] = c;
  tx_head = next;
  if (!tx_busy) {
    tx_busy = 1;
    TI = 1;                     // start the handler on the first byte
  }
  ES = 1;
}

static void tx_str(const char *s) {
  while (*s) {
    tx_put(*s++);
  }
}

static void tx_uint(uint16_t v) {
  char buf[5];
  uint8_t i, n = fmt_uint(buf, v);
  for (i = 0; i < n; i++) {
    tx_put(buf[i]);
  }
}

// " <v>", the separator first, which is how every field after the first is
// printed.
static void tx_field(uint16_t v) {
  tx_put(' ');
  tx_uint(v);
}

static void uart_init(void) {
  SCON = 0x50;                  // mode 1, receiver on
  T2CON = 0x30;                 // Timer2 clocks both RX and TX
  // 12 MHz / (32 * 39) = 9615 baud, 0.16% fast. Timer1 is busy as the
  // time base, and at 12 MHz it could not make 9600 anyway.
  RCAP2H = 0xFFu;
  RCAP2L = (uint8_t)(0u - 39u);
  TH2 = RCAP2H;
  TL2 = RCAP2L;
  TR2 = 1;
  ES = 1;
}

// --- control ---------------------------------------------------------------

#define MODE_OPEN 0u
#define MODE_PID  1u

static uint8_t mode = MODE_OPEN;
static uint16_t setpoint;
static __xdata rpm_est_t est;
static __xdata fan_pid_t pid;
static uint16_t rpm_hist[2];    // the two raw readings before this one
static uint16_t rpm_now;        // median of the last three, what the loop sees
static uint8_t tel_div, tel_cnt, tel_seq;
static uint16_t tel_drops;

// Feedforward, RPM against duty in permille, measured at 500 Hz by
// `fan sweep` (mean of the up and down runs; L replaces it at run time).
// The fan does not turn reliably below 15%.
static const CODE uint16_t ff_rpm_default[] = {
  0, 543, 1315, 2088, 2860, 3632, 4405, 5177, 5949, 6722, 7494
};
static const CODE uint16_t ff_pm_default[] = {
  0, 150, 266, 374, 477, 571, 665, 762, 858, 914, 1000
};
static __xdata uint16_t ff_rpm[FF_MAX];
static __xdata uint16_t ff_duty[FF_MAX];
static uint8_t ff_n;

static void tick(void) {
  uint16_t rpm;
  uint32_t t_last, t_prev, now;
  uint16_t n;

  EA = 0;
  n = tach_n;
  t_last = tach_t1;
  t_prev = tach_t0;
  NOW_US(now);
  EA = 1;
  rpm = rpm_update(&est, n, t_last, t_prev, now);
  rpm_now = median3(rpm, rpm_hist[0], rpm_hist[1]);
  rpm_hist[1] = rpm_hist[0];
  rpm_hist[0] = rpm;
  rpm = rpm_now;

  if (mode == MODE_PID) {
    set_duty(pid_step(&pid, setpoint, rpm));
  }

  if (tel_div && ++tel_cnt >= tel_div) {
    tel_cnt = 0;
    // A line is at most 14 bytes. Drop it rather than stall the loop when
    // the UART is behind.
    if (tx_free() < 16u) {
      tel_drops++;
    } else {
      tx_put("0123456789ABCDEF"[tel_seq >> 4]);
      tx_put("0123456789ABCDEF"[tel_seq & 15u]);
      tel_seq++;
      tx_field(rpm);
      tx_field(duty_to_permille(duty));
      tx_put('\n');
    }
  }
}

static void report(void) {
  tx_str("rpm");
  tx_field(rpm_now);
  tx_str(" duty");
  tx_field(duty_to_permille(duty));
  tx_str(" set");
  tx_field(setpoint);
  tx_str(mode == MODE_PID ? " pid" : " open");
  tx_str(" pwm_us");
  tx_field(pwm_period_us);
  tx_str(" drops");
  tx_field(tel_drops);
  tx_str("\r\n");
}

// Release both signal pins and count edges on each. The fan's PWM input
// floats high, so it runs flat out, and whichever pin toggles is the tach.
static void wire_check(void) {
  uint16_t p16 = 0, n;
  uint32_t t0;
  uint8_t last, cur;

  ET0 = 0;
  TR0 = 0;
  PWM_OUT = 1;
  tx_str("wire check, 3 s at full speed\r\n");
  t0 = now_us();
  while (now_us() - t0 < 2000000UL) {
    ;                           // spin up
  }
  EA = 0;
  n = tach_n;
  EA = 1;
  last = PWM_OUT;
  t0 = now_us();
  while (now_us() - t0 < 1000000UL) {
    cur = PWM_OUT;              // reads the pin, since the latch is 1
    if (last && !cur) {
      p16++;
    }
    last = cur;
  }
  EA = 0;
  n = tach_n - n;
  EA = 1;

  tx_str("edges/s P3.3");
  tx_field(n);
  tx_str(" P1.6");
  tx_field(p16);
  if (n > 10u && p16 < 2u) {
    tx_str(": ok\r\n");
  } else if (p16 > 10u && n < 2u) {
    tx_str(": swapped, exchange the P1.6 and P3.3 wires\r\n");
  } else {
    tx_str(": no clean tach, check the wiring\r\n");
  }

  TR0 = 1;
  ET0 = 1;
  set_duty(duty);
}

static __xdata uint16_t arg[3];

static void handle_line(void) {
  const char *s = rx_buf + 1;
  char cmd = rx_buf[0] & 0xDF;  // upper case
  uint8_t i, nargs = 0;

  while (nargs < 3u && (s = parse_uint(s, &arg[nargs]))) {
    nargs++;
  }

  if (cmd == 'D' && nargs == 1u && arg[0] <= 1000u) {
    mode = MODE_OPEN;
    setpoint = 0;
    set_duty(permille_to_duty(arg[0]));
  } else if (cmd == 'S' && nargs == 1u && arg[0] <= 20000u) {
    setpoint = arg[0];
    if (!setpoint) {
      mode = MODE_OPEN;
      set_duty(0);
    } else {
      pid.ff = ff_lookup(ff_rpm, ff_duty, ff_n, setpoint);
      if (mode != MODE_PID) {
        pid_reset(&pid, rpm_now);
        mode = MODE_PID;
      }
    }
  } else if (cmd == 'F' && nargs == 1u && arg[0] >= 100u && arg[0] <= 60000u) {
    pwm_period_us = arg[0];
    set_duty(duty);
  } else if (cmd == 'T' && nargs == 1u && arg[0] <= 250u) {
    tel_div = (uint8_t)arg[0];
    tel_cnt = 0;
    tel_seq = 0;
    tel_drops = 0;
  } else if (cmd == 'K' && nargs == 3u && arg[0] <= 32767u &&
             arg[1] <= 32767u && arg[2] <= 32767u) {
    pid.kp = arg[0];
    pid.ki = arg[1];
    pid.kd = arg[2];
    pid_reset(&pid, rpm_now);
  } else if (cmd == 'L' && nargs == 3u && arg[0] < FF_MAX &&
             arg[1] <= 20000u && arg[2] <= 1000u) {
    i = (uint8_t)arg[0];
    ff_rpm[i] = arg[1];
    ff_duty[i] = permille_to_duty(arg[2]);
    if (ff_n <= i) {
      ff_n = i + 1u;
    }
    pid.ff = ff_lookup(ff_rpm, ff_duty, ff_n, setpoint);
  } else if (cmd == 'L' && nargs == 0u) {
    for (i = 0; i < ff_n; i++) {
      tx_put('L');
      tx_field(i);
      tx_field(ff_rpm[i]);
      tx_field(duty_to_permille(ff_duty[i]));
      tx_str("\r\n");
    }
    return;
  } else if (cmd == 'C' && nargs == 0u) {
    ff_n = 0;
    pid.ff = 0;
  } else if ((cmd == 'R' || cmd == ('?' & 0xDF)) && nargs == 0u) {
    report();
    return;
  } else if (cmd == 'W' && nargs == 0u) {
    wire_check();
    return;
  } else {
    tx_str("err\r\n");
    return;
  }
  tx_str("ok\r\n");
}

void main(void) {
  uint32_t next, now;
  uint8_t i;

  P2 = 0xFF;                    // LEDs off
  // P1.0..P1.3 low keeps the ULN2003 (stepper) off and P1.5 low keeps the
  // buzzer coil off. P1.6 is high from reset, and the fan runs flat out
  // while the bootloader waits; it goes low here, which stops the fan.
  P1 = 0x90;

  for (i = 0; i < sizeof(ff_rpm_default) / sizeof(ff_rpm_default[0]); i++) {
    ff_rpm[i] = ff_rpm_default[i];
    ff_duty[i] = permille_to_duty(ff_pm_default[i]);
  }
  ff_n = i;
  // SIMC PI from the 500 Hz step fits (K 8.1 RPM/permille, tau 0.33 s,
  // theta 0.065 s with the tick), for a closed-loop time constant of tau/2:
  // Kc 0.18 duty counts per RPM, Ti 0.33 s.
  pid.kp = 11811;
  pid.ki = 719;
  pid.kd = 0;
  pid.floor = 102;              // 10%: below it the tach goes quiet

  TMOD = 0x11;                  // Timer0 and Timer1 both mode 1, 16-bit
  TH0 = 0xFFu;
  TL0 = 0xF0u;
  IT1 = 1;                      // INT1 on the falling edge
  // Time base, tach and PWM at high priority: timestamps are never torn and
  // edges are never late behind the UART.
  PT1 = 1;
  PX1 = 1;
  PT0 = 1;
  uart_init();
  pwm_high = 0;
  set_duty(BOOT_DUTY);
  ET1 = 1;
  EX1 = 1;
  TR1 = 1;
  TR0 = 1;
  EA = 1;

  rpm_reset(&est, 0);
  tx_str("\r\n10_fan_control: R reads, D sets duty, S sets RPM, W checks"
         " wires\r\n");

  next = now_us() + TICK_US;
  for (;;) {
    now = now_us();
    if ((int32_t)(now - next) >= 0) {
      next += TICK_US;
      if ((int32_t)(now - next) >= 0) {
        next = now + TICK_US;   // a whole tick behind, e.g. after W
      }
      tick();
    }
    if (line_ready) {
      handle_line();
      rx_len = 0;
      line_ready = 0;
    }
  }
}
