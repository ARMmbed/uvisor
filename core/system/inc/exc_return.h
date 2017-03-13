/*
 * Copyright (c) 2016, ARM Limited, All Rights Reserved
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
#ifndef __EXC_RETURN_H__
#define __EXC_RETURN_H__

/* Note: For the ARMv7-M architecture there are no explicit bit names. We use
 * the same names of the ARMv8-M architecture where relevant, so they will be
 * forwards compatible. */

#define EXC_RETURN_FType_Pos    4U
#define EXC_RETURN_FType_Msk    (0x1UL << EXC_RETURN_FType_Pos)
#define EXC_RETURN_Mode_Pos     3U
#define EXC_RETURN_Mode_Msk     (0x1UL << EXC_RETURN_Mode_Pos)
#define EXC_RETURN_SPSEL_Pos    2U
#define EXC_RETURN_SPSEL_Msk    (0x1UL << EXC_RETURN_SPSEL_Pos)

#if defined(ARCH_CORE_ARMv8M)

#define EXC_RETURN_S_Pos        6U
#define EXC_RETURN_S_Msk        (0x1UL << EXC_RETURN_S_Pos)
#define EXC_RETURN_DCRS_Pos     5U
#define EXC_RETURN_DCRS_Msk     (0x1UL << EXC_RETURN_DCRS_Pos)
#define EXC_RETURN_ES_Pos       0U
#define EXC_RETURN_ES_Msk       (0x1UL << EXC_RETURN_ES_Pos)

#define EXC_FROM_S(lr)  (lr & EXC_RETURN_S_Msk)
#define EXC_TO_S(lr)    (lr & EXC_RETURN_ES_Msk)

#else

#define EXC_RETURN_S_Msk        0U

#endif /* defined(ARCH_CORE_ARMv8M) */

#define EXC_FROM_NP(lr)     (lr & EXC_RETURN_Mode_Msk)
#define EXC_FROM_PSP(lr)    (lr & EXC_RETURN_SPSEL_Msk)

#endif /* __EXC_RETURN_H__ */
