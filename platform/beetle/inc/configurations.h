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
#ifndef __CONFIGURATIONS_H__
#define __CONFIGURATIONS_H__

/*******************************************************************************
 * Family-wide configurations
 ******************************************************************************/

/* NVIC configuration */
/* Note: This is the maximum of all the avilable NVIC vectors and hence it is
 *       common to all configurations */
#define NVIC_VECTORS 45

/* Minimum memory requirements
 * These symbols basically encode the following:
 *   FLASH_LENGTH_MIN = min(FLASH_LENGTH_i) for i in device family
 *   SRAM_LENGTH_MIN  = min(SRAM_LENGTH_i)  for i in device family */
#define FLASH_LENGTH_MIN 0x40000
#define SRAM_LENGTH_MIN  0x20000

/* Memory boundaries */
#define FLASH_ORIGIN 0x0
#define FLASH_OFFSET 0x400

/*******************************************************************************
 * Hardware-specific configurations
 *
 * Configurations are named after the parameter values, in this order:
 *   - CORE
 *   - SRAM_ORIGIN
 *   - SRAM_OFFSET
 *
 ******************************************************************************/
#if defined(CONFIGURATION_BEETLE_M3_0x20000000_0x0)

/* ARM core selection */
#define CORE_CORTEX_M3

/* Memory boundaries */
#define SRAM_ORIGIN  0x20000000
#define SRAM_OFFSET  0x140

#else /* defined(CONFIGURATION_BEETLE_M3_0x20000000_0x0) */

#error "Unrecognized uVisor configuration. Check your Makefile."

#endif /* unrecognized configuration */

#endif /* __CONFIGURATIONS_H__ */
