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
#ifndef __UVISOR_LIB_H__
#define __UVISOR_LIB_H__

#include <stdint.h>

/* the symbol UVISOR_PRESENT is defined here based on the supported platforms */
#include "uvisor-lib/platforms.h"

/* these header files are included independently from the platform */
#include "uvisor-lib/uvisor_exports.h"
#include "uvisor-lib/vmpu_exports.h"
#include "uvisor-lib/halt_exports.h"

/* conditionally included header files */
#ifdef  UVISOR_PRESENT

#include "uvisor-lib/benchmark.h"
#include "uvisor-lib/secure_access.h"
#include "uvisor-lib/box_config.h"
#include "uvisor-lib/error.h"
#include "uvisor-lib/interrupts.h"
#include "uvisor-lib/secure_gateway.h"
#include "uvisor-lib/svc_exports.h"
#include "uvisor-lib/svc_gw_exports.h"

#else /*UVISOR_PRESENT*/

#include "uvisor-lib/unsupported.h"

#endif/*UVISOR_PRESENT*/

#endif/*__UVISOR_LIB_H__*/
