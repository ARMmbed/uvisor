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
#ifndef __LINKER_H__
#define __LINKER_H__

EXTERN const uint32_t __code_end__;
EXTERN const uint32_t __stack_end__;

EXTERN uint32_t __bss_start__;
EXTERN uint32_t __bss_end__;

EXTERN uint32_t __data_start__;
EXTERN uint32_t __data_end__;
EXTERN const uint32_t __data_start_src__;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t *mode;

    /* initialized secret data section */
    uint32_t *cfgtbl_start, *cfgtbl_end;

    /* initialized secret data section */
    uint32_t *data_src, *data_start, *data_end;

    /* uninitialized secret data section */
    uint32_t *bss_start, *bss_end;

    /* uvisors protected flash memory regions */
    uint32_t *secure_start, *secure_end;

    /* memory reserved for uvisor */
    uint32_t *reserved_start, *reserved_end;
} PACKED UvConfig;

EXTERN const UvConfig __uvisor_config;

#endif/*__LINKER_H__*/
