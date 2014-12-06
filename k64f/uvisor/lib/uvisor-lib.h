#ifndef __UVISOR_LIB_H__
#define __UVISOR_LIB_H__

#ifdef __cplusplus
#define UVISOR_EXTERN extern "C"
#else
#define UVISOR_EXTERN extern
#endif

#define UVISOR_BOX_MAGIC 0x42CFB66FUL
#define UVISOR_BOX_VERSION 100

#define UVISOR_PACKED __attribute__((packed))
#define UVISOR_SET_MODE(mode) UVISOR_EXTERN const uint32_t __uvisor_mode = (mode);
#define UVISOR_ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))
#define UVISOR_ROUND32(x) (((x)+31UL) & ~0x1FUL)
#define UVISOR_PAD32(x) (32 - (sizeof(x) & ~0x1FUL))
#define UVISOR_STACK_BAND_SIZE 128
#define UVISOR_STACK_SIZE_ROUND(size) (UVISOR_ROUND32(size)+(UVISOR_STACK_BAND_SIZE*2))

#ifndef UVISOR_BOX_STACK_SIZE
#define UVISOR_BOX_STACK_SIZE 1024
#endif/*UVISOR_BOX_STACK*/

#define UVISOR_SECURE(config_type) \
    __attribute__((section(".text.secured"), aligned(32))) \
    static const volatile struct { \
        config_type secure; \
        uint8_t padding[UVISOR_PAD32(config_type)]; \
    } UVISOR_PACKED

#define UVISOR_BOX_CONFIG(acl_list, stack_size) \
    \
    UVISOR_EXTERN uint8_t __attribute__((section(".uvisor.stack"), aligned(32))) \
      acl_list ## _stack[UVISOR_STACK_SIZE_ROUND(stack_size)];\
    \
    static const UvBoxConfig acl_list ## _cfg = { \
        UVISOR_BOX_MAGIC, \
        UVISOR_BOX_VERSION, \
        sizeof(acl_list ## _stack), \
        acl_list, \
        UVISOR_ARRAY_COUNT(acl_list) \
    }; \
    \
    UVISOR_EXTERN const __attribute__((section(".uvisor.cfgtbl"), aligned(4))) \
    void* const acl_list ## _cfg_ptr  =  & acl_list ## _cfg;

typedef uint32_t UvBoxAcl;

typedef struct
{
    const volatile void* start;
    uint32_t length;
    UvBoxAcl acl;
} UVISOR_PACKED UvBoxAclItem;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t stack_size;

    const UvBoxAclItem* const acl_list;
    uint32_t acl_count;
} UVISOR_PACKED UvBoxConfig;

#endif/*__UVISOR_LIB_H__*/
