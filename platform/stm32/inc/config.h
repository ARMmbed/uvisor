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
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <uvisor-config.h>

/* memory not to be used by uVisor linker script */
#define RESERVED_FLASH 0x400

/* maximum memory used by uVisor */
#define USE_FLASH_SIZE UVISOR_FLASH_SIZE
#define USE_SRAM_SIZE  UVISOR_SRAM_SIZE

/* stack configuration */
#define STACK_SIZE       2048
#define STACK_GUARD_BAND  (STACK_SIZE / 8)

/* reserved regions */
#define ARMv7M_MPU_RESERVED_REGIONS 2

#endif/*__CONFIG_H__*/
