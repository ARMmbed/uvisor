/* General C++ Object Thunking class
 *
 * - allows direct callbacks to non-static C++ class functions
 * - keeps track for the corresponding class instance
 * - supports an optional context parameter for the called function
 * - ideally suited for class object receiving interrupts (NVIC_SetVector)
 *
 * Copyright (c) 2014 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CTHUNK_H__
#define __CTHUNK_H__

/* IRQ/Exception compatible thunk entry function */
typedef void (*CThunkEntry)(void);
typedef void (*CThunkCallback)(void* instance, void* context);

template<class T>
class CThunk
{
    public:
        typedef void (T::*CCallbackSimple)(void);
        typedef void (T::*CCallback)(void* context);

        inline CThunk(T *instance)
        {
            init(instance, NULL, NULL);
        }

        inline CThunk(T &instance, CCallback callback)
        {
            init(instance, callback, NULL);
        }

        inline CThunk(T &instance, CCallbackSimple callback)
        {
            init(instance, (CCallback)callback, NULL);
        }

        inline CThunk(T &instance, CCallback callback, void* context)
        {
            init(instance, callback, context);
        }

        inline void callback(CCallback callback)
        {
            m_callback = callback;
        }

        inline void callback(CCallbackSimple callback)
        {
            m_callback = (CCallback)callback;
        }

        inline void context(void* context)
        {
            m_thunk.context = (uint32_t)context;
        }

        inline void context(uint32_t context)
        {
            m_thunk.context = context;
        }

        /* get thunk entry point for connecting rhunk to an IRQ table */
        inline operator CThunkEntry(void)
        {
            return (CThunkEntry)&m_thunk;
        }

        /* get thunk entry point for connecting rhunk to an IRQ table */
        inline operator uint32_t(void)
        {
            return (uint32_t)&m_thunk;
        }

        /* simple test function */
        inline void call(void)
        {
            ((CThunkEntry)&m_thunk)();
        }

    private:
        volatile T* m_instance;
        volatile CCallback m_callback;
        typedef struct
        {
            uint32_t code;
            uint32_t instance;
            uint32_t context;
            uint32_t callback;
        } __attribute__((packed)) CThunkTrampoline;

        static void trampoline(void* instance, void* context)
        {
            dprintf("Trampoline: instance=0x%08X context=0x%08X\n", instance, context);
//            ((CThunk*)instance)->context;
        }

        volatile CThunkTrampoline m_thunk;

        inline void init(T *instance, CCallback callback, void* context)
        {
#ifdef __thumb2__
            m_thunk.code = 0x8003E89F;
#else
#error "TODO: add support for non-cortex-m3 trampoline, too"
#endif
            m_thunk.context = (uint32_t)context;
            m_thunk.callback = (uint32_t)&trampoline;
            m_callback = callback;
            m_instance = instance;
        }
};

#endif/*__CTHUNK_H__*/
