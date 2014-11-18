#ifndef __VMPU_H__
#define __VMPU_H__

/* allow to override default MPU regions */
#ifndef VMPU_REGIONS
#define VMPU_REGIONS 32
#endif/*VMPU_REGIONS*/

typedef unsigned int TACL;

#define TACL_READ    0x01
#define TACL_WRITE   0x02
#define TACL_EXECUTE 0x04

extern void vmpu_init(void);

extern int vmpu_acl_dev(TACL acl, uint16_t device_id);
extern int vmpu_acl_mem(TACL acl, uint32_t addr, uint32_t size);
extern int vmpu_acl_reg(TACL acl, uint32_t addr, uint32_t rmask, uint32_t wmask);
extern int vmpu_acl_bit(TACL acl, uint32_t addr);

extern int vmpu_wr(uint32_t addr, uint32_t data);
extern int vmpu_rd(uint32_t addr);

#endif/*__VMPU_H__*/
