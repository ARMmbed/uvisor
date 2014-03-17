#ifndef __SVC_H__
#define __SVC_H__

/* wrapper for SVC call */
#define SVC(number) asm volatile("svc %0"::"I"(number))

extern void svc_init(void);

/* issue external declarations */
#define SVC_SERVICE(id, func_name, param...) extern void func_name(param);

#endif/*__SVC_H__*/

/* SVC API declaration */
#ifdef  SVC_SERVICE

/* list of provided SVC API services */
SVC_SERVICE( 0x01, cb_write_data, char *string, int length );

/* re-trigger SVC_SERVICE delcarations */
#undef SVC_SERVICE
#endif/*SVC_SERVICE*/
