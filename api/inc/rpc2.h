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
#ifndef __UVISOR_API_RPC2_H__
#define __UVISOR_API_RPC2_H__

#include "api/inc/uvisor_exports.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t magic;
} rpc2_gateway_t;

typedef int (* rpc2_fnptr_t)(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3,
                             int box_id_caller);

/** RPC Call Descriptor
 *
 * This is the descriptor sent by RPC senders and handled by RPC receivers.
 *
 * @param[in]  p0               first parameter
 * @param[in]  p1               second parameter
 * @param[in]  p2               third parameter
 * @param[in]  p3               fourth parameter
 * @param[in]  fnptr            the function to call
 * @param[in]  completion_port  the port number to wait for completion on
 */
typedef struct uvisor_rpc2_call {
    uint32_t p0;
    uint32_t p1;
    uint32_t p2;
    uint32_t p3;
    rpc2_fnptr_t fnptr;
    size_t completion_port;
} uvisor_rpc2_call_t;

/** RPC Return Descriptor
 *
 * This is the descriptor sent by RPC receivers and handled by RPC senders.
 *
 * @param[in]  ret    the return value from the call
 */
typedef struct uvisor_rpc2_return {
    uint32_t ret;
} uvisor_rpc2_return_t;

/** RPC Completion Cookie
 *
 * This is used to wait for an RPC to complete.
 *
 * @param[in]  completion_port  the port number to wait for completion on
 * @param[in]  box_id       the id of the box we expect completion from
 */
typedef struct uvisor_rpc2_cookie {
    size_t completion_port;
    int box_id;
} uvisor_rpc2_cookie_t;

/** Wait for incoming RPC.
 *
 * @param fn_ptr_array       an array of RPC function targets that this call to
 *                           `rpc_fncall_waitfor` should handle RPC to
 * @param fn_count           the number of function targets in this array
 * @param box_id             an ID of a box that is allowed to send to this box, or
 *                           UVISOR_BOX_ID_ANY to allow messages from any box
 * @param timeout_ms         specifies how long to wait (in ms) for an incoming
 *                           RPC message before returning
 */
UVISOR_EXTERN int rpc_waitfor(const rpc2_fnptr_t fn_array[], size_t fn_count, int box_id, uint32_t timeout_ms);

/** Start an asynchronous RPC.
 *
 * After this call successfully completes, the caller can, at any time in any
 * thread, wait on the cookie to get the result of the call.
 * @param[in]  p0      first parameter
 * @param[in]  p1      second parameter
 * @param[in]  p2      third parameter
 * @param[in]  p3      fourth parameter
 * @param[in]  fnptr   the function to call
 * @param[in]  box_id  the ID of the box to call the function within
 * @param[out] cookie  wait on this cookie to get the result of the call
 * @returns            non-zero on error, zero on success
 */
UVISOR_EXTERN int rpc_async(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3,
                            const rpc2_fnptr_t fnptr, int box_id, uvisor_rpc2_cookie_t * cookie);

/** Wait for an outgoing RPC to finish.
 *
 * Wait for the result of a previously started asynchronous RPC. After this
 * call, ret will contain the return value of the RPC. The return value of this
 * function may indicate that there was an error or a timeout with non-zero.
 *
 * @param cookie[in]     The cookie to wait on for the result of an asynchronous RPC
 * @param timeout_ms[in] How long to wait (in ms) for the asynchronous RPC
 *                       message to finish before returning
 * @param ret[out]       The return value resulting from the finished RPC to
 *                       the target function. Use NULL when don't care.
 * @returns              Non-zero on error or timeout, zero on successful wait
 */
UVISOR_EXTERN int rpc_wait(uvisor_rpc2_cookie_t cookie, uint32_t timeout_ms, uint32_t * ret);

/** Start an asynchronous RPC with a gateway.
 *
 * After this call successfully completes, the caller can, at any time in any
 * thread, wait on the cookie to get the result of the call.
 * @param[in]  p0       first parameter
 * @param[in]  p1       second parameter
 * @param[in]  p2       third parameter
 * @param[in]  p3       fourth parameter
 * @param[in]  gateway  the address of the RPC gateway
 * @param[out] cookie   wait on this cookie to get the result of the call
 * @returns             non-zero on error, zero on success
 */
UVISOR_EXTERN int rpc_gateway_async(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3,
                                    const rpc2_gateway_t * gateway, uvisor_rpc2_cookie_t * cookie);

#endif
