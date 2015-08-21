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
#ifndef __UVISOR_EXPORTS_H__
#define __UVISOR_EXPORTS_H__

#ifdef  __cplusplus
#define UVISOR_EXTERN extern "C"
#else
#define UVISOR_EXTERN extern
#endif/*__CPP__*/

#define UVISOR_NOINLINE  __attribute__((noinline))
#define UVISOR_PACKED    __attribute__((packed))
#define UVISOR_WEAK      __attribute__((weak))
#define UVISOR_ALIAS(f)  __attribute__((weak, alias (#f)))
#define UVISOR_LINKTO(f) __attribute__((alias (#f)))
#define UVISOR_NORETURN  __attribute__((noreturn))
#define UVISOR_NAKED     __attribute__((naked))
#define UVISOR_RAMFUNC   __attribute__ ((section (".ramfunc"), noinline))

/* array count macro */
#define UVISOR_ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

/* maximum number of boxes allowed: 1 is the minimum (unprivileged box) */
#define UVISOR_MAX_BOXES 5U

#endif/*__UVISOR_EXPORTS_H__*/
