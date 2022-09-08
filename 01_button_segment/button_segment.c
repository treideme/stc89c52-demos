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
 * @file button_segment.c Simple click-button counter using 7-segment displays.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>

static void delay(unsigned int t);

void main() {
  for(;;) {
    // Toggle LED at P2_0
    P2_0 = 0;
    delay(30000);
    P2_0 = 1;
    delay(30000);
  }
}

static void delay(unsigned int t) {
  while (t--)
    ;
}

