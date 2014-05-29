#ifndef __MPU_H__
#define __MPU_H__

/* bottom MPU regions allocated for dynamic ACL */
#define MPU_DYNAMIC_REGIONS 3
/* maximum amount of ACLs for dynamic regions */
#define MPU_MAX_ACL 8

#define MPU_RBAR(region,addr) (((uint32_t)(region))|MPU_RBAR_VALID_Msk|addr)

#define MPU_REGION_SPLIT 8

#define MPU_RASR_AP_PNO_UNO (0x00UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRW_UNO (0x01UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRW_URO (0x02UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRW_URW (0x03UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRO_UNO (0x05UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_AP_PRO_URO (0x06UL<<MPU_RASR_AP_Pos)
#define MPU_RASR_XN         (0x01UL<<MPU_RASR_XN_Pos)
#define MPU_RASR_CB_NOCACHE (0x00UL<<MPU_RASR_B_Pos)
#define MPU_RASR_CB_WB_WRA  (0x01UL<<MPU_RASR_B_Pos)
#define MPU_RASR_CB_WT      (0x02UL<<MPU_RASR_B_Pos)
#define MPU_RASR_CB_WB      (0x03UL<<MPU_RASR_B_Pos)
#define MPU_RASR_SRD(x)     (((uint32_t)(x))<<MPU_RASR_SRD_Pos)

extern int mpu_init(void);
extern void mpu_debug(void);
extern uint8_t mpu_region_bits(uint32_t size);
extern int mpu_set(uint8_t region, void* base, uint32_t size, uint32_t flags);
extern int mpu_acl_set(void* base, uint32_t size, uint8_t priority, uint32_t flags);
extern void mpu_acl_debug(void);

#endif/*__MPU_H__*/

