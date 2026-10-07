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
 * @file logic.c RPM estimate, PI(D) step, feedforward and number helpers for
 *       10_fan_control.
 * @author Thomas Reidemeister
 */
#include "logic.h"

#define Q16_DUTY_MAX ((int32_t)DUTY_MAX << 16)

void rpm_reset(rpm_est_t *r, uint16_t n) {
  r->ref_n = n;
  r->ref_t = 0;
  r->rpm = 0;
  r->valid = 0;
}

uint16_t rpm_update(rpm_est_t *r, uint16_t n, uint32_t t_last,
                    uint32_t t_prev, uint32_t now) REENTRANT {
  uint16_t avail, m;
  uint32_t t_m, dt, v;

  if (!r->valid) {
    // The first edge after a reset or a stall only sets the reference.
    if (n != r->ref_n) {
      r->ref_n = n;
      r->ref_t = t_last;
      r->valid = 1;
    }
    r->rpm = 0;
    return 0;
  }

  avail = n - r->ref_n;
  m = avail & (uint16_t)~1u;           // whole revolutions only (TACH_PPR == 2)
  if (m) {
    // With an odd count the newest edge is half a turn past the last whole
    // revolution, so that revolution ended on the edge before it.
    t_m = (avail & 1u) ? t_prev : t_last;
    dt = t_m - r->ref_t;
    if (m > 128u || dt == 0) {
      // Not plausible for one 20 ms tick: start over rather than overflow.
      rpm_reset(r, n);
      return 0;
    }
    v = ((uint32_t)m * (60000000UL / TACH_PPR)) / dt;
    r->rpm = v > RPM_MAX ? RPM_MAX : (uint16_t)v;
    r->ref_n += m;
    r->ref_t = t_m;
  } else {
    dt = now - r->ref_t;
    if (dt > STALL_US) {
      rpm_reset(r, n);
      return 0;
    }
    if (dt) {
      // Less than a revolution in dt, so the speed is below one turn per dt.
      v = 60000000UL / dt;
      if (v < r->rpm) {
        r->rpm = (uint16_t)v;
      }
    }
  }
  return r->rpm;
}

void pid_reset(fan_pid_t *c, uint16_t rpm) {
  c->i_acc = 0;
  c->prev_rpm = rpm;
}

uint16_t pid_step(fan_pid_t *c, uint16_t sp, uint16_t rpm) REENTRANT {
  // rpm_update() caps the speed at RPM_MAX and the gains are at most 32767,
  // so with e and the speed change inside int16 no sum below can overflow.
  int16_t e = (int16_t)(sp - rpm);
  int32_t i_new, u;

  u = ((int32_t)c->ff << 16) + (int32_t)e * c->kp -
      (int32_t)(int16_t)(rpm - c->prev_rpm) * c->kd;
  c->prev_rpm = rpm;
  i_new = c->i_acc + (int32_t)e * c->ki;
  if (i_new > Q16_DUTY_MAX) {
    i_new = Q16_DUTY_MAX;
  } else if (i_new < -Q16_DUTY_MAX) {
    i_new = -Q16_DUTY_MAX;
  }
  u += i_new;

  if (u > Q16_DUTY_MAX) {
    if (e < 0) {
      c->i_acc = i_new;               // only integrate back out of saturation
    }
    return DUTY_MAX;
  }
  if (u < ((int32_t)c->floor << 16)) {
    if (e > 0) {
      c->i_acc = i_new;
    }
    return c->floor;
  }
  c->i_acc = i_new;
  return (uint16_t)((u + 0x8000L) >> 16);
}

uint16_t median3(uint16_t a, uint16_t b, uint16_t c) {
  uint16_t t;
  if (a > b) {
    t = a;
    a = b;
    b = t;
  }
  if (b > c) {
    b = c;
  }
  return a > b ? a : b;
}

uint16_t ff_lookup(const uint16_t *rpm_tab, const uint16_t *duty_tab,
                   uint8_t n, uint16_t rpm) REENTRANT {
  uint8_t i;
  uint16_t x0, x1, y0, y1;

  if (n == 0) {
    return 0;
  }
  if (rpm <= rpm_tab[0]) {
    return duty_tab[0];
  }
  for (i = 1; i < n; i++) {
    if (rpm <= rpm_tab[i]) {
      x0 = rpm_tab[i - 1];
      x1 = rpm_tab[i];
      y0 = duty_tab[i - 1];
      y1 = duty_tab[i];
      if (x1 == x0) {
        return y1;
      }
      return (uint16_t)((int32_t)y0 +
          ((int32_t)(rpm - x0) * ((int32_t)y1 - (int32_t)y0)) /
          (int32_t)(x1 - x0));
    }
  }
  return duty_tab[n - 1];
}

uint16_t permille_to_duty(uint16_t pm) {
  if (pm >= 1000u) {
    return DUTY_MAX;
  }
  return (uint16_t)(((uint32_t)pm * DUTY_MAX + 500u) / 1000u);
}

uint16_t duty_to_permille(uint16_t d) {
  return (uint16_t)(((uint32_t)d * 1000u + DUTY_MAX / 2u) >> 10);
}

const char *parse_uint(const char *s, uint16_t *v) {
  uint16_t x = 0;
  uint8_t any = 0;

  while (*s == ' ') {
    s++;
  }
  while (*s >= '0' && *s <= '9') {
    if (x > 6553u || (x == 6553u && *s > '5')) {
      x = 65535u;
    } else {
      x = x * 10u + (uint8_t)(*s - '0');
    }
    any = 1;
    s++;
  }
  *v = x;
  return any ? s : 0;
}

static const CODE uint16_t pow10[5] = {10000u, 1000u, 100u, 10u, 1u};

uint8_t fmt_uint(char *buf, uint16_t v) {
  uint8_t i, n = 0;
  char digit;

  for (i = 0; i < 5u; i++) {
    digit = '0';
    while (v >= pow10[i]) {
      v -= pow10[i];
      digit++;
    }
    if (digit != '0' || n || i == 4u) {
      buf[n++] = digit;
    }
  }
  return n;
}
