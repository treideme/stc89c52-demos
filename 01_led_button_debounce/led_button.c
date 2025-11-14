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
 * @file led_button.c Button debouncing with a timer.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>

void timer0_init(void) {
  TMOD &= 0xF0;	/* Clear Timer 0 mode bits */
  TMOD |= 0x1;	/* Set Timer 0 mode to 16-bit */
  TH0 = 0x3c;	/* Set Timer 0 high byte for 16-bit mode */
  TL0 = 0xb0;	/* Set Timer 0 low byte for 16-bit mode */
  TF0 = 0;	/* Clear Timer 0 overflow flag */
  TR0 = 1;	/* Start Timer 0 */
}

void main(void) {
  timer0_init();

  ET0 = 1;	/* Enable Timer 0 interrupt */
  EA  = 1; /* Enable global interrupts */

  for(;;);
}

__bit led_0_state = 0;
unsigned char button_0_counter = 0;
__bit led_1_state = 0;
unsigned char button_1_counter = 0;
__bit led_2_state = 0;
unsigned char button_2_counter = 0;
__bit led_3_state = 0;
unsigned char button_3_counter = 0;

void tf0_isr(void) __interrupt(TF0_VECTOR) {
    // Adance counters if started hysteresis
    if(button_0_counter) {
      button_0_counter++;
      if(button_0_counter >= 20)
        button_0_counter = 0; // Reset counter after hysteresis period
    }
    if(button_1_counter) {
      button_1_counter++;
      if (button_1_counter >= 20)
        button_1_counter = 0; // Reset counter after hysteresis period
    }
    if(button_2_counter) {
      button_2_counter++;
      if (button_2_counter >= 20)
        button_2_counter = 0; // Reset counter after hysteresis period
    }
    if(button_3_counter) {
      button_3_counter++;
      if (button_3_counter >= 20)
        button_3_counter = 0; // Reset counter after hysteresis period
    }

    // Button 0
    if(P3_1 == 0 && button_0_counter == 0) {
      led_0_state = !led_0_state;
      button_0_counter++; // Reset counter after valid press
    }
    if(P3_0 == 0 && button_1_counter == 0) {
      led_1_state = !led_1_state;
      button_1_counter++; // Reset counter after valid press
    }
    if(P3_2 == 0 && button_2_counter == 0) {
      led_2_state = !led_2_state;
      button_2_counter++; // Reset counter after valid press
    }
    if(P3_3 == 0 && button_3_counter == 0) {
      led_3_state = !led_3_state;
      button_3_counter++; // Reset counter after valid press
    }

    P2_0 = led_0_state;
    P2_1 = led_1_state;
    P2_2 = led_2_state;
    P2_3 = led_3_state;

    TF0 = 0;	/* Clear Timer 0 overflow flag */
}

