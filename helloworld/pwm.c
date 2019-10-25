#include "pwm.h"
#include <mcs51/8051.h>
#include "stc89xx.h"

pwm_t s_pwm_cfg;

int8_t pwm_start(uint16_t freq, uint32_t witdh_us) {

    // Configure T1 to 200kHz
    //            12Mhz
    // f_t2 = --------------------
    //        6*(TH1,TL1+1)
    // 0xfff6 -> 100kHz
    TL1 = 0x01;
    TH1 = 0x00;

    s_pwm_cfg.interval    = (uint16_t)(100000/((uint32_t)freq));
    s_pwm_cfg.on_interval = witdh_us;
    s_pwm_cfg.on_interval = 1;
    s_pwm_cfg.interval = 10;
    s_pwm_cfg.count    = 0;

    ET1 = 1; // Enable timer1 interrupt
    TR1 = 1; // start timer
    return 0;
}

int8_t pwm_stop() {
    TR1 = 0;
    return 0;
}

void timer1_callback(void) {
    TF0 = 0;
    PWM_PIN = !PWM_PIN;

//    if(s_pwm_cfg.count++ == s_pwm_cfg.interval) {
//        PWM_PIN = 1;
//        s_pwm_cfg.on_count = 0;
//    }
//    if(s_pwm_cfg.on_count++ == s_pwm_cfg.on_interval) {
//        PWM_PIN = 0;
//    }
}
