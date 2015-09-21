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
#ifndef __SVC_EXPORTS_H__
#define __SVC_EXPORTS_H__

/* maximum depth of nested context switches
 * this includes both IRQn and secure gateways, as they use the same state stack
 * for their context switches */
#define UVISOR_SVC_CONTEXT_MAX_DEPTH 0x10

/* SVC takes a 8bit immediate, which is split as follows:
 *    bit 7 to bit UVISOR_SVC_CUSTOM_BITS: hardcoded table
 *    bit UVISOR_SVC_CUSTOM_BITS - 1 to bit 0: custom table
 * in addition, if the hardcoded table is used, the lower bits
 *    bit UVISOR_SVC_CUSTOM_BITS - 1 to bit 0
 * are used to pass the number of arguments of the SVC call (up to 4) */
#define UVISOR_SVC_CUSTOM_BITS     4
#define UVISOR_SVC_FIXED_BITS      (8 - UVISOR_SVC_CUSTOM_BITS)
#define UVISOR_SVC_CUSTOM_MSK      ((uint8_t) (0xFF >> UVISOR_SVC_FIXED_BITS))
#define UVISOR_SVC_FIXED_MSK       ((uint8_t) ~UVISOR_SVC_CUSTOM_MSK)
#define UVISOR_SVC_CUSTOM_TABLE(x) ((uint8_t)  (x) &  UVISOR_SVC_CUSTOM_MSK)
#define UVISOR_SVC_FIXED_TABLE(x)  ((uint8_t) ((x) << UVISOR_SVC_CUSTOM_BITS)  \
                                                   &  UVISOR_SVC_FIXED_MSK)

/* SVC immediate values for custom table */
#define UVISOR_SVC_ID_ISR_SET        UVISOR_SVC_CUSTOM_TABLE(0)
#define UVISOR_SVC_ID_ISR_GET        UVISOR_SVC_CUSTOM_TABLE(1)
#define UVISOR_SVC_ID_IRQ_ENABLE     UVISOR_SVC_CUSTOM_TABLE(2)
#define UVISOR_SVC_ID_IRQ_DISABLE    UVISOR_SVC_CUSTOM_TABLE(3)
#define UVISOR_SVC_ID_IRQ_PEND_CLR   UVISOR_SVC_CUSTOM_TABLE(4)
#define UVISOR_SVC_ID_IRQ_PEND_SET   UVISOR_SVC_CUSTOM_TABLE(5)
#define UVISOR_SVC_ID_IRQ_PEND_GET   UVISOR_SVC_CUSTOM_TABLE(6)
#define UVISOR_SVC_ID_IRQ_PRIO_SET   UVISOR_SVC_CUSTOM_TABLE(7)
#define UVISOR_SVC_ID_IRQ_PRIO_GET   UVISOR_SVC_CUSTOM_TABLE(8)
#define UVISOR_SVC_ID_BENCHMARK_CFG  UVISOR_SVC_CUSTOM_TABLE(9)
#define UVISOR_SVC_ID_BENCHMARK_RST  UVISOR_SVC_CUSTOM_TABLE(10)
#define UVISOR_SVC_ID_BENCHMARK_STOP UVISOR_SVC_CUSTOM_TABLE(11)
#define UVISOR_SVC_ID_HALT_USER_ERR  UVISOR_SVC_CUSTOM_TABLE(12)

/* SVC immediate values for hardcoded table (call from unprivileged) */
#define UVISOR_SVC_ID_UNVIC_OUT      UVISOR_SVC_FIXED_TABLE(1)
#define UVISOR_SVC_ID_CX_IN          UVISOR_SVC_FIXED_TABLE(2)
#define UVISOR_SVC_ID_CX_OUT         UVISOR_SVC_FIXED_TABLE(3)

/* SVC immediate values for hardcoded table (call from privileged) */
#define UVISOR_SVC_ID_UNVIC_IN       UVISOR_SVC_FIXED_TABLE(1)

/* unprivileged code uses a secure gateway to switch context */
#define UVISOR_SVC_ID_SECURE_GATEWAY(args) ((UVISOR_SVC_ID_CX_IN) | (UVISOR_SVC_CUSTOM_TABLE(args)))

/* macro to execute an SVCall; additional metadata can be provided, which will
 * be appended right after the svc instruction */
/* note: the macro is implicitly overloaded to allow 0 to 4 32bits arguments */
#if defined(__CC_ARM)

#elif defined(__GNUC__)

#define UVISOR_SVC(id, metadata, ...) \
    ({ \
        UVISOR_MACRO_REGS_ARGS(__VA_ARGS__); \
        UVISOR_MACRO_REGS_RETVAL(res); \
        asm volatile( \
            "svc %[svc_id]\n" \
            metadata \
            :          "=r" (res) \
            : [svc_id] "I"  (id), \
              UVISOR_MACRO_GCC_ASM_INPUT(__VA_ARGS__) \
        ); \
        res; \
    })

#endif /* defined(__CC_ARM) || defined(__GNUC__) */

#endif/*__SVC_EXPORTS_H__*/
