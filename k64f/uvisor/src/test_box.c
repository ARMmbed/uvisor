#include <uvisor.h>
#include <vmpu.h>

#include "test_box.h"

typedef struct {
    uint32_t value1;
    uint32_t value2;
} TSecretData;

typedef union {
    TSecretData data;
    uint8_t padding[(sizeof(TSecretData)+31) & ~0x1FUL];
} TSecretDataPadded;

//__attribute__ ((section ("protected"), aligned(32)))
static const volatile TSecretDataPadded g_test_config = {{
    0xDEADEEF,
    12345678
}};

uint32_t test_xor(uint32_t a)
{
    return a ^ g_test_config.data.value1;
}

uint32_t test_add(uint32_t a)
{
    return a + g_test_config.data.value2;
}

const void* test_fn_list[] = {
    test_xor,
    test_add
};

const TACL_Entry test_acl_list[] = {
    /* ACL for config block */
    {(void*)&g_test_config, sizeof(g_test_config)},
};

const TBoxDesc test_box = {
    UVISOR_MAGIC,
    STACK_SIZE,

    /* register ACL list */
    test_acl_list,
    ARRAY_COUNT(test_acl_list),

    /* register exported functions */
    test_fn_list,
    ARRAY_COUNT(test_fn_list),
};
