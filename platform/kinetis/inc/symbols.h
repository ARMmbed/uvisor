/*
 * Copyright (c) 2013-2014, ARM Limited, All Rights Reserved
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
#ifndef __SYMBOLS_H__
#define __SYMBOLS_H__

/* Currently we only support the chip on the FRDM-K64F board. */
#if defined(CPU_MK64FN1M0VLL12) || \
    defined(CONFIGURATION_KINETIS_K64F)

#define FLASH_ORIGIN 0x00000000
#define FLASH_LENGTH 0x100000
#define SRAM_ORIGIN  0x1FFF0000
#define SRAM_LENGTH  0x40000

#endif

#endif /* __SYMBOLS_H__ */
