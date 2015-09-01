/*
 * Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __HALT_H__
#define __HALT_H__

#include "halt_exports.h"

typedef enum {
    PERMISSION_DENIED = 1,
    SANITY_CHECK_FAILED,
    NOT_IMPLEMENTED,
    NOT_ALLOWED,
    FAULT_MEMMANAGE,
    FAULT_BUS,
    FAULT_USAGE,
    FAULT_HARD,
    FAULT_DEBUG,
    __THALTERROR_MAX /* always keep as the last element of the enum */
} THaltError;

#define HALT_INTRA_PATTERN_DELAY 0x400000U
#define HALT_INTER_PATTERN_DELAY (4*HALT_INTRA_PATTERN_DELAY)

#ifdef  NDEBUG
#define HALT_ERROR(reason, ...) halt_led(reason)
#else /*NDEBUG*/
#define HALT_ERROR(reason, ...) \
        halt_line(__FILE__, __LINE__, reason, __VA_ARGS__)
#endif/*NDEBUG*/

extern void halt_led(THaltError reason);
extern void halt_user_error(THaltUserError reason);
extern void halt_error(THaltError reason, const char *fmt, ...);
extern void halt_line(const char *file, uint32_t line, THaltError reason,
                      const char *fmt, ...);

#endif/*__HALT_H__*/
