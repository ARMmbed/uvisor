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
#include <uvisor.h>
#include "halt.h"
#include "secure_gateway.h"
#include "vmpu.h"

/** Mask to verifiy that an opcode is a secure gateway SVCall
 *
 * @internal
 *
 * The mask is made of the basic SVCall opcode plus the secure gateway SVC ID,
 * but excluding the number of arguments. */
#define SECURE_GATEWAY_SVC_OPCODE_MASK (uint16_t) (0xDF00 + UVISOR_SVC_ID_SECURE_GATEWAY(0))

/** Perform the validation of a secure gateway.
 *
 * @internal */
TSecureGateway * secure_gateway_check(TSecureGateway * secure_gateway)
{
    int i, count;

    /* Verify that the secure gateway is in public flash. */
    /* Note: We assume here that TSecureGateway is a multiple of 32 bit. If that
     * is not true there might be false negatives with an unaligned secure
     * gateway. */
    count = 0;
    for (i = 0; i < sizeof(TSecureGateway) / sizeof(uint32_t); i++) {
        if (!vmpu_public_flash_addr(((uint32_t) secure_gateway) + sizeof(uint32_t) * i)) {
            count++;
        }
    }
    if (count) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Secure gateway 0x%08X is not in public flash (%d words)",
                   (uint32_t) secure_gateway, count);
    }

    /* Verify that the secure gateway opcode is valid. For this check we mask
     * the nargs field away. */
    if ((secure_gateway->svc_opcode & ~((uint16_t) UVISOR_SVC_FAST_NARGS_MASK)) != SECURE_GATEWAY_SVC_OPCODE_MASK) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Secure gateway 0x%08X does not contain a valid SVC opcode (0x%04X)",
                   (uint32_t) secure_gateway, secure_gateway->svc_opcode);
    }

    /* Verify that the secure gateway magic is present. */
    if (secure_gateway->magic != UVISOR_SECURE_GATEWAY_MAGIC) {
        HALT_ERROR(SANITY_CHECK_FAILED, "Secure gateway 0x%08X does not contain a valid magic (0x%08X)",
                   (uint32_t) secure_gateway, secure_gateway->magic);
    }

    /* Verify that the destination function is in public flash. */
    if (!vmpu_public_flash_addr(secure_gateway->dst_fn)) {
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "The destination function (0x%08X) of secure gateway 0x%08X is not in public flash",
                   secure_gateway->dst_fn, (uint32_t) secure_gateway);
    }

    /* Verify that the destination box configuration pointer is in the dedicated
     * flash region. */
    if (secure_gateway->dst_box_cfg_ptr < (uint32_t) __uvisor_config.cfgtbl_ptr_start ||
        secure_gateway->dst_box_cfg_ptr >= (uint32_t) __uvisor_config.cfgtbl_ptr_end) {
        HALT_ERROR(SANITY_CHECK_FAILED,
                   "The pointer (0x%08X) in the secure gateway 0x%08X is not a valid box configuration pointer",
                   secure_gateway->dst_box_cfg_ptr, (uint32_t) secure_gateway);
    }

    /* If none of the checks above failed, then the input pointer is a valid
     * secure gateway. */
    return secure_gateway;
}

/** Thunk function for the secure gateway
 *
 * @internal
 *
 * This function is used to return from a context switch triggered by a secure
 * gateway. It is never called directly, but its address is inserted in the
 * exception stack frame of the destination box (as the lr value) to trigger the
 * return from the context switch automatically. */
static void UVISOR_NAKED secure_gateway_thunk(void)
{
    UVISOR_SVC(UVISOR_SVC_ID_CX_OUT, "");
}

/** Perform a context switch-in as a result of a secure gateway.
 *
 * @internal
 *
 * This function is implemented as a wrapper, needed to make sure that the lr
 * register doesn't get polluted and to provide context privacy during a context
 * switch. The actual function is ::secure_gateway_context_switch_in. */
void UVISOR_NAKED secure_gateway_in(uint32_t svc_sp, uint32_t svc_pc, uint8_t svc_id)
{
    /* According to the ARM ABI, r0, r1, and r2 will have the following values
     * when this function is called:
     *   r0 = svc_sp
     *   r1 = svc_pc
     *   r2 = svc_id */
    asm volatile(
        "push {r4 - r11}\n"                             /* Store the callee-saved registers on the MSP (privileged). */
        "push {lr}\n"                                   /* Preserve the lr register. */
        "bl   secure_gateway_context_switch_in\n"       /* privacy = secure_gateway_context_switch_in(svc_sp, svc_pc, svc_id) */
        "cmp  r0, #0\n"                                 /* if (privacy)  */
        "beq  secure_gateway_no_regs_clearing\n"        /* {             */
        "mov  r4,  #0\n"                                /*     Clear r4  */
        "mov  r5,  #0\n"                                /*     Clear r5  */
        "mov  r6,  #0\n"                                /*     Clear r6  */
        "mov  r7,  #0\n"                                /*     Clear r7  */
        "mov  r8,  #0\n"                                /*     Clear r8  */
        "mov  r9,  #0\n"                                /*     Clear r9  */
        "mov  r10, #0\n"                                /*     Clear r10 */
        "mov  r11, #0\n"                                /*     Clear r11 */
        "secure_gateway_no_regs_clearing:\n"            /* } else { ; }  */
        "pop  {pc}\n"                                   /* Return. Note: Callee-saved registers are not popped here. */
                                                        /* The destination function will be executed after this. */
        :: "r" (svc_sp), "r" (svc_pc), "r" (svc_id)
    );
}

/** Perform a context switch-in as a result of a secure gateway.
 *
 * @internal
 *
 * This function implements ::secure_gateway_in, which is instead only a
 * wrapper.
 *
 * @warning This function trusts the SVCall parameters that are passed to it.
 *
 * @param svc_sp[in]    Unprivileged stack pointer at the time of the secure
 *                      gateway
 * @param svc_pc[in]    Program counter at the time of the secure gateway
 * @param svc_id[in]    Full ID field of the SVCall opcode */
bool secure_gateway_context_switch_in(uint32_t src_svc_sp, uint32_t src_svc_pc, uint8_t src_svc_id)
{
    int nargs;
    uint8_t dst_id;
    uint32_t dst_fn;
    uint32_t src_sp, dst_sp;
    uint32_t xpsr;
    TSecureGateway * secure_gateway;

    nargs = (int) UVISOR_SVC_FAST_NARGS_GET(src_svc_id);
    if (nargs > CONTEXT_SWITCH_FUNCTION_MAX_NARGS) {
        HALT_ERROR(NOT_ALLOWED, "Max 4 32 bit arguments allowed");
    }

    /* The stack pointer provided to us as an input comes from an SVC exception.
     * We must check that it corresponds to a full frame that the source box can
     * access. */
    /* This function halts if it finds an error. */
    src_sp = context_validate_exc_sf(src_svc_sp);

    /* Check that the SVC came from within a secure gateway. */
    /* This function halts if it finds an error. */
    secure_gateway = secure_gateway_check((TSecureGateway *) src_svc_pc);

    /* Destination box: Gather information from the secure gateway. */
    dst_id = secure_gateway_id_from_cfg_ptr(secure_gateway->dst_box_cfg_ptr);
    dst_fn = secure_gateway->dst_fn;

    /* Currently we do not support secure gateways to oneself. Secure gateways
     * to box 0 are also not allowed. */
    if (!dst_id) {
        HALT_ERROR(NOT_ALLOWED, "A secure gateway to box 0 is not allowed.");
    }
    if (g_active_box == dst_id) {
        HALT_ERROR(NOT_ALLOWED, "The destination (%d) of a secure gateway must be different from its source (%d).",
                                dst_id, g_active_box);
    }

    /* Forge a stack frame for the destination box and transfer nargs arguments
     * from the source stack. */
    xpsr = ((uint32_t *) src_sp)[7];
    dst_sp = context_forge_exc_sf(src_sp, dst_id, dst_fn, (uint32_t) secure_gateway_thunk, xpsr, nargs);

    /* Perform the context switch-in to the destination box. */
    /* This function halts if it finds an error. */
    context_switch_in(CONTEXT_SWITCH_FUNCTION_GATEWAY, dst_id, src_sp, dst_sp);

    /* Return whether the destination box requires privacy or not. */
    /* TODO: Context privacy is currently unsupported. */
    return false;
}

/** Perform a context switch-out as a result of a secure gateway.
 *
 * @internal
 *
 * This function is implemented as a wrapper, needed to make sure that the lr
 * register doesn't get polluted and to provide context privacy during a context
 * switch. The actual function is ::secure_gateway_context_switch_out. */
void UVISOR_NAKED secure_gateway_out(uint32_t svc_sp)
{
    /* According to the ARM ABI, r0 will have the following value when this
     * function is called:
     *   r0 = svc_sp */
    asm volatile(
        "push {lr}\n"                                   /* Preserve the lr register. */
        "bl   secure_gateway_context_switch_out\n"      /* secure_gateway_context_switch_out(svc_sp) */
        "pop  {lr}\n"                                   /* Restore the lr register. */
        "pop  {r4-r11}\n"                               /* Restore the callee-saved registers saved at switch-in time. */
        "bx   lr\n"                                     /* Return. */
        :: "r" (svc_sp)
    );
}

/** Perform a context switch-out from a previous secure gateway.
 *
 * @internal
 *
 * This function implements ::secure_gateway_out, which is instead only a
 * wrapper.
 *
 * @warning This function trusts the SVCall parameters that are passed to it.
 *
 * @param svc_sp[in]    Unprivileged stack pointer at the time of the secure
 *                      gateway return handler (thunk) */
void secure_gateway_context_switch_out(uint32_t svc_sp)
{
    uint32_t dst_sp;
    TContextPreviousState * previous_state;

    /* Discard the unneeded exception stack frame from the destination box
     * stack. The destination box is the currently active one. */
    dst_sp = context_validate_exc_sf(svc_sp);
    context_discard_exc_sf(g_active_box, dst_sp);

    /* Perform the context switch back to the previous state.
     * Upon return, the previous state structure will be populated. */
    previous_state = context_switch_out(CONTEXT_SWITCH_FUNCTION_GATEWAY);

    /* Copy the return value from the destination stack frame.
     * Note: We trust the source box stack pointer as stored in the previous
     *       state.*/
    ((uint32_t *) previous_state->src_sp)[0] = ((uint32_t *) dst_sp)[0];
}
