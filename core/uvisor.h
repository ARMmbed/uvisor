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
#ifndef __UVISOR_H__
#define __UVISOR_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* The platform configuration file must be included first. */
#include "config.h"

/* uVisor configurations */
#include "uvisor-config.h"

/* CMSIS header files */
#include "core_generic.h"

/* Definitions and checks for hardware-specific support. */
#include "hardware_support.h"

/* definitions that are made visible externally */
#include "api/inc/uvisor_exports.h"

#ifndef TRUE
#define TRUE 1
#endif/*TRUE*/

#ifndef FALSE
#define FALSE 0
#endif/*FALSE*/

#include <tfp_printf.h>
#ifndef dprintf
#define dprintf tfp_printf
#endif/*dprintf*/

/* unprivileged box as called by privileged code */
typedef void (*UnprivilegedBoxEntry)(void);

#ifdef  NDEBUG
#define DPRINTF(...) {}
#define assert(...)
#else /*NDEBUG*/
#define DPRINTF dprintf
#define assert(x) if(!(x)){dprintf("HALTED(%s:%i): assert(%s)\n",\
                                   __FILE__, __LINE__, #x);while(1);}
#endif/*NDEBUG*/

#ifdef  CHANNEL_DEBUG
#if (CHANNEL_DEBUG<0) || (CHANNEL_DEBUG>31)
#error "Debug channel needs to be >=0 and <=31"
#endif
#endif/*CHANNEL_DEBUG*/

/* Core-only compiler attributes
 * These definitions should be shared by the uVisor library. */
#define UVISOR_NOINLINE    __attribute__((noinline))
#define UVISOR_ALIAS(f)    __attribute__((weak, alias (#f)))
#define UVISOR_LINKTO(f)   __attribute__((alias (#f)))
#define UVISOR_NAKED       __attribute__((naked))

/* system wide error codes  */
#include "iot-error.h"

/* IOT-OS specific includes */
#include "linker.h"

/* System IRQ vector table and default ISRs */
#include "system.h"

#endif/*__UVISOR_H__*/
