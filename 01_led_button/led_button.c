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
 * @file led_button.c Simple LED button mirroring.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>

void main(void) {
  for(;;) {
    // Toggle LED at P2_0..P2_3 when buttons at P3_0..P3_3 are pressed
    P2_0 = P3_1;
    P2_1 = P3_0;
    P2_2 = P3_2;
    P2_3 = P3_3;
  }
}