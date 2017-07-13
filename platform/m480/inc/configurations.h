/*
 * Copyright (c) 2016-2017, Nuvoton, All Rights Reserved
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

/* The symbols below *must* be calculated from values across the family. */

/* Maximum number of vectors seen across the family:
 *   NVIC_VECTORS = max(NVIC_VECTORS_i) for i in family */
#define NVIC_VECTORS 96

/* Minimum memory requirements:
 *   FLASH_LENGTH_MIN = min(FLASH_LENGTH_i) for i in family
 *   SRAM_LENGTH_MIN = min(SRAM_LENGTH_i) for i in family */
#define FLASH_LENGTH_MIN 0x40000
#define SRAM_LENGTH_MIN  0x18000

/* The symbols below can be either configuration-specific or family-wide,
 * depending on your requirements. See the porting guide for more details. */

/* Memory boundaries */
#define FLASH_ORIGIN 0x00000000
#define FLASH_OFFSET 0x400

/*******************************************************************************
 * Hardware-specific configurations
 *
 * Configurations are named after the parameter values, in this order:
 *   - CORE
 *   - SRAM_ORIGIN
 *   - SRAM_OFFSET
 ******************************************************************************/

/* The symbols below are specific to each configuration. */

#if defined(CONFIGURATION_M480_CORTEX_M4_0x20000000_0x0)

/* ARM core selection */
#define CORE_CORTEX_M4

/* Memory boundaries */
#define SRAM_ORIGIN 0x20000000
#define SRAM_OFFSET 0x0

#else /* Hardware-specific configurations */

#error "Unrecognized uVisor configuration. Check your Makefile."

#endif /* Hardware-specific configurations */

#endif /* __CONFIGURATIONS_H__ */
