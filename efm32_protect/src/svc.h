#ifndef __SVC_H__
#define __SVC_H__

#define CBAPI_GET_VERSION 0

extern void svc_init(void);

/* issue external declarations */
#define CRYPTOBOX(id, func_name, ...) extern void func_name(__VA_ARGS__);

#endif/*__SVC_H__*/

/* SVC API declaration */
#ifdef  SVC_SERVICE

/* list of provided SVC API services */
CRYPTOBOX( 0x01, cb_write_data, char *string, int length );

/* re-trigger SVC_SERVICE delcarations */
#undef SVC_SERVICE
#endif/*SVC_SERVICE*/
