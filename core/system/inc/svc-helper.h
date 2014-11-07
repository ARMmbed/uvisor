#ifndef __SVC_HELPER_H__
#define __SVC_HELPER_H__

/* FIXME all these helper functions must be inlined */
/* FIXME all these helper functions do not automatically provide arguments
 *      to the SCV handler nor receive its result at return. The handler
 *      must take care of fetching arguments from the exception sf and
 *      copy the result on the stacked r0 */

#define __SVC0(number) ({ \
    register uint32_t __p0  asm("r12") = number; \
    register uint32_t __res asm("r0"); \
    asm volatile("svc %0" : "=r" (__res) : "I" (number), "r" (__p0)); \
    __res; )}

#define __SVC1(number, arg1) ({ \
    register uint32_t __p0  asm("r12") = number; \
    register uint32_t __p1  asm("r0")  = arg1; \
    register uint32_t __res asm("r0"); \
    asm volatile("svc %1" : "=r" (__res) : "I" (number), "r" (__p0), "r" (__p1)); \
    __res; })

#define __SVC2(number, arg1, arg2) ({ \
    register uint32_t __p0  asm("r12") = number; \
    register uint32_t __p1  asm("r0")  = arg1; \
    register uint32_t __p2  asm("r1")  = arg2; \
    register uint32_t __res asm("r0"); \
    asm volatile("svc %1" : "=r" (__res) : "I" (number), "r" (__p0), "r" (__p1), "r" (__p2)); \
    __res; })

#define __SVC3(number, arg1, arg2, arg3) ({ \
    register uint32_t __p0  asm("r12") = number; \
    register uint32_t __p1  asm("r0")  = arg1; \
    register uint32_t __p2  asm("r1")  = arg2; \
    register uint32_t __p3  asm("r2")  = arg3; \
    register uint32_t __res asm("r0"); \
    asm volatile("svc %1" : "=r" (__res) : "I" (number), "r" (__p0), "r" (__p1), "r" (__p2), "r" (__p3)); \
    __res; })

#define __SVC4(number, arg1, arg2, arg3, arg4) ({ \
    register uint32_t __p0  asm("r12") = number; \
    register uint32_t __p1  asm("r0")  = arg1; \
    register uint32_t __p2  asm("r1")  = arg2; \
    register uint32_t __p3  asm("r2")  = arg3; \
    register uint32_t __p4  asm("r3")  = arg4; \
    register uint32_t __res asm("r0"); \
    asm volatile("svc %1" : "=r" (__res) : "I" (number), "r" (__p0), "r" (__p1), "r" (__p2), "r" (__p3), "r" (__p4)); \
    __res; })

#endif/*__SVC_HELPER_H__*/
