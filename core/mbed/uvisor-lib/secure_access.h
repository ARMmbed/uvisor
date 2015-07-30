/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#ifndef __SECURE_ACCESS_H__
#define __SECURE_ACCESS_H__

#define ADDRESS_WRITE32(addr, val) uvisor_write32((volatile uint32_t *) (addr), (uint32_t) (val))
#define ADDRESS_WRITE16(addr, val) uvisor_write16((volatile uint16_t *) (addr), (uint16_t) (val))
#define ADDRESS_WRITE8(addr, val)  uvisor_write8((volatile uint8_t  *) (addr), (uint8_t ) (val))

#define ADDRESS_READ32(addr) uvisor_read32((volatile uint32_t *) (addr))
#define ADDRESS_READ16(addr) uvisor_read16((volatile uint16_t *) (addr))
#define ADDRESS_READ8(addr)  uvisor_read8((volatile uint8_t  *) (addr))

#define UNION_READ_FS(addr, type, field) \
    ({ \
        type ## _t res; \
        res.U = uvisor_read32((volatile uint32_t *) (addr)); \
        res.field; \
    })

static inline __attribute__((always_inline)) void uvisor_write32(uint32_t volatile *addr, uint32_t val)
{
    register uint32_t r0 __asm__("r0") = (uint32_t) addr;
    register uint32_t r1 __asm__("r1") = val;
    __asm__ volatile(
        "str %[v], [%[a]]\n"
        :: [a] "r" (r0),
           [v] "r" (r1)
    );
}

static inline __attribute__((always_inline)) void uvisor_write16(uint16_t volatile *addr, uint16_t val)
{
    register uint32_t r0 __asm__("r0") = (uint32_t) addr;
    register uint16_t r1 __asm__("r1") = val;
    __asm__ volatile(
        "strh %[v], [%[a]]\n"
        :: [a] "r" (r0),
           [v] "r" (r1)
    );
}

static inline __attribute__((always_inline)) void uvisor_write8(uint8_t volatile *addr, uint8_t val)
{
    register uint32_t r0 __asm__("r0") = (uint32_t) addr;
    register uint8_t  r1 __asm__("r1") = val;
    __asm__ volatile(
        "strb %[v], [%[a]]\n"
        :: [a] "r" (r0),
           [v] "r" (r1)
    );
}

static inline __attribute__((always_inline)) uint32_t uvisor_read32(uint32_t volatile *addr)
{
    register uint32_t r0  __asm__("r0") = (uint32_t) addr;
    register uint32_t res __asm__("r0");
    __asm__ volatile(
        "ldr %[r], [%[a]]\n"
        : [r] "=r" (res)
        : [a] "r"  (r0)
    );
    return (uint32_t) res;
}

static inline __attribute__((always_inline)) uint16_t uvisor_read16(uint16_t volatile *addr)
{
    register uint32_t r0  __asm__("r0") = (uint32_t) addr;
    register uint16_t res __asm__("r0");
    __asm__ volatile(
        "ldrh %[r], [%[a]]\n"
        : [r] "=r" (res)
        : [a] "r"  (r0)
    );
    return res;
}

static inline __attribute__((always_inline)) uint8_t uvisor_read8(uint8_t volatile *addr)
{
    register uint32_t r0  __asm__("r0") = (uint32_t) addr;
    register uint8_t  res __asm__("r0");
    __asm__ volatile(
        "ldrb %[r], [%[a]]\n"
        : [r] "=r" (res)
        : [a] "r"  (r0)
    );
    return res;
}

#endif/*__SECURE_ACCESS_H__*/
