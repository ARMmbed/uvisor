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
#ifndef __IOT_ERROR_H__
#define __IOT_ERROR_H__

/* include system-wide error codes */
#define E_SUCCESS  0
#define E_ERROR   -1
#define E_ALIGN   -2
#define E_PARAM   -3
#define E_INVALID -4
#define E_MEMORY  -5

/* custom error codes start at E_USER */
#define E_USER -0x1000

#endif/*__IOT_ERROR_H__*/
