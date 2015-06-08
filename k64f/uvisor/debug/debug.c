/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <uvisor.h>
#include "debug.h"
#include "halt.h"
#include "svc.h"
#include "memory_map.h"

#define DEBUG_PRINT_HEAD(x) {\
    DPRINTF("\n***********************************************************\n");\
    DPRINTF("                    "x"\n");\
    DPRINTF("***********************************************************\n\n");\
}
#define DEBUG_PRINT_END()   {\
    DPRINTF("***********************************************************\n\n");\
}

#ifndef DEBUG_MAX_BUFFER
#define DEBUG_MAX_BUFFER 128
#endif/*DEBUG_MAX_BUFFER*/

uint8_t g_buffer[DEBUG_MAX_BUFFER];
int g_buffer_pos;

#ifndef CHANNEL_DEBUG
void default_putc(uint8_t data)
{
    if(g_buffer_pos<(DEBUG_MAX_BUFFER-1))
        g_buffer[g_buffer_pos++] = data;
    else
        data = '\n';

    if(data == '\n')
    {
        g_buffer[g_buffer_pos] = 0;
        asm(
            "mov r0, #4\n"
            "mov r1, %[data]\n"
            "bkpt #0xAB\n"
            :
            : [data] "r" (&g_buffer)
            : "r0", "r1"
        );
        g_buffer_pos = 0;
    }
}
#endif/*CHANNEL_DEBUG*/

inline void debug_print_cx_state(int _indent)
{
    int i;

    /* generate indentation depending on nesting depth */
    char _sp[UVISOR_MAX_BOXES + 4];

    for (i = 0; i < _indent; i++)
    {
        _sp[i]     = ' ';
    }
    _sp[i] = '\0';

    /* print state stack */
    if (!g_svc_cx_state_ptr)
    {
        dprintf("%sNo saved state\n\r", _sp);
    }
    else
    {
        for (i = 0; i < g_svc_cx_state_ptr; i++)
        {
            dprintf("%sState %d\n\r",        _sp, i);
            dprintf("%s  src_id %d\n\r",     _sp, g_svc_cx_state[i].src_id);
            dprintf("%s  src_sp 0x%08X\n\r", _sp, g_svc_cx_state[i].src_sp);
        }
    }

    /* print current stack pointers for all boxes */
    dprintf("%s     ", _sp);
    for (i = 0; i < g_svc_cx_box_num; i++)
    {
        dprintf("------------ ");
    }
    dprintf("\n%sSP: |", _sp);
    for (i = 0; i < g_svc_cx_box_num; i++)
    {
        dprintf(" 0x%08X |", g_svc_cx_curr_sp[i]);
    }
    dprintf("\n%s     ", _sp);
    for (i = 0; i < g_svc_cx_box_num; i++)
    {
        dprintf("------------ ");
    }
    dprintf("\n");
}

inline void debug_print_mpu_config(void)
{
    int i, j;

    dprintf("* MPU CONFIGURATION\n");

    /* CESR */
    dprintf("  CESR: 0x%08X\n", MPU->CESR);
    /* EAR, EDR */
    dprintf("       Slave 0    Slave 1    Slave 2    Slave 3    Slave 4\n");
    dprintf("  EAR:");
    for (i = 0; i < 5; i++) {
        dprintf(" 0x%08X", MPU->SP[i].EAR);
    }
    dprintf("\n  EDR:");
    for (i = 0; i < 5; i++) {
        dprintf(" 0x%08X", MPU->SP[i].EDR);
    }
    dprintf("\n");
    /* region descriptors */
    dprintf("       Start      End        Perm.      Valid\n");
    for (i = 0; i < 12; i++) {
        dprintf("  R%02d:", i);
        for (j = 0; j < 4; j++) {
            dprintf(" 0x%08X", MPU->WORD[i][j]);
        }
        dprintf("\n");
    }
    dprintf("\n");
    /* the alternate view is not printed */
}

inline void debug_print_unpriv_exc_sf(uint32_t lr)
{
    int i;
    uint32_t *sp;
    uint32_t mode = lr & 0x4;

    dprintf("* EXCEPTION STACK FRAME\n");

    /* get stack pointer to exception frame */
    if(mode)
    {
        sp = (uint32_t *) __get_PSP();
        dprintf("  Exception from unprivileged code\n");
        dprintf("    psp:     0x%08X\n\r", sp);
        dprintf("    lr:      0x%08X\n\r", lr);
    }
    else
    {
        dprintf("  Exception from privileged code\n");
        dprintf("  Cannot print exception stack frame.\n\n");
        return;
    }

    char exc_sf_verbose[SVC_CX_EXC_SF_SIZE + 1][6] = {
        "r0", "r1", "r2", "r3", "r12",
        "lr", "pc", "xPSR", "align"
    };

    /* print exception stack frame */
    dprintf("  Exception stack frame:\n");
    i = SVC_CX_EXC_SF_SIZE;
    if(sp[8] & (1 << 9))
    {
        dprintf("    psp[%02d]: 0x%08X | %s\n", i, sp[i], exc_sf_verbose[i]);
    }
    for(i = SVC_CX_EXC_SF_SIZE - 1; i >= 0; --i)
    {
        dprintf("    psp[%02d]: 0x%08X | %s\n", i, sp[i], exc_sf_verbose[i]);
    }
    dprintf("\n");
}

inline uint32_t debug_aips_slot_from_addr(uint32_t aips_addr)
{
    return ((aips_addr & 0xFFF000) >> 12);
}

inline uint32_t debug_aips_bitn_from_alias(uint32_t alias, uint32_t aips_addr)
{
    uint32_t bitn;

    bitn = ((alias - MEMORY_MAP_BITBANDING_START) -
           ((aips_addr - MEMORY_MAP_PERIPH_START) << 5)) >> 2;

    return bitn;
}

inline uint32_t debug_aips_addr_from_alias(uint32_t alias)
{
    uint32_t aips_addr;

    aips_addr  = ((((alias - MEMORY_MAP_BITBANDING_START) >> 2) & ~0x1F) >> 3);
    aips_addr += MEMORY_MAP_PERIPH_START;

    return aips_addr;
}

inline void debug_map_addr_to_periph(uint32_t address)
{
    uint32_t aips_addr;
    const MemMap *map;

    dprintf("* MEMORY MAP\n");
    dprintf("  Address:           0x%08X\n", address);

    /* find system memory region */
    if((map = memory_map_name(address)) != NULL)
    {
        dprintf("  Region/Peripheral: %s\n", map->name);
        dprintf("    Base address:    0x%08X\n", map->base);
        dprintf("    End address:     0x%08X\n", map->end);

        if(address >= MEMORY_MAP_PERIPH_START &&
           address <= MEMORY_MAP_PERIPH_END)
        {
            dprintf("    AIPS slot:       %d\n",
                    debug_aips_slot_from_addr(map->base));
        }
        else if(address >= MEMORY_MAP_BITBANDING_START &&
                address <= MEMORY_MAP_BITBANDING_END)
        {
            dprintf("    Before bitband:  0x%08X\n",
                    (aips_addr = debug_aips_addr_from_alias(address)));
            map = memory_map_name(aips_addr);
            dprintf("    Alias:           %s\n", map->name);
            dprintf("      Base address:  0x%08X\n", map->base);
            dprintf("      End address:   0x%08X\n", map->end);
            dprintf("    Accessed bit:    %d\n",
                     debug_aips_bitn_from_alias(address, aips_addr));
            dprintf("    AIPS slot:       %d\n",
                     debug_aips_slot_from_addr(aips_addr));
        }

        dprintf("\n");
    }

}

inline void debug_cx_switch_in(void)
{
    int i;

    /* indent debug messages linearly with the nesting depth */
    dprintf("\n\r");
    for(i = 0; i < g_svc_cx_state_ptr; i++)
    {
        dprintf("--");
    }
    dprintf("> Context switch in\n");
    debug_print_cx_state(2 + (g_svc_cx_state_ptr << 1));
    dprintf("\n\r");
}

inline void debug_cx_switch_out(void)
{
    int i;

    /* indent debug messages linearly with the nesting depth */
    dprintf("\n\r<--");
    for(i = 0; i < g_svc_cx_state_ptr; i++)
    {
        dprintf("--");
    }
    dprintf(" Context switch out\n");
    debug_print_cx_state(4 + (g_svc_cx_state_ptr << 1));
    dprintf("\n\r");
}

inline void debug_fault_bus(uint32_t lr)
{
    uint32_t addr = SCB->BFAR;

    DEBUG_PRINT_HEAD("BUS FAULT");
    debug_print_unpriv_exc_sf(lr);
    debug_map_addr_to_periph(addr);
    debug_print_mpu_config();
    DEBUG_PRINT_END();
}

inline void debug_mpu_fault(void)
{
    uint32_t cesr = MPU->CESR;
    uint32_t sperr = cesr >> 27;
    uint32_t edr, ear, eacd;
    int s, r, i, found;

    if(sperr)
    {
        for(s = 4; s >= 0; s--)
        {
            if(sperr & (1 << s))
            {
                edr = MPU->SP[4 - s].EDR;
                ear = MPU->SP[4 - s].EAR;
                eacd = edr >> 20;

                dprintf("* MPU FAULT:\n\r");
                dprintf("  Slave port:       %d\n\r", 4 - s);
                dprintf("  Address:          0x%08X\n\r", ear);
                dprintf("  Faulting regions: ");
                found = 0;
                for(r = 11; r >= 0; r--)
                {
                    if(eacd & (1 << r))
                    {
                        if(!found)
                        {
                            dprintf("\n\r");
                            found = 1;
                        }
                        dprintf("    R%02d:", 11 - r);
                        for (i = 0; i < 4; i++) {
                            dprintf(" 0x%08X", MPU->WORD[11 - r][i]);
                        }
                        dprintf("\n\r");
                    }
                }
                if(!found)
                    dprintf("[none]\n\r");
                dprintf("  Master port:      %d\n\r", (edr >> 4) & 0xF);
                dprintf("  Error attribute:  %s %s (%s mode)\n\r",
                        edr & 0x2 ? "Data" : "Instruction",
                        edr & 0x1 ? "WRITE" : "READ",
                        edr & 0x4 ? "supervisor" : "user");
                break;
            }
        }
    }
    else
        dprintf("No MPU violation found\n\r");
    dprintf("\n\r");
}

inline void debug_fault(THaltError reason, uint32_t lr)
{
    /* fault-specific code */
    switch(reason)
    {
        case FAULT_BUS:
            DEBUG_PRINT_HEAD("BUS FAULT");
            debug_mpu_fault();
            debug_map_addr_to_periph(SCB->BFAR);
            dprintf("* CFSR : 0x%08X\n\r\n\r", SCB->CFSR);
            break;
        case FAULT_USAGE:
            DEBUG_PRINT_HEAD("USAGE FAULT");
            dprintf("* CFSR : 0x%08X\n\r\n\r", SCB->CFSR);
            break;
        case FAULT_HARD:
            DEBUG_PRINT_HEAD("HARD FAULT");
            dprintf("* HFSR : 0x%08X\n\r\n\r", SCB->HFSR);
            break;
        case FAULT_DEBUG:
            DEBUG_PRINT_HEAD("DEBUG FAULT");
            dprintf("* DFSR : 0x%08X\n\r\n\r", SCB->DFSR);
            break;
        default:
            dprintf("No fault occurred\n\r");
    }

    /* blue screen */
    debug_print_unpriv_exc_sf(lr);
    debug_print_mpu_config();
    DEBUG_PRINT_END();
}

inline void debug_init(void)
{
    /* debugging bus faults requires them to be precise, so write buffering is
     * disabled; note: it slows down execution */
    SCnSCB->ACTLR |= 0x2;
}
