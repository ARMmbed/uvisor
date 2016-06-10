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
#include <uvisor.h>
#include "context.h"
#include "halt.h"
#include "register_gateway.h"
#include "vmpu.h"

#define REGISTER_GATEWAY_STATUS_OK            (0)
#define REGISTER_GATEWAY_STATUS_ERROR_FLASH   (-1)
#define REGISTER_GATEWAY_STATUS_ERROR_MAGIC   (-2)
#define REGISTER_GATEWAY_STATUS_ERROR_BOX_PTR (-3)
#define REGISTER_GATEWAY_STATUS_ERROR_BOX_ID  (-4)
#define REGISTER_GATEWAY_STATUS_ERROR_ADDRESS (-5)

/** Perform the validation of a register gateway.
 * @internal
 * @warning This function does not verify that the operation, value and mask
 * make sense together. The code that performs the operation should check for
 * this and return an error accordingly.
 * @param register_gateway[in]  Pointer to the register gateway structure in
 *                              flash.
 * @returns the status of the check. A negative number indicates an error.
 */
static int register_gateway_check(TRegisterGateway const * const register_gateway)
{
    /* Verify that the register gateway is in public flash. */
    /* Note: We only check that the start and end addresses of the gateway are
     * in public flash. Since the gateway is fairly small, we can safely assume
     * that all of it is in public flash. */
    if (!vmpu_public_flash_addr((uint32_t) register_gateway) ||
        !vmpu_public_flash_addr((uint32_t) ((void *) register_gateway + sizeof(TRegisterGateway) - 1))) {
        DPRINTF("Register gateway 0x%08X is not in public flash.\r\n", (uint32_t) register_gateway);
        return REGISTER_GATEWAY_STATUS_ERROR_FLASH;
    }

    /* Verify that the register gateway magic is present. */
    if (register_gateway->magic != UVISOR_REGISTER_GATEWAY_MAGIC) {
        DPRINTF("Register gateway 0x%08X does not contain a valid magic (0x%08X).\r\n",
                (uint32_t) register_gateway, register_gateway->magic);
        return REGISTER_GATEWAY_STATUS_ERROR_MAGIC;
    }

    /* Verify that the box configuration pointer is in the dedicated flash
     * region. */
    /* Note: We only check that the box pointer is larger than the start of the
     * pointer region. The subsequent substraction that we do to calculate the
     * box ID is then guaranteed to be sane. */
    if (register_gateway->box_ptr < (uint32_t) __uvisor_config.cfgtbl_ptr_start) {
        DPRINTF("The pointer (0x%08X) in the register gateway 0x%08X is not a valid box configuration pointer.\r\n",
                register_gateway->box_ptr, (uint32_t) register_gateway);
        return REGISTER_GATEWAY_STATUS_ERROR_BOX_PTR;
    }

    /* Verify that the gateway is being performed by the currently active box.
     * The check does not apply if the gateway has been explicitly marked as
     * shared. */
    uint8_t box_id = (uint8_t) ((uint32_t *) register_gateway->box_ptr - __uvisor_config.cfgtbl_ptr_start);
    if (!(register_gateway->operation & __UVISOR_RGW_OP_SHARED_MASK) && (box_id != g_active_box)) {
        DPRINTF("Register gateway is owned by box %d, while the active box is %d.\r\n", box_id, g_active_box);
        return REGISTER_GATEWAY_STATUS_ERROR_BOX_ID;
    }

    /* Verify that the address is in one of the allowed memory sections:
     *   0x40000000 - 0x43FFFFFF: Peripheral + Peripheral alias
     *   0xE00FF000 - 0xE00FFFFF: Custom ROM Table */
    uint32_t address = register_gateway->address;
    if (((address & VMPU_PERIPH_FULL_MASK) != VMPU_PERIPH_START) &&
        ((address & VMPU_ROMTABLE_MASK) != VMPU_ROMTABLE_START)) {
        DPRINTF("Register gateways can only target the peripheral or ROM Table memory regions. "
                "Address 0x%08X not allowed.\r\n", address);
        return REGISTER_GATEWAY_STATUS_ERROR_ADDRESS;
    }

    /* If the code got here, then everything is fine. */
    return REGISTER_GATEWAY_STATUS_OK;
}

/* Perform a register gateway operation. */
void register_gateway_perform_operation(uint32_t svc_sp, uint32_t svc_pc)
{
    /* Check if the SVCall points to a register gateway. */
    TRegisterGateway const * const register_gateway = (TRegisterGateway const * const) svc_pc;
    int status = register_gateway_check(register_gateway);
    if (status != REGISTER_GATEWAY_STATUS_OK) {
        HALT_ERROR(PERMISSION_DENIED, "Register gateway 0x%08X not allowed. Error: %d.",
                   (uint32_t) register_gateway, status);
    }

    /* From now on we can assume the register_gateway structure and the address
     * are valid. */

    /* Fetch the value from the user stack.
     * This is only needed for write operations. */
    uint32_t value = vmpu_unpriv_uint32_read(svc_sp);

    /* De-reference the address.
     * The value at *address is always needed for every operation. */
    uint32_t address = register_gateway->address;
    uint32_t width = (register_gateway->operation & __UVISOR_RGW_OP_WIDTH_MASK) >> __UVISOR_RGW_OP_WIDTH_POS;
    uint32_t result = 0;
    switch(width) {
    case 4:
        result = (uint32_t) *((uint32_t *) address);
        break;
    case 2:
        result = (uint32_t) *((uint16_t *) address);
        break;
    case 1:
        result = (uint32_t) *((uint8_t *) address);
        break;
    default:
        HALT_ERROR(NOT_ALLOWED, "Register level gateway: Width %d not allowed.", width);
        break;
    }

    /* Perform the actual operation.
     * Read operations store the return value onto the user stack. */
    uint32_t operation = (register_gateway->operation & __UVISOR_RGW_OP_TYPE_MASK) >> __UVISOR_RGW_OP_TYPE_POS;
    switch (operation) {
    case UVISOR_RGW_OP_READ:
        return vmpu_unpriv_uint32_write(svc_sp, result);
    case UVISOR_RGW_OP_READ_AND:
        result &= register_gateway->mask;
        return vmpu_unpriv_uint32_write(svc_sp, result);
    case UVISOR_RGW_OP_WRITE:
        result = value;
        break;
    case UVISOR_RGW_OP_WRITE_AND:
        result &= (value | ~(register_gateway->mask));
        break;
    case UVISOR_RGW_OP_WRITE_OR:
        result |= (value & register_gateway->mask);
        break;
    case UVISOR_RGW_OP_WRITE_XOR:
        result ^= (value & register_gateway->mask);
        break;
    case UVISOR_RGW_OP_WRITE_REPLACE:
        result = (result & ~(register_gateway->mask)) | (value & register_gateway->mask);
        break;
    default:
        HALT_ERROR(NOT_ALLOWED, "Register level gateway: Operation 0x%08X not recognised.", operation);
        break;
    }

    /* Store the result at the target address.
     * The code runs here only if the register gateway performs a write
     * operation. */
    switch(width) {
    case 4:
        *((uint32_t *) address) = (uint32_t) result;
        break;
    case 2:
        *((uint16_t *) address) = (uint16_t) result;
        break;
    case 1:
        *((uint8_t *) address) = (uint8_t) result;
        break;
    default:
        HALT_ERROR(NOT_ALLOWED, "Register level gateway: Width %d not allowed.", width);
        break;
    }
}
