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
#ifndef  __UVISOR_CONFIG_H__

/* Configuration-specific symbols */
#include "config.h"

#define UVISOR_MAGIC 0x2FE539A6

/** Maximum flash space that will be used by uVisor
 * The actual flash space will be resolved at uVisor-core link time. This
 * symbol is only used for link-time verifications, and is only increased upon a
 * link failure, so it can be used to track down a rough estimation of the
 * uVisor flash usage upper bound.
 */
#define UVISOR_FLASH_LENGTH_MAX 0x9C00

/** Size of the SRAM space protected by uVisor for its own SRAM sections
 * The actual SRAM space used by uVisor depends on the SRAM_OFFSET symbol, which
 * is configuration-specific. See \ref UVISOR_SRAM_LENGTH_USED for more
 * information.
 */
#define UVISOR_SRAM_LENGTH_PROTECTED 0x2000

/** Actual SRAM space that will be used by uVisor
 * This is the space that is reserved by the uVisor in the host linker script.
 * The uVisor will use this space for its internal BSS and data sections, stack
 * and heap, but it will protect the full \ref UVISOR_SRAM_LENGTH_PROTECTED.
 */
#define UVISOR_SRAM_LENGTH_USED (UVISOR_SRAM_LENGTH_PROTECTED - SRAM_OFFSET)

/* Check that UVISOR_SRAM_LENGTH_USED is sane. */
#if SRAM_OFFSET >= UVISOR_SRAM_LENGTH_PROTECTED
#error "Invalid SRAM configuration. SRAM_OFFSET must be smaller than UVISOR_SRAM_LENGTH_PROTECTED"
#endif

/* Check that the SRAM offset is aligned to 32 bytes. */
#if (SRAM_ORIGIN + SRAM_OFFSET) & 0x1FUL
#error "Invalid SRAM configuration. SRAM_ORIGIN + SRAM_OFFSET must be aligned to 32 bytes."
#endif /* (SRAM_ORIGIN + SRAM_OFFSET) & 0x1FUL */

/* SRAM_OFFSET >= UVISOR_SRAM_LENGTH_PROTECTED */
/** Check that the SRAM_ORIGIN address is a multiple of the \ref
 * UVISOR_SRAM_LENGTH_PROTECTED space that will be protected by uVisor at
 * runtime. */
/* Note: This only applies to the ARMv7-M MPU driver. */
#if defined(ARCH_MPU_ARMv7M)
#if (SRAM_ORIGIN % UVISOR_SRAM_LENGTH_PROTECTED) != 0
#error "The SRAM origin must be aligned to a multiple of UVISOR_SRAM_LENGTH_PROTECTED"
#endif /* (SRAM_ORIGIN % UVISOR_SRAM_LENGTH_PROTECTED) != 0 */
#endif /* defined(ARCH_MPU_ARMv7M) */

#endif /* __UVISOR_CONFIG_H__ */
