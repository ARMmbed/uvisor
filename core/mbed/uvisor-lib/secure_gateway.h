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
#ifndef __SECURE_GATEWAY_H__
#define __SECURE_GATEWAY_H__

/* this macro selects an overloaded macro (variable number of arguments) */
#define SELECT_MACRO(_0, _1, _2, _3, _4, NAME, ...) NAME

/* used to count macro arguments */
#define NARGS(...)                                                             \
     SELECT_MACRO(_0, ##__VA_ARGS__, 4, 3, 2, 1, 0)

/* used to declare register values to hold the variable number of arguments */
#define SELECT_ARGS(...)                                                       \
     SELECT_MACRO(_0, ##__VA_ARGS__, SELECT_ARGS4,                             \
                                     SELECT_ARGS3,                             \
                                     SELECT_ARGS2,                             \
                                     SELECT_ARGS1,                             \
                                     SELECT_ARGS0)(__VA_ARGS__)                \

#define SELECT_ARGS0()
#define SELECT_ARGS1(a0)                                                       \
        register uint32_t r0 asm("r0") = (uint32_t) a0;
#define SELECT_ARGS2(a0, a1)                                                   \
        register uint32_t r0 asm("r0") = (uint32_t) a0;                        \
        register uint32_t r1 asm("r1") = (uint32_t) a1;
#define SELECT_ARGS3(a0, a1, a2)                                               \
        register uint32_t r0 asm("r0") = (uint32_t) a0;                        \
        register uint32_t r1 asm("r1") = (uint32_t) a1;                        \
        register uint32_t r2 asm("r2") = (uint32_t) a2;
#define SELECT_ARGS4(a0, a1, a2, a3)                                           \
        register uint32_t r0 asm("r0") = (uint32_t) a0;                        \
        register uint32_t r1 asm("r1") = (uint32_t) a1;                        \
        register uint32_t r2 asm("r2") = (uint32_t) a2;                        \
        register uint32_t r3 asm("r3") = (uint32_t) a3;

/* used to declare registers in the asm volatile to avoid compiler opt */
#define SELECT_REGS(...) \
     SELECT_MACRO(_0, ##__VA_ARGS__, SELECT_REGS4,                             \
                                     SELECT_REGS3,                             \
                                     SELECT_REGS2,                             \
                                     SELECT_REGS1,                             \
                                     SELECT_REGS0)(__VA_ARGS__)                \

#define SELECT_REGS0()
#define SELECT_REGS1(a0)             , "r" (r0)
#define SELECT_REGS2(a0, a1)         , "r" (r0), "r" (r1)
#define SELECT_REGS3(a0, a1, a2)     , "r" (r0), "r" (r1), "r" (r2)
#define SELECT_REGS4(a0, a1, a2, a3) , "r" (r0), "r" (r1), "r" (r2), "r" (r3)

/* the actual secure gateway */
#define secure_gateway(dst_box, dst_fn, ...)                                   \
    ({                                                                         \
        SELECT_ARGS(__VA_ARGS__)                                               \
        register uint32_t res asm("r0");                                       \
        asm volatile(                                                          \
            "svc   %[svc_id]\n"                                                \
            "b.n   skip_args%=\n"                                              \
            ".word "UVISOR_TO_STRING(UVISOR_SVC_GW_MAGIC)"\n"                  \
            ".word "UVISOR_TO_STRING(dst_fn)"\n"                               \
            ".word "UVISOR_TO_STRING(dst_box)"_cfg_ptr\n"                      \
            "skip_args%=:\n"                                                   \
            :          "=r" (res)                                              \
            : [svc_id] "I"  (UVISOR_SVC_ID_SECURE_GATEWAY(NARGS(__VA_ARGS__))) \
                       SELECT_REGS(__VA_ARGS__)                                \
        );                                                                     \
        res;                                                                   \
     })

#endif/*__SECURE_GATEWAY_H__*/
