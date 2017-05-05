/*
 * Copyright (c) 2017, ARM Limited, All Rights Reserved
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
#ifndef __SECURE_TRANSITIONS_H__
#define __SECURE_TRANSITIONS_H__

#include "api/inc/uvisor_exports.h"

#define SECURE_TRANSITION_S_TO_NS(handler, ...) \
    ({ \
        UVISOR_MACRO_REGS_ARGS(uint32_t, ##__VA_ARGS__); \
        UVISOR_MACRO_REGS_RETVAL(uint32_t, res); \
        asm volatile( \
            "blxns %[hdlr]\n" \
            : UVISOR_MACRO_GCC_ASM_OUTPUT(res) \
            : [hdlr] "r" (handler), \
              UVISOR_MACRO_GCC_ASM_INPUT(__VA_ARGS__) \
        ); \
        res; \
    })

#endif /* __SECURE_TRANSITIONS_H__ */
