#ifndef __SVC_H__
#define __SVC_H__

/* wrapper for SVC call */
#define SVC(number) asm volatile("svc %0"::"I"(number))

extern void svc_init(void);

#endif/*__SVC_H__*/
