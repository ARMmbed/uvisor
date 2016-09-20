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
#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "api/inc/context_exports.h"
#include "api/inc/uvisor_exports.h"
#include <stdint.h>

/** Exception stack frame size
 *
 * @warning Currently only the FPU-disabled case is supported. Exceptions that
 * generates when the FPU is enabled will escalate to a hard fault. */
#define CONTEXT_SWITCH_EXC_SF_WORDS 8
#define CONTEXT_SWITCH_EXC_SF_BYTES (CONTEXT_SWITCH_EXC_SF_WORDS * sizeof(uint32_t))

/** Maximum number of arguments that can be passed to a function-bound context
 * switch. */
#define CONTEXT_SWITCH_FUNCTION_MAX_NARGS 4

/** Types of context switches
 *
 * There are two main types of context switches:
 *  1. Tied to a function. Returning from the context switch will trigger a
 *     callback to a destination function. Arguments can be passed in the
 *     process.
 *  2. Unbound. Only the state and MPU configurations are switched. The host OS
 *     must take care of deferring execution to a different callback. The stack
 *     frame and stack pointer of the destination box are not touched. */
typedef enum {
    CONTEXT_SWITCH_FUNCTION_GATEWAY, /**< Bound to function. Allows 4 32 bit arguments, 1 32 bit return value. */
    CONTEXT_SWITCH_FUNCTION_ISR,     /**< Bound to an ISR. Does not allow arguments, nor a return value. */
    CONTEXT_SWITCH_FUNCTION_DEBUG,   /**< Bound to debug handler. Uses the format specified by the debug box driver. */
    CONTEXT_SWITCH_UNBOUND_THREAD,   /**< Not bound to a handler. Used for thread switching. No switch-out. */
} TContextSwitchType;

/** Snapshot of a previous context state
 *
 * This struct contains the information needed to restore a previously stalled
 * context. It is needed when returning from a nested context switch.
 *
 * The padding and alignment requirements ensure that this structure is accessed
 * more quickly. In particular, the src_id field is put first so that a single
 * byte-load to the structure location gives the source box ID directly.
 *
 * @warning We assume that the context type (a value from the
 * ::TContextSwitchType enum) fits into 8 bits. */
typedef struct {
    uint8_t src_id;   /**< ID of the box the context belongs to */
    uint8_t type;     /**< Context switch type */
    uint8_t __pad[2]; /**< Padding to get 32 bit alignment */
    uint32_t src_sp;  /**< Stack pointer to restore for the context */
} UVISOR_PACKED TContextPreviousState;

/** Current state of a box
 *
 * This struct contains the stack pointer and the bss pointer for all boxes.
 * The bss pointer is expected not to change once a box has been
 * initialized. Instead, the stack pointer is updated at every context switch.
 *
 * @warning The stack pointer field is not an accurate snapshot of the boxes'
 * stack pointer at every time. It is only updated when a context switch occurs.
 */
typedef struct {
    uint32_t sp;        /**< Stack pointer */
    uint32_t bss;       /**< Bss pointer */
    uint32_t bss_size;  /**< Bss size */
} TContextCurrentState;

/** Currently active box */
uint8_t g_active_box;

/** Array of current state for all boxes
 *
 * @warning The stack pointer field is not an accurate snapshot of the boxes'
 * stack pointer at every time. It is only updated when a context switch occurs.
 */
TContextCurrentState g_context_current_states[UVISOR_MAX_BOXES];

/** Return a pointer to the previous box context state.
 *
 * The previous context is not popped out of the state stack. Instead, only a
 * pointer to the previous state is returned, so that the user can infer whether
 * the box is running as a result of a context switch.
 *
 * @returns the pointer to the previous box context state. */
TContextPreviousState * context_state_previous(void);

/** Forge a new exception stack frame and copy arguments from an old one.
 *
 * @warning This function trusts all the arguments that are passed to it. Input
 * verification should be performed by the caller. In particular, it is assumed
 * that the source exception stack pointer really points to a source-box-owned
 * memory.
 *
 * @warning This function assumes that the source context is still active while
 * forging a new stack frame in the destination stack. It is also assumed that
 * the destination context is disabled.
 *
 * @warning The FPU exception stack frame is currently not supported.
 *
 * @param src_sp[in]    Stack pointer of the box we are switching from
 * @param dst_id[in]    ID of the of the box we are switching to
 * @param dst_fn[in]    Destination handler for the function-bounded context
 *                      swtich
 * @param dst_lr[in]    Handler to execute upon return from the context switch
 *                      destination handler
 * @param xpsr[in]      Value to put in the stacked xPSR register
 * @param nargs[in]     Number of arguments to pass to the destination handler.
 *                      Max allowed: CONTEXT_SWITCH_FUNCTION_MAX_NARGS
 * @returns the stack pointer pointing to the newly forged exception frame. */
uint32_t context_forge_exc_sf(uint32_t src_sp, uint8_t dst_id, uint32_t dst_fn, uint32_t dst_lr, uint32_t xpsr, int nargs);

/** Discard an unused exception stack frame from the destination box.
 *
 * This function discards an excpetion stack frame pointed by the input stack
 * pointer. It can be used to delete the spurious SVCall exception stack frame
 * that is created upon return from a function-bounded context switch.
 *
 * @warning This function trusts all the arguments that are passed to it. In
 * particular, it does not check whether the passed stack pointer really belongs
 * to the destination box, nor that it effectively points to an SVCall-generated
 * stack frame.
 *
 * @warning The FPU exception stack frame is currently not supported.
 *
 * @param dst_id[in]    ID of the destination box, which owns the stack frame to
 *                      discard
 * @param dst_sp[in]    Pointer to the exception stack frame to discard */
void context_discard_exc_sf(uint8_t dst_id, uint32_t dst_sp);

/** Validate an exception stack frame, checking that the currently active box
 *  has access to it.
 *
 * @warning This function assumes that the currently active box is the one that
 * is being verified here. If a context is switched before calling this
 * function, the checks will apply to the newly switched context.
 *
 * @warning The FPU exception stack frame is currently not supported.
 *
 * @param exc_sp[in]    The stack pointer after an exception occurred.
 * @returns the same stack pointer that was given as input, if no access fault
 * occurred. */
uint32_t context_validate_exc_sf(uint32_t exc_sp);

/** Switch the context from the source box to the destination one, using the
 *  stack pointers provided as input.
 *
 * This function requires not only the ID of the destination box, but also the
 * stack pointers of both the source and destination boxes, as different
 * gateways might use different stacks.
 *
 * @warning This function trusts all the arguments that are passed to it. Input
 * verification should be performed by the caller. In particular, we do not
 * verify that the provided stack pointers belong to the corresponding boxes.
 *
 * @param context_type[in]  Type of context switch. See ::TContextSwitchType for
 *                          the available enum items
 * @param dst_id[in]        ID of the box we are switching to
 * @param src_sp[in]        Stack pointer of the box we are switching from
 * @param dst_sp[in]        Stack pointer of the box we are switching to. Unused
 *                          if the context switch is unbound */
void context_switch_in(TContextSwitchType context_type, uint8_t dst_id, uint32_t src_sp, uint32_t dst_sp);

/** Switch the context back from the destination box to the source one.
 *
 * @param context_type  Type of context switch to perform
 * @returns the pointer to the previous box context state. */
TContextPreviousState * context_switch_out(TContextSwitchType context_type);

#endif /* __CONTEXT_H__ */
