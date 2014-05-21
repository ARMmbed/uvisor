#ifndef __LINKER_H__
#define __LINKER_H__

extern uint32_t __stack_end__;

extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern const uint32_t __bss_size__;

extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern const uint32_t __data_start_src__;
extern const uint32_t __data_size__;
extern uint32_t __stack_start__;

#endif/*__LINKER_H__*/
