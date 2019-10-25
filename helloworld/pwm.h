#ifndef _pwm_h_

#include <stdint.h>
#include <stddef.h>

#define PWM_PIN P2_2

typedef struct {
    uint16_t interval;
    uint16_t on_interval;
    uint16_t count;
    uint16_t on_count;
} pwm_t;

int8_t pwm_start(uint16_t freq, uint32_t witdh_us);
int8_t pwm_stop();

void timer1_callback(void);

#endif /* _pwm_h_ */
