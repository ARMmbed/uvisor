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
#ifndef __VMPU_FREESCALE_K64_MAP_H__
#define __VMPU_FREESCALE_K64_MAP_H__

#define MEMORY_MAP_SRAM_START       ((uint32_t) SRAM_ORIGIN)
#define MEMORY_MAP_SRAM_END         ((uint32_t) (SRAM_ORIGIN + SRAM_LENGTH))
#define MEMORY_MAP_PERIPH_START     ((uint32_t) 0x40000000)
#define MEMORY_MAP_PERIPH_END       ((uint32_t) 0x400FEFFF)
#define MEMORY_MAP_GPIO_START       ((uint32_t) 0x400FF000)
#define MEMORY_MAP_GPIO_END         ((uint32_t) 0x400F0000)
#define MEMORY_MAP_BITBANDING_START ((uint32_t) 0x42000000)
#define MEMORY_MAP_BITBANDING_END   ((uint32_t) 0x43FFFFFF)

#endif/*__VMPU_FREESCALE_K64_MAP_H__*/
