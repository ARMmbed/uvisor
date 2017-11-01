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
#ifndef __VMPU_FREESCALE_K64_MEM_H__
#define __VMPU_FREESCALE_K64_MEM_H__

#ifndef UVISOR_MAX_ACLS
#define UVISOR_MAX_ACLS 16
#endif/*UVISOR_MAX_ACLS*/

/* default permission mask for the background MPU region
 *    | # | Bus master | Supervisor | User  |
 *    |---|------------|------------|-------|
 *    | 0 | core       | r/w        | -     |
 *    | 1 | debugger   | r/w/x      | r/w/x |
 *    | 2 | dma        | -          | -     |
 *    | 3 | enet       | -          | -     |
 *    | 4 | usb        | NA         | -     |
 *    | 5 | core       | NA         | -     |
 */

#define MPU_RGDn_WORD2_MxUM_NONE    0x00
#define MPU_RGDn_WORD2_MxUM_EXECUTE 0x01
#define MPU_RGDn_WORD2_MxUM_WRITE   0x02
#define MPU_RGDn_WORD2_MxUM_READ    0x04
#define MPU_RGDn_WORD2_MxSM_RWX     0X00
#define MPU_RGDn_WORD2_MxSM_RX      0X01
#define MPU_RGDn_WORD2_MxSM_RW      0X02
#define MPU_RGDn_WORD2_MxSM_AS_UM   0X03


extern int vmpu_mem_add(uint8_t box_id, void* start, uint32_t size, UvisorBoxAcl acl);
extern void vmpu_mem_switch(uint8_t src_box, uint8_t dst_box);
extern uint32_t vmpu_fault_find_acl_mem(uint8_t box_id, uint32_t fault_addr, uint32_t size);
extern void vmpu_mem_init(void);
extern int vmpu_mem_push_page_acl(uint32_t start_addr, uint32_t end_addr);

#endif/*__VMPU_FREESCALE_K64_MEM_H__*/
