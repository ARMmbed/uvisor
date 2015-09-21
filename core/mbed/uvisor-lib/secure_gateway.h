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
#ifndef __SECURE_GATEWAY_H__
#define __SECURE_GATEWAY_H__

/* secure gateway metadata */
#if defined(__CC_ARM)

/* TODO/FIXME */

#elif defined(__GNUC__)

#define __UVISOR_SECURE_GATEWAY_METADATA(dst_box, dst_fn) \
    "b.n skip_args%=\n" \
    ".word "UVISOR_TO_STRING(UVISOR_SVC_GW_MAGIC)"\n" \
    ".word "UVISOR_TO_STRING(dst_fn)"\n" \
    ".word "UVISOR_TO_STRING(dst_box)"_cfg_ptr\n" \
    "skip_args%=:\n"

#endif /* defined(__CC_ARM) || defined(__GNUC__) */

/* secure gateway */
#define secure_gateway(dst_box, dst_fn, ...) \
    ({ \
        uint32_t res = UVISOR_SVC(UVISOR_SVC_ID_SECURE_GATEWAY(UVISOR_MACRO_NARGS(__VA_ARGS__)), \
                                  __UVISOR_SECURE_GATEWAY_METADATA(dst_box, dst_fn), \
                                  __VA_ARGS__); \
        res; \
    })

#endif/*__SECURE_GATEWAY_H__*/
