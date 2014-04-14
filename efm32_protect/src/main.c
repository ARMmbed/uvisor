#include <iot-os.h>
#include "mpu.h"
#include "svc.h"
#include "debug.h"

void default_handler(void)
{
    DEBUG_TxByte('D');
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

    /* switch stack to unprivileged */
    __set_PSP(RAM_MEM_BASE+SRAM_SIZE);

    __set_CONTROL(__get_CONTROL()|3);
    __ISB();
    __DSB();

    __SVC0(1);
    __SVC0(2);
    __SVC1(3,0x11111111);
    __SVC2(4,0x11111111,0x22222222);
    __SVC3(5,0x11111111,0x22222222,0x33333333);
    __SVC4(6,0x11111111,0x22222222,0x33333333,0x44444444);

    dprintf("Hello World 0x%08X\r\n", __SVC1(7, 0xBEEFCAFE));

    while(1);
}
