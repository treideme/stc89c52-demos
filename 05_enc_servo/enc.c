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
 * @file lcd.c 7 Segment display interfacing, dynamic switching.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>
#include <mcs51/compiler.h> // NOP
#include <stdint.h>

#define SW  P1_5
#define DT  P1_6
#define CLK P1_7

#define LED P2

void delay(uint16_t t) {
  while (t--) // Simple delay loop (more than 1us at 12MHz)
    ;
}

void main(void) {
  SW = 1; // Enable pull-up
  DT = 1; // Enable pull-up
  CLK = 1; // Enable pull-up

  __bit last_clk = 1;
  uint8_t count = 0;
  for(;;) {
    // Read encoder
    __bit clk = CLK;
    if(last_clk == 1 && clk == 0) { // Falling edge detected
      if(DT == 0) {
        // Clockwise
        if(count < 7) {
          count++;
        } else {
          count = 0;
        }
      } else {
        // Counter-clockwise
        if(count > 0) {
          count--;
        } else {
          count = 7;
        }
      }
    }
    last_clk = clk;

    // Read switch
    if(SW == 0) {
      LED = 0xFF; // Reset on button press
      while(SW == 0); // Wait for release
    } else {
      LED = ~(1 << count); // Display count (active low)
    }
    delay(1000); // Debounce delay
  }
}



