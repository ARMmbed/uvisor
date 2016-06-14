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
#include "uvisor-lib/uvisor-lib.h"
#include "cmsis_os.h"
#include <stdint.h>
#include <stdlib.h> // malloc

/* Symbols exported by the mbed linker script */
UVISOR_EXTERN uint32_t __uvisor_cfgtbl_ptr_start;
UVISOR_EXTERN uint32_t __uvisor_cfgtbl_ptr_end;

/* This must be called from privileged code: it reads the uVisor cfgtbl (which
 * is protected from reading) and also creates threads in different box
 * contexts than the current box context. This function assumes that no other
 * thread creation will be going on in parallel (that could screw up the thread
 * creation context state). */
int box_mains_start(void (* set_thread_creation_context)(int box_id))
{
    int32_t box_id;
    const UvisorBoxConfig ** box_cfgtbl = (const UvisorBoxConfig **)&__uvisor_cfgtbl_ptr_start;

    /* Assert that set_thread_creation_context is non-0. It'd be programmer
     * error if set_thread_creation_context weren't set, as that'd mean the box
     * main threads wouldn't be created in the proper context. */
    assert(set_thread_creation_context != 0);

    box_id = 0;
    for(box_cfgtbl = (const UvisorBoxConfig**) &__uvisor_cfgtbl_ptr_start;
            box_cfgtbl < (const UvisorBoxConfig**) &__uvisor_cfgtbl_ptr_end;
            box_cfgtbl++) {
        const UvisorBoxConfig * box_cfg = *box_cfgtbl;
        if (box_cfg->main_function != NULL) {
            osThreadDef_t thread_def;
            thread_def.pthread = box_cfg->main_function;
            thread_def.tpriority = box_cfg->main_priority;

            /* XXX TODO Decide how we want to come up with a stack size here.
             * Probably needs to use box-specific allocator if we don't want to
             * use the process stack. */
            thread_def.stacksize = box_cfg->main_stack_size;
            thread_def.stack_pointer = g_svc_cx_curr_sp[box_id];

            if (thread_def.stack_pointer == NULL)
            {
                return -1; /* XXX Use nice Out of memory error here */
            }

            /* Set the thread creation context so that all new threads are
             * created in the box that we want to make the main thread for. */
            if (set_thread_creation_context) {
                /* NOTE: This function will fail with a bus fault if we aren't
                 * privileged. */
                set_thread_creation_context(box_id);
            }

            /* osThreadCreate can't be called from inside an
             * ISR. We are currently in an SVC context. So, we call
             * svcThreadCreate directly. This is a bit dirty because its an
             * implementation detail function. There is no public CMSIS RTOS
             * function to create a thread from privileged mode. */
            //osThreadCreate(&thread_def, NULL); XXX Delete me.
            extern osThreadId  svcThreadCreate(const osThreadDef_t * thread_def, void * argument, void * context);
            svcThreadCreate(&thread_def, NULL, NULL);
        }

        box_id++;
    }

    /* Reset the thread creation context so that all new threads are created in
     * the g_active_box. */
    if (set_thread_creation_context) {
        set_thread_creation_context(-1);
    }

    return 0;
}
