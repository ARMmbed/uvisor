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
#include "api/inc/rpc2.h"
#include "api/inc/ipc.h"

/* A generic port for all incoming RPC. */
static const size_t UVISOR_RPC_PORT = 'f'; /* 'f' is for function. That's good enough for me. */

static uint32_t wait_for_all(uint32_t wait_tokens)
{
    uint32_t done_tokens = 0;
    static const uint32_t timeout_ms = 0;
    static const uint32_t delay_iterations = 100000;

    /* Spin until all tokens complete. */
    while (ipc_waitforall(wait_tokens, &done_tokens, timeout_ms))
    {
        /* Waste time a bit. */
        /* FIXME: When we can run RTOS in other boxes, or have a uVisor
         * scheduler yield or sleep function, remove this room heater loop. */
        for (volatile int i = 0; i <= delay_iterations; i++);
    }

    return done_tokens;
}

static int do_rpc(int caller_box_id, uvisor_rpc2_call_t * rpc_msg)
{
    int status;
    uint32_t done_tokens;
    uvisor_rpc2_return_t rpc_ret;
    uvisor_ipc_desc_t desc;

    /* Do the RPC. */
    rpc_ret.ret = rpc_msg->fnptr(rpc_msg->p0, rpc_msg->p1, rpc_msg->p2, rpc_msg->p3,
                                 caller_box_id);

    /* Send the RPC response to the port the caller box expects. */
    desc.box_id = caller_box_id;
    desc.port = rpc_msg->completion_port;
    desc.len = sizeof(rpc_ret);

    status = ipc_send(&desc, &rpc_ret);
    if (status) {
        return status;
    }

    /* Wait for the send to complete, to keep the IPC descriptor and RPC
     * response in memory long enough for uVisor to read them (before they go
     * out of scope). */
    done_tokens = wait_for_all(desc.token);
    if (!(done_tokens & desc.token)) {
        /* Token we wanted didn't complete */
        return -1;
    }

    return 0;
}

static int handle_rpc(int caller_box_id, uvisor_rpc2_call_t * rpc_msg, const rpc2_fnptr_t fn_array[], size_t fn_count)
{
    size_t i;
    /* Check if the target function is in the fn_array */
    for (i = 0; i < fn_count; ++i) {
        if (fn_array[i] == rpc_msg->fnptr) {
            return do_rpc(caller_box_id, rpc_msg);
        }
    }

    /* The RPC target was not in the list of acceptable RPC targets. */
    return -1;
}

int rpc_waitfor(const rpc2_fnptr_t fn_array[], size_t fn_count, int box_id, uint32_t timeout_ms)
{
    int status;
    uvisor_ipc_desc_t desc = {0};
    uvisor_rpc2_call_t rpc_msg;

    /* Receive the RPC response over the generic RPC port. */
    desc.box_id = box_id;
    desc.port = UVISOR_RPC_PORT;
    desc.len = sizeof(rpc_msg);

    status = ipc_recv(&desc, &rpc_msg);
    if (status) {
        return status;
    }

    /* Wait for the receive to complete. */
    if (timeout_ms == 0) {
        /* Try once. */
        uint32_t done_tokens = 0;
        status = ipc_waitforall(desc.token, &done_tokens, timeout_ms);
        if (status) {
            return status;
        }

        /* See if done_tokens matches */
        if (done_tokens != desc.token) {
            /* Not good. No match. Fail. */
            return -1;
        }
    } else {
        /* Spin forever */
        wait_for_all(desc.token);
    }

    /* Handle the RPC */
    handle_rpc(desc.box_id, &rpc_msg, fn_array, fn_count);

    return 0;
}

int rpc_async(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3,
              const rpc2_fnptr_t fnptr, int box_id, uvisor_rpc2_cookie_t *cookie)
{
    int status;
    uint32_t done_tokens;
    size_t completion_port;
    uvisor_rpc2_call_t rpc_msg = {0};
    uvisor_ipc_desc_t desc = {0};

    /* Allocate a return port to receive a response on. */
    completion_port = 0; /* XXX TODO */

    /* Build the RPC message. */
    rpc_msg.p0 = p0;
    rpc_msg.p1 = p1;
    rpc_msg.p2 = p2;
    rpc_msg.p3 = p3;
    rpc_msg.fnptr = fnptr;
    rpc_msg.completion_port = completion_port;

    /* Send the message to the generic RPC port. */
    desc.box_id = box_id;
    desc.port = UVISOR_RPC_PORT;
    desc.len = sizeof(rpc_msg);

    status = ipc_send(&desc, &rpc_msg);
    if (status) {
        return status;
    }

    /* Wait for the send to complete, to keep the IPC descriptor and RPC
     * message in memory long enough for uVisor to read them (before they go
     * out of scope). */
    done_tokens = wait_for_all(desc.token);
    if (!(done_tokens & desc.token)) {
        /* Token we wanted didn't complete */
        return -1;
    }

    /* The message has been sent. Update the cookie with the port we should wait on. */
    cookie->completion_port = completion_port;
    cookie->box_id = box_id;

    return 0;
}

int rpc_wait(uvisor_rpc2_cookie_t cookie, uint32_t timeout_ms, uint32_t * ret)
{
    int status;
    uvisor_ipc_desc_t desc = {0};
    uvisor_rpc2_return_t rpc_ret = {0};

    /* Receive the RPC response over the negotiated port for the call. */
    desc.box_id = cookie.box_id;
    desc.port = cookie.completion_port;
    desc.len = sizeof(rpc_ret);

    status = ipc_recv(&desc, &rpc_ret);
    if (status) {
        return status;
    }

    /* Wait for the receive to complete. */
    if (timeout_ms == 0) {
        /* Try once. */
        uint32_t done_tokens = 0;
        status = ipc_waitforall(desc.token, &done_tokens, timeout_ms);
        if (status) {
            return status;
        }

        /* See if done_tokens matches */
        if (done_tokens != desc.token) {
            /* Not good. No match. Fail. */
            return -1;
        }
    } else {
        /* Spin forever */
        wait_for_all(desc.token);
    }

    /* If ret is provided, update ret with the RPC return value */
    if (ret) {
        *ret = rpc_ret.ret;
    }

    return 0;
}

int rpc_gateway_async(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3,
                      const rpc2_gateway_t * gateway, uvisor_rpc2_cookie_t * cookie)
{
    /* TODO */
    return 0;
}
