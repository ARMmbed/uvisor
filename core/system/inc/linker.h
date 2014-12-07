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

    /* uvisors protected flash memory regions */
    uint32_t *secure_start, *secure_end;

} PACKED TUVisorConfig;

EXTERN const TUVisorConfig __uvisor_config;

#endif/*__LINKER_H__*/
