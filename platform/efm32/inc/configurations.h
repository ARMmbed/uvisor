/*
 * Copyright 2016 Silicon Laboratories, Inc.
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
#ifndef __CONFIGURATIONS_H__
#define __CONFIGURATIONS_H__

/*******************************************************************************
 * Universal configurations
 ******************************************************************************/

/* Maximum number of interrupt vectors */
#define NVIC_VECTORS 40

/* Memory boundaries */
#define FLASH_ORIGIN 0x00000000
#define FLASH_OFFSET 0x100

/* Memory boundaries */
#define SRAM_ORIGIN  0x20000000
#define SRAM_OFFSET  0x0

/* Minimum memory requirements */
#define FLASH_LENGTH_MIN 0x10000    /* 64k */
#define SRAM_LENGTH_MIN  0x8000     /* 32k */

/* Host platform memory requirements */
#define HOST_SRAM_ORIGIN_MIN 0x20000000
#define HOST_SRAM_LENGTH_MAX 0x20000

/*******************************************************************************
 * Hardware-specific configurations
 *
 * Configurations are named after the parameter values, in this order:
 *   - CORE
 *   - PLATFORM
 ******************************************************************************/
#if defined(CONFIGURATION_EFM32_M3_P1)
    /* ARM core selection */
#   define CORE_CORTEX_M3
#elif defined(CONFIGURATION_EFM32_M4_P1)
    /* ARM core selection */
#   define CORE_CORTEX_M4
#else
#    error "Unrecognized uVisor configuration. Check your Makefile."
#endif

#endif /* __CONFIGURATIONS_H__ */
