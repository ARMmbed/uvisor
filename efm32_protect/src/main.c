#include <iot-os.h>
#include "mpu.h"
#include "svc.h"
#include "debug.h"

void default_handler(void)
{
    DEBUG_TxByte('D');

//    dprintf("IPSR=0x%03X\r\n",__get_IPSR());
}

static inline void hardware_init(void)
{
    /* Enable clocks for peripherals */
    CMU->CTRL |= (CMU_CTRL_HFLE | CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ);
    CMU->OSCENCMD =  CMU_OSCENCMD_HFXOEN;
    CMU->HFCORECLKDIV = CMU_HFCORECLKDIV_HFCORECLKLEDIV;
    CMU->HFPERCLKDIV = CMU_HFPERCLKDIV_HFPERCLKEN;
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

    /* Set calibration for 48MHz crystal */
    CMU->HFRCOCTRL = CMU_HFRCOCTRL_BAND_28MHZ |
        ((DEVINFO->HFRCOCAL1 & _DEVINFO_HFRCOCAL1_BAND28_MASK)
        >> _DEVINFO_AUXHFRCOCAL1_BAND28_SHIFT);
    while(!(CMU->STATUS & CMU_STATUS_HFRCORDY));
    MSC->READCTRL = (MSC->READCTRL & ~_MSC_READCTRL_MODE_MASK)|MSC_READCTRL_MODE_WS2;
    CMU->CMD = CMU_CMD_HFCLKSEL_HFXO;

    /* Enable output */
    DEBUG_init();

    /* register SVC call interface */
    svc_init();

    /* setup MPU */
    mpu_init();
}

void debug_stack(const char* msg)
{
    uint32_t sp;
    __asm__  volatile ("mov %0, sp" : "=g" (sp) : );

    dprintf("\n\r%s:\n\r", msg);
    dprintf("\tSP  : 0x%08X\r\n", sp);
    dprintf("\tPSP : 0x%08X\r\n", __get_PSP());
    dprintf("\tMSP : 0x%08X\r\n", __get_MSP());
}

#define GLOBAL_STACK 0x40
uint32_t g_test_stack[GLOBAL_STACK];

void main_entry(void)
{
    /* initialize hardware */
    hardware_init();

    /* configure MPU */

    mpu_set(7,
        (void*)RAM_MEM_BASE,
        SECURE_RAM_SIZE,
        MPU_RASR_AP_PRW_UNO|MPU_RASR_CB_WB_WRA|MPU_RASR_XN
    );

    mpu_set(6,
        (void*)FLASH_MEM_BASE,
        FLASH_SIZE,
        MPU_RASR_AP_PRO_URO|MPU_RASR_CB_WT
    );

    mpu_set(5,
        DEBUG_USART,
        1024,
        MPU_RASR_AP_PRW_URW
    );

    mpu_set(4,
        (void*)RAM_MEM_BASE,
        SRAM_SIZE,
        MPU_RASR_AP_PRW_URW|MPU_RASR_CB_WB_WRA|MPU_RASR_XN
    );

    MPU->CTRL = MPU_CTRL_ENABLE_Msk|MPU_CTRL_PRIVDEFENA_Msk;
    mpu_debug();

    debug_stack("PRE_CONTROL");

    /* switch stack to unprivileged */
    __set_PSP(RAM_MEM_BASE+SRAM_SIZE);

    debug_stack("POST_SETUP");

    memset(&g_test_stack, 0, sizeof(g_test_stack));

    __set_CONTROL(__get_CONTROL()|3);
    __ISB();
    __DSB();

    debug_stack("POST_CONTROL");

    SVC(1);
    SVC(2);
    cb_write_data((char*)0xDEADEEF,0x12345678);

    dprintf("Hello World\r\n");

    while(1);
}
