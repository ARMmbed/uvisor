/*
 * Copyright (c) 2015-2015, ARM Limited, All Rights Reserved
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
#ifndef __UVISOR_LIB_REGISTER_GATEWAY_H__
#define __UVISOR_LIB_REGISTER_GATEWAY_H__

/* register gateway operations */
/* note: do not use special characters as these numbers will be stringified */
#define UVISOR_OP_READ(op)  (op)
#define UVISOR_OP_WRITE(op) ((1 << 4) | (op))
#define UVISOR_OP_NOP       0x0
#define UVISOR_OP_AND       0x1
#define UVISOR_OP_OR        0x2
#define UVISOR_OP_XOR       0x3

/* default mask for whole register operatins */
#define __UVISOR_OP_DEFAULT_MASK 0x0

/* register gateway metadata */
#if defined(__CC_ARM)

/* TODO/FIXME */

#elif defined(__GNUC__)

/* 1 argument: simple read, no mask */
#define __UVISOR_REGISTER_GATEWAY_METADATA1(src_box, addr) \
    "b.n skip_args%=\n" \
    ".word " UVISOR_TO_STRING(UVISOR_SVC_GW_MAGIC) "\n" \
    ".word " UVISOR_TO_STRING(addr) "\n" \
    ".word " UVISOR_TO_STRING(src_box) "_cfg_ptr\n" \
    ".word " UVISOR_TO_STRING(UVISOR_OP_READ(UVISOR_OP_NOP)) "\n" \
    ".word " UVISOR_TO_STRING(__UVISOR_OP_DEFAULT_MASK) "\n" \
    "skip_args%=:\n"

/* 2 arguments: simple write, no mask */
#define __UVISOR_REGISTER_GATEWAY_METADATA2(src_box, addr, val) \
    "b.n skip_args%=\n" \
    ".word " UVISOR_TO_STRING(UVISOR_SVC_GW_MAGIC) "\n" \
    ".word " UVISOR_TO_STRING(addr) "\n" \
    ".word " UVISOR_TO_STRING(src_box) "_cfg_ptr\n" \
    ".word " UVISOR_TO_STRING(UVISOR_OP_WRITE(UVISOR_OP_NOP)) "\n" \
    ".word " UVISOR_TO_STRING(__UVISOR_OP_DEFAULT_MASK) "\n" \
    "skip_args%=:\n"

/* 3 arguments: masked read */
#define __UVISOR_REGISTER_GATEWAY_METADATA3(src_box, addr, op, mask) \
    "b.n skip_args%=\n" \
    ".word " UVISOR_TO_STRING(UVISOR_SVC_GW_MAGIC) "\n" \
    ".word " UVISOR_TO_STRING(addr) "\n" \
    ".word " UVISOR_TO_STRING(src_box) "_cfg_ptr\n" \
    ".word " UVISOR_TO_STRING(UVISOR_OP_READ(op)) "\n" \
    ".word " UVISOR_TO_STRING(mask) "\n" \
    "skip_args%=:\n"

/* 4 arguments: masked write */
#define __UVISOR_REGISTER_GATEWAY_METADATA4(src_box, addr, val, op, mask) \
    "b.n skip_args%=\n" \
    ".word " UVISOR_TO_STRING(UVISOR_SVC_GW_MAGIC) "\n" \
    ".word " UVISOR_TO_STRING(addr) "\n" \
    ".word " UVISOR_TO_STRING(src_box) "_cfg_ptr\n" \
    ".word " UVISOR_TO_STRING(UVISOR_OP_WRITE(op)) "\n" \
    ".word " UVISOR_TO_STRING(mask) "\n" \
    "skip_args%=:\n"

#endif /* __CC_ARM or __GNUC__ */

#define __UVISOR_REGISTER_GATEWAY_METADATA(src_box, ...) \
     __UVISOR_MACRO_SELECT(_0, ##__VA_ARGS__, __UVISOR_REGISTER_GATEWAY_METADATA4, \
                                              __UVISOR_REGISTER_GATEWAY_METADATA3, \
                                              __UVISOR_REGISTER_GATEWAY_METADATA2, \
                                              __UVISOR_REGISTER_GATEWAY_METADATA1, \
                                              /* no macro for 0 args */          )(src_box, ##__VA_ARGS__)

/* register-level gateway - read */
/* FIXME currently only a hardcoded 32bit constant can be used for the addr field */
#define uvisor_read(src_box, ...) \
    ({ \
        uint32_t res = UVISOR_SVC(UVISOR_SVC_ID_REGISTER_GATEWAY, \
                                  __UVISOR_REGISTER_GATEWAY_METADATA(src_box, ##__VA_ARGS__)); \
        res; \
    })

/* register-level gateway - write */
#define uvisor_write(src_box, addr, val, ...) \
    ({ \
        UVISOR_SVC(UVISOR_SVC_ID_REGISTER_GATEWAY, \
                   __UVISOR_REGISTER_GATEWAY_METADATA(src_box, addr, val, ##__VA_ARGS__), \
                   val); \
    })

#endif /* __UVISOR_LIB_REGISTER_GATEWAY_H__ */
