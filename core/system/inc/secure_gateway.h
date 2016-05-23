/*
 * Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
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

#include "context.h"
#include "linker.h"
#include "api/inc/secure_gateway_exports.h"
#include "api/inc/uvisor_exports.h"
#include <stdint.h>

/** Secure gateway structure */
typedef struct {
    uint16_t svc_opcode;        /**< 16 bit SVCall opcode */
    uint16_t branch;            /**< Branch instruction to skip metadata */
    uint32_t magic;             /**< Secure gateway magic value (for verification) */
    uint32_t dst_fn;            /**< Destination handler for the destination box */
    uint32_t dst_box_cfg_ptr;   /**< Pointer to the destination box descriptor */
} UVISOR_PACKED TSecureGateway;

/** Perform the validation of a secure gateway.
 *
 * Once this function is run, a secure gateway structure ::TSecureGateway can be
 * fully trusted and its elements read and used without any further check.
 *
 * @param secure_gateway[in]     Pointer to a supposed secure gateway
 * @returns the same pointer to the secure gateway as provided as input if the
 * verifiaction is successful. */
TSecureGateway * secure_gateway_check(TSecureGateway * secure_gateway);

/** Perform a context switch-in as a result of a secure gateway.
 *
 * This function uses information from an SVCall to retrieve a secure gateway,
 * validate it, and use its metadata to perform the context switch. It should
 * not be called directly by new code, as it is mapped to the hardcoded table of
 * SVCall handlers in uVisor.
 *
 * @warning This function trusts the SVCall parameters that are passed to it.
 *
 * @param svc_sp[in]    Unprivileged stack pointer at the time of the secure
 *                      gateway
 * @param svc_pc[in]    Program counter at the time of the secure gateway
 * @param svc_id[in]    Full ID field of the SVCall opcode */
void secure_gateway_in(uint32_t svc_sp, uint32_t svc_pc, uint8_t svc_id);

/** Perform a context switch-out from a previous secure gateway.
 *
 * This function uses information from an SVCall to return from a previously
 * switched-in secure gateway. It should not be called directly by new code, as
 * it is mapped to the hardcoded table of SVCall handlers in uVisor.
 *
 * @warning This function trusts the SVCall parameters that are passed to it.
 *
 * @param svc_sp[in]    Unprivileged stack pointer at the time of the secure
 *                      gateway return handler (thunk) */
void secure_gateway_out(uint32_t svc_sp);

/** Return the box ID corresponding to a box configuration pointer.
 *
 * @param box_cfg_ptr[in]   Box configuration pointer
 *
 * @warning This function assumes that the input box configuration pointer is
 * within the expected region for configuration pointers. If this pointer is
 * obtained by a secure gateway checked by running the ::secure_gateway_check
 * function, then it can be trusted. */
static inline uint8_t secure_gateway_id_from_cfg_ptr(uint32_t box_cfg_ptr)
{
    return (uint8_t) (((uint32_t *) box_cfg_ptr - (uint32_t *) __uvisor_config.cfgtbl_ptr_start) & 0xFF);
}

#endif /* __SECURE_GATEWAY_H__ */
