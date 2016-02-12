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
#include "uvisor-lib/uvisor-lib.h"

void vIRQ_SetVectorX(uint32_t irqn, uint32_t vector, uint32_t flag)
{
    if(__uvisor_mode == 0) {
        uvisor_disabled_set_vector(irqn, vector);
    }
    else {
        UVISOR_SVC(UVISOR_SVC_ID_ISR_SET, "", irqn, vector, flag);
    }
}

void vIRQ_SetVector(uint32_t irqn, uint32_t vector)
{
    vIRQ_SetVectorX(irqn, vector, 0);
}

uint32_t vIRQ_GetVector(uint32_t irqn)
{
    if(__uvisor_mode == 0) {
        return uvisor_disabled_get_vector(irqn);
    }
    else {
        return UVISOR_SVC(UVISOR_SVC_ID_ISR_GET, "", irqn);
    }
}

void vIRQ_EnableIRQ(uint32_t irqn)
{
    if(__uvisor_mode == 0) {
        NVIC_EnableIRQ((IRQn_Type) irqn);
    }
    else {
        UVISOR_SVC(UVISOR_SVC_ID_IRQ_ENABLE, "", irqn);
    }
}

void vIRQ_DisableIRQ(uint32_t irqn)
{
    if(__uvisor_mode == 0) {
        NVIC_DisableIRQ((IRQn_Type) irqn);
    }
    else {
        UVISOR_SVC(UVISOR_SVC_ID_IRQ_DISABLE, "", irqn);
    }
}

void vIRQ_ClearPendingIRQ(uint32_t irqn)
{
    if(__uvisor_mode == 0) {
        NVIC_ClearPendingIRQ((IRQn_Type) irqn);
    }
    else {
        UVISOR_SVC(UVISOR_SVC_ID_IRQ_PEND_CLR, "", irqn);
    }
}

void vIRQ_SetPendingIRQ(uint32_t irqn)
{
    if(__uvisor_mode == 0) {
        NVIC_SetPendingIRQ((IRQn_Type) irqn);
    }
    else {
        UVISOR_SVC(UVISOR_SVC_ID_IRQ_PEND_SET, "", irqn);
    }
}

uint32_t vIRQ_GetPendingIRQ(uint32_t irqn)
{
    if(__uvisor_mode == 0) {
        return NVIC_GetPendingIRQ((IRQn_Type) irqn);
    }
    else {
        return UVISOR_SVC(UVISOR_SVC_ID_IRQ_PEND_GET, "", irqn);
    }
}

void vIRQ_SetPriority(uint32_t irqn, uint32_t priority)
{
    if(__uvisor_mode == 0) {
        NVIC_SetPriority((IRQn_Type) irqn, priority);
    }
    else {
        UVISOR_SVC(UVISOR_SVC_ID_IRQ_PRIO_SET, "", irqn, priority);
    }
}

uint32_t vIRQ_GetPriority(uint32_t irqn)
{
    if(__uvisor_mode == 0) {
        return NVIC_GetPriority((IRQn_Type) irqn);
    }
    else {
        return UVISOR_SVC(UVISOR_SVC_ID_IRQ_PRIO_GET, "", irqn);
    }
}

int vIRQ_GetLevel(void)
{
    /* if uVisor is disabled we use the standard priority levels */
    if (__uvisor_mode == 0) {
        /* check if an IRQn is active (an ISR is being served) */
        uint32_t ipsr = __get_IPSR();
        int irqn = (int) (ipsr & 0x1FF) - NVIC_USER_IRQ_OFFSET;
        if (irqn >= NVIC_NUM_VECTORS || !ipsr || !NVIC_GetActive((IRQn_Type) irqn)) {
            return -1;
        }

        /* return the priority level of the active IRQ */
        /* if we are in a system interrupt we do not provide the actual
         * priority level, which is usually negative, since we already use -1
         * for stating "no IRQn is active". This is consistent with the
         * behavior of the actual uVisor-managed API, as this call will never
         * come from a system interrupt. The caller can still use this
         * information to assess that an IRQn is actually active (0 = lowest
         * priority) */
        if (irqn < 0) {
            return 0;
        }
        else {
            return NVIC_GetPriority((IRQn_Type) irqn);
        }
    }
    else {
        return UVISOR_SVC(UVISOR_SVC_ID_IRQ_LEVEL_GET, "");
    }
}
