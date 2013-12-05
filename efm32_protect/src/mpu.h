#ifndef __MPU_H__
#define __MPU_H__

#define MPU_MAX_USER_REGIONS 4
#define MPU_RBAR(region,addr) ((region)|MPU_RBAR_VALID_Msk|addr)

extern void mpu_init(void);
extern void mpu_debug(void);
extern uint8_t mpu_region_bits(uint32_t size);
extern int mpu_set(uint8_t region, void* base, uint32_t size, uint32_t flags);

#endif/*__MPU_H__*/

