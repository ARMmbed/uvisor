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
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <reent.h>
#include <string.h>

#include "uvisor-lib/uvisor-lib.h"
#include "cmsis_os.h"

#define DPRINTF(...) {};
/* Use printf with caution inside malloc: printf may allocate memory itself,
   so using printf in malloc may lead to recursive calls! */
/* #define DPRINTF(fmt, ...) printf(fmt, ## __VA_ARGS__) */

extern UvisorBoxIndexOS *const __uvisor_ps;
extern void uvisor_malloc_init(void);

static int is_kernel_initialized()
{
    static uint8_t kernel_running = 0;
    return (kernel_running || (kernel_running = osKernelRunning()));
}

static int init_allocator()
{
    int ret = 0;
    if (__uvisor_ps == NULL) {
#if defined(UVISOR_PRESENT) && (UVISOR_PRESENT == 1)
        return -1;
#else
        uvisor_malloc_init();
#endif
    }

    if ((__uvisor_ps->mutex_id == NULL) && is_kernel_initialized()) {
        /* point the mutex pointer to the data */
        __uvisor_ps->mutex = &(__uvisor_ps->mutex_data);
        /* create mutex if not already done */
        __uvisor_ps->mutex_id = osMutexCreate((osMutexDef_t *) &(__uvisor_ps->mutex));
        /* mutex failed to be created */
        if (__uvisor_ps->mutex_id == NULL) {
            return -1;
        }
    }

    if (__uvisor_ps->index.active_heap == NULL) {
        /* we need to initialize the process heap */
        if (__uvisor_ps->index.process_heap != NULL) {
            /* lock the mutex during initialization */
            int kernel_initialized = is_kernel_initialized();
            if (kernel_initialized) {
                osMutexWait(__uvisor_ps->mutex_id, osWaitForever);
            }
            /* initialize the process heap */
            UvisorAllocator allocator = uvisor_allocator_create_with_pool(
                __uvisor_ps->index.process_heap,
                __uvisor_ps->index.process_heap_size);
            /* set the allocator */
            ret = allocator ? 0 : -1;
            __uvisor_ps->index.active_heap = allocator;
            /* release the mutex */
            if (kernel_initialized) {
                osMutexRelease(__uvisor_ps->mutex_id);
            }
        }
        else {
            DPRINTF("uvisor_allocator: No process heap available!\n");
            ret = -1;
        }
    }
    return ret;
}

/* public API */
UvisorAllocator uvisor_get_allocator(void)
{
    if (init_allocator()) {
        return NULL;
    }
    return __uvisor_ps->index.active_heap;
}

#define OP_MALLOC  0
#define OP_REALLOC 1
#define OP_FREE    2

#define HEAP_ACTIVE  0
#define HEAP_PROCESS 1

static void * memory(void * ptr, size_t size, int heap, int operation)
{
    /* buffer the return value */
    void * ret = NULL;
    /* initialize allocator */
    if (init_allocator()) {
        return NULL;
    }
    /* check if we need to aquire the mutex */
    int mutexed = (is_kernel_initialized() && (heap == HEAP_PROCESS));
    void * allocator = (heap == HEAP_PROCESS) ?
                       (__uvisor_ps->index.process_heap) :
                       (__uvisor_ps->index.active_heap);

    /* aquire the mutex if required */
    if (mutexed) {
        osMutexWait(__uvisor_ps->mutex_id, osWaitForever);
    }
    /* perform the required operation */
    switch(operation)
    {
        case OP_MALLOC:
            ret = uvisor_malloc(allocator, size);
            break;
        case OP_REALLOC:
            ret = uvisor_realloc(allocator, ptr, size);
            break;
        case OP_FREE:
            uvisor_free(allocator, ptr);
            break;
        default:
            break;
    }
    /* release the mutex if required */
    if (mutexed) {
        osMutexRelease(__uvisor_ps->mutex_id);
    }
    return ret;
}

/* wrapped memory management functions */
#if defined (__CC_ARM)
void * $Sub$$_malloc_r(struct _reent * r, size_t size) {
    return memory(r, size, HEAP_ACTIVE, OP_MALLOC);
}
void * $Sub$$_realloc_r(struct _reent * r, void * ptr, size_t size) {
    (void)r;
    return memory(ptr, size, HEAP_ACTIVE, OP_REALLOC);
}
void $Sub$$_free_r(struct _reent * r, void * ptr) {
    (void)r;
    memory(ptr, 0, HEAP_ACTIVE, OP_FREE);
}
#elif defined (__GNUC__)
void * __wrap__malloc_r(struct _reent * r, size_t size) {
    return memory(r, size, HEAP_ACTIVE, OP_MALLOC);
}
void * __wrap__realloc_r(struct _reent * r, void * ptr, size_t size) {
    (void)r;
    return memory(ptr, size, HEAP_ACTIVE, OP_REALLOC);
}
void __wrap__free_r(struct _reent * r, void * ptr) {
    (void)r;
    memory(ptr, 0, HEAP_ACTIVE, OP_FREE);
}
#elif defined (__ICCARM__)
#   warning "Using uVisor allocator is not available for IARCC. Falling back to newlib allocator."
#endif

void * malloc_p(size_t size) {
    return memory(NULL, size, HEAP_PROCESS, OP_MALLOC);
}
void * realloc_p(void * ptr, size_t size) {
    return memory(ptr, size, HEAP_PROCESS, OP_REALLOC);
}
void free_p(void * ptr) {
    memory(ptr, 0, HEAP_PROCESS, OP_FREE);
}
