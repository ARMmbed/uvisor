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
#ifndef __UVISOR_EXPORTS_H__
#define __UVISOR_EXPORTS_H__

/* maximum number of boxes allowed: 1 is the minimum (unprivileged box) */
#define UVISOR_MAX_BOXES 5U

/* extern keyword */
#ifdef  __cplusplus
#define UVISOR_EXTERN extern "C"
#else
#define UVISOR_EXTERN extern
#endif/*__CPP__*/

/* compiler attributes */
#define UVISOR_FORCEINLINE __attribute__((always_inline))
#define UVISOR_NOINLINE    __attribute__((noinline))
#define UVISOR_PACKED      __attribute__((packed))
#define UVISOR_WEAK        __attribute__((weak))
#define UVISOR_ALIAS(f)    __attribute__((weak, alias (#f)))
#define UVISOR_LINKTO(f)   __attribute__((alias (#f)))
#define UVISOR_NORETURN    __attribute__((noreturn))
#define UVISOR_NAKED       __attribute__((naked))
#define UVISOR_RAMFUNC     __attribute__ ((section (".ramfunc"), noinline))

/* array count macro */
#define UVISOR_ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

/* convert macro argument to string */
/* note: this needs one level of indirection, accomplished with the helper macro
 *       __UVISOR_TO_STRING */
#define __UVISOR_TO_STRING(x) #x
#define UVISOR_TO_STRING(x)   __UVISOR_TO_STRING(x)

/* select an overloaded macro, so that 0 to 4 arguments can be used */
#define __UVISOR_MACRO_SELECT(_0, _1, _2, _3, _4, NAME, ...) NAME

/* count macro arguments */
#define UVISOR_MACRO_NARGS(...) \
     __UVISOR_MACRO_SELECT(_0, ##__VA_ARGS__, 4, 3, 2, 1, 0)

/* declare explicit callee-saved registers to hold input arguments (0 to 4) */
#define UVISOR_MACRO_REGS_ARGS(...) \
     __UVISOR_MACRO_SELECT(_0, ##__VA_ARGS__, __UVISOR_MACRO_REGS_ARGS4, \
                                              __UVISOR_MACRO_REGS_ARGS3, \
                                              __UVISOR_MACRO_REGS_ARGS2, \
                                              __UVISOR_MACRO_REGS_ARGS1, \
                                              __UVISOR_MACRO_REGS_ARGS0)(__VA_ARGS__)
#define __UVISOR_MACRO_REGS_ARGS0()
#define __UVISOR_MACRO_REGS_ARGS1(a0) \
        register uint32_t r0 asm("r0") = (uint32_t) a0;
#define __UVISOR_MACRO_REGS_ARGS2(a0, a1) \
        register uint32_t r0 asm("r0") = (uint32_t) a0; \
        register uint32_t r1 asm("r1") = (uint32_t) a1;
#define __UVISOR_MACRO_REGS_ARGS3(a0, a1, a2) \
        register uint32_t r0 asm("r0") = (uint32_t) a0; \
        register uint32_t r1 asm("r1") = (uint32_t) a1; \
        register uint32_t r2 asm("r2") = (uint32_t) a2;
#define __UVISOR_MACRO_REGS_ARGS4(a0, a1, a2, a3) \
        register uint32_t r0 asm("r0") = (uint32_t) a0; \
        register uint32_t r1 asm("r1") = (uint32_t) a1; \
        register uint32_t r2 asm("r2") = (uint32_t) a2; \
        register uint32_t r3 asm("r3") = (uint32_t) a3;

/* declare explicit callee-saved registers to hold output values */
/* note: currently only one 32bit output value is allowed */
#define UVISOR_MACRO_REGS_RETVAL(name) \
    register uint32_t name asm("r0");

/* declare callee-saved input operands for gcc-style extended inline asm */
/* note: this macro requires that a C variable having the same name of the
 *       corresponding callee-saved register is declared; these operands follow
 *       the official ABI for ARMv7M (e.g. 2 input arguments of 32bits each max,
 *       imply that registers r0 and r1 are used) */
/* note: gcc only */
/* note: for 0 inputs a dummy immediate is passed to avoid errors on a misplaced
 *       comma in the inline assembly */
#ifdef __GNUC__

#define UVISOR_MACRO_GCC_ASM_INPUT(...) \
     __UVISOR_MACRO_SELECT(_0, ##__VA_ARGS__, __UVISOR_MACRO_GCC_ASM_INPUT4, \
                                              __UVISOR_MACRO_GCC_ASM_INPUT3, \
                                              __UVISOR_MACRO_GCC_ASM_INPUT2, \
                                              __UVISOR_MACRO_GCC_ASM_INPUT1, \
                                              __UVISOR_MACRO_GCC_ASM_INPUT0)(__VA_ARGS__)
#define __UVISOR_MACRO_GCC_ASM_INPUT0()               [__dummy] "I" (0)
#define __UVISOR_MACRO_GCC_ASM_INPUT1(a0)             "r" (r0)
#define __UVISOR_MACRO_GCC_ASM_INPUT2(a0, a1)         "r" (r0), "r" (r1)
#define __UVISOR_MACRO_GCC_ASM_INPUT3(a0, a1, a2)     "r" (r0), "r" (r1), "r" (r2)
#define __UVISOR_MACRO_GCC_ASM_INPUT4(a0, a1, a2, a3) "r" (r0), "r" (r1), "r" (r2), "r" (r3)

#endif /* __GNUC__ */

#endif/*__UVISOR_EXPORTS_H__*/
