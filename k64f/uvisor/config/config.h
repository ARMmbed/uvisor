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

#define RESERVED_FLASH 0x420
#define RESERVED_SRAM  0x200

#define XTAL_FREQUENCY 48000000UL

#define USE_FLASH_SIZE UVISOR_FLASH_SIZE
#define USE_SRAM_SIZE  UVISOR_SRAM_SIZE

#define STACK_SIZE 2048
#define STACK_GUARD_BAND 128
#define TOTAL_SRAM_SIZE (256*1024UL)

#endif/*__CONFIG_H__*/
