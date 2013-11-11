#ifndef __MPU_H__
#define __MPU_H__

#define MPU_MAX_USER_REGIONS 4

#define MPU_RBAR_MASK (~((1<<(MPU_RBAR_VALID_Pos+1))-1))
#define MPU_RBAR(slot,addr) ((slot|MPU_RBAR_VALID_Msk)|(MPU_RBAR_MASK&addr))

#define MPU_RASR(size_bits, )

typedef struct
{
    uint32_t rbar, rasr;
} MPU_Region;

extern void mpu_init(void);

#endif/*__MPU_H__*/

