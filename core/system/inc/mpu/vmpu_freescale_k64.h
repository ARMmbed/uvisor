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
#ifndef __VMPU_FREESCALE_K64_H__
#define __VMPU_FREESCALE_K64_H__

/* MPU fault status codes */
#define VMPU_FAULT_NONE     (-1)
#define VMPU_FAULT_MULTIPLE (-2)

/** Get the slave port number from the MPU->CESR SPERR field.
 * @returns The slave port number, or a negative value to signal an error
 * @retval \ref VMPU_FAULT_NONE     No MPU violation found
 * @retval \ref VMPU_FAULT_MULTIPLE Multiple MPU violations found
 */
UVISOR_FORCEINLINE int vmpu_fault_get_slave_port(void)
{
    uint32_t mpu_cesr_sperr = (MPU->CESR & MPU_CESR_SPERR_MASK) >> MPU_CESR_SPERR_SHIFT;
    switch (mpu_cesr_sperr) {
        case 0x00:
            return VMPU_FAULT_NONE;
        case 0x01:
            return 4;
        case 0x02:
            return 3;
        case 0x04:
            return 2;
        case 0x08:
            return 1;
        case 0x10:
            return 0;
        default:
            return VMPU_FAULT_MULTIPLE;
    }
}

/** Clear the fault bits in the MPU->CESR SPERR field.
 * @note We only clear the fault required by the input argument. If there are
 * multiple faults only one will be cleared.
 * @param slave_port[in]    Slave port number for which the fault must be
 *                          cleared
 */
UVISOR_FORCEINLINE void vmpu_fault_clear_slave_port(int slave_port)
{
    /* We only clear the bits if the slave port is valid. */
    if (slave_port >= 0 && slave_port <= 4) {
        uint32_t slave_port_bits = 1 << ((uint32_t) slave_port);
        MPU->CESR |= (slave_port_bits << MPU_CESR_SPERR_SHIFT) & MPU_CESR_SPERR_SHIFT;
    }
}

#endif /* __VMPU_FREESCALE_K64_H__ */
