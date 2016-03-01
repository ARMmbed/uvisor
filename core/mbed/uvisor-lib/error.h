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
#ifndef __UVISOR_LIB_ERROR_H__
#define __UVISOR_LIB_ERROR_H__

#define UVISOR_ERROR_INVALID_BOX_ID (-2)
#define UVISOR_ERROR_BUFFER_TOO_SMALL (-3)
#define UVISOR_ERROR_BOX_NAMESPACE_ANONYMOUS (-4)

UVISOR_EXTERN void uvisor_error(THaltUserError reason);

#endif /* __UVISOR_LIB_ERROR_H__ */
