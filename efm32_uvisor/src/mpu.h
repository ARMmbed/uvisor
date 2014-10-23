#ifndef __MPU_H__
#define __MPU_H__

/* bottom MPU regions allocated for dynamic ACL */
#define MPU_DYN_REGIONS    3

/* maximum amount of ACLs for dynamic regions */
#define MPU_MAX_ACL     8

#define MPU_RBAR(region,addr)   (((uint32_t)(region))|MPU_RBAR_VALID_Msk|addr)
#define MPU_RBAR_RNR(addr)     (addr)

/* number of sub-regions in a region */
#define MPU_REGION_SPLIT 8

/* sub-region sizes */
#define FLASH_SUBREGION_SIZE    (FLASH_SIZE >> 3)
#define RAM_SUBREGION_SIZE    (SRAM_SIZE >> 3)

/* exception frame dimension (words) */
#define EXC_SF_SIZE    8    // no fp operations

/* FIXME modules/libraries may span over more than one sub-region, hence stack ptrs and entry
 * points should be made module-relative, and not sub-region-relative */
/* code and stack pointers
 * each sub-region has its own
 *     - stack         BOX_STACK(subregion_#)
 *     - entry point        BOX_ENTRY(subregion_#)
 *    - magic value        BOX_CHECK(subregion_#)    to check correct firmware relocation
 *    - description table    BOX_TABLE(subregion_#    except for the uVisor
 */
#define TABLE_OFFSET    ((uint32_t) 0x4)
#define ENTRY_OFFSET    ((uint32_t) 0x8)
#define BOX_CHECK(x)    ((uint32_t *)      (FLASH_MEM_BASE + x * FLASH_SUBREGION_SIZE))
#define BOX_TABLE(x)    ((ExportTable *) *((uint32_t *) (FLASH_MEM_BASE + x * FLASH_SUBREGION_SIZE + TABLE_OFFSET)))
#define BOX_ENTRY(x)    ((uint32_t *)      (FLASH_MEM_BASE + x * FLASH_SUBREGION_SIZE + ENTRY_OFFSET))
#define BOX_STACK(x)    ((uint32_t *)      (RAM_MEM_BASE + (x + 1) * RAM_SUBREGION_SIZE))

/* box stacks are updated at runtime by the uVisor during context switches */
extern uint32_t * g_box_stack[MPU_REGION_SPLIT];

/* stack of active boxes
 * context switches can be nested and a dedicated stack is used to fetch
 * src and dst boxes */
#define ACT_BOX_STACK_SIZE    ((uint32_t *) 0x100)
#define ACT_BOX_STACK_PTR    ((uint32_t *) 0x20002000)
#define ACT_BOX_STACK_MIN    ((uint32_t *) (ACT_BOX_STACK_PTR - ACT_BOX_STACK_SIZE))
#define ACT_BOX_STACK_INIT    ((uint32_t)   0x00010001)
extern uint32_t * g_act_box;

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
extern void mpu_check_permissions(uint32_t *);
extern int mpu_check_act_box_stack(void);
extern void mpu_check_fw_reloc(int);

#endif/*__MPU_H__*/
