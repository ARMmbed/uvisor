/* General C++ Object Thunking class
 *
 * - allows direct callbacks to non-static C++ class functions
 * - keeps track for the corresponding class instance
 * - supports an optional context parameter for the called function
 * - ideally suited for class object receiving interrupts (NVIC_SetVector)
 *
 * Copyright (c) 2014 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdint.h>
#include "CThunk.h"

#ifdef __thumb__
const uint8_t g_cthunk_opcodes[CTHUNK_OPCODES_SIZE] = {
    /* LDM PCz!, {r0, r1, pc}             */
    0x9F, 0xE8, 0x03, 0x80,
};
#endif
