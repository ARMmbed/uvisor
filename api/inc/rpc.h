/*
 * Copyright (c) 2016, ARM Limited, All Rights Reserved
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
#ifndef __UVISOR_API_RPC_H__
#define __UVISOR_API_RPC_H__

#include "api/inc/uvisor_exports.h"
#include <stdint.h>

/** Specify the maximum number of incoming RPC messages for a box
 *
 * @param max_num_incoming_rpc The maximum number of incoming RPC messages for
 *                             a box
 */
/* XXX This is a dummy implementation. */
#define UVISOR_BOX_RPC_MAXIMUM_INCOMING_RPC(max_num_incoming_rpc)

/* This is the token to wait on for the result of an asynchronous RPC. */
typedef uint32_t uvisor_rpc_result_t;

typedef uint32_t (*TFN_Ptr)(uint32_t, uint32_t, uint32_t, uint32_t);
typedef int (*TFN_RPC_Callback)(int);

/* This synchronous function is easy-mode. It gets you talkin' with the other
 * box, but not in a mutually distrustful fashion: for instance, you have to
 * trust that the target box will eventually return. */
UVISOR_EXTERN uint32_t rpc_fncall(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, const TFN_Ptr fn);

/* Start an asynchronous RPC. After this call, the caller can, at any time in
 * any thread, wait on the result token to get the result of the call. */
UVISOR_EXTERN int rpc_fncall_async(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, const TFN_Ptr fn,
                                   uvisor_rpc_result_t result);

/** Wait for incoming RPC.
 *
 * @param fn_ptr_array an array of RPC function targets that this call to
 *                     `rpc_fncall_waitfor` should handle RPC to
 * @param fn_count     the number of function targets in this array
 * @param timeout_ms   specifies how long to wait (in ms) for an incoming RPC
 *                     message before returning
 */
int rpc_fncall_waitfor(const TFN_Ptr fn_ptr_array[], size_t fn_count, uint32_t timeout_ms);

#endif /* __UVISOR_API_RPC_H__ */
