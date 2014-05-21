#include <iot-os.h>
#include "mpu.h"
#include "svc.h"

void halt_error(const char* msg)
{
    dprintf("\nERROR: %s\n", msg);
    while(1);
}

void default_handler(void)
{
    halt_error("Spurious IRQ");
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

    /*Initialize SWO debug settings */
#ifdef  CHANNEL_DEBUG
    /* Enable SWO pin */
    GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC0 | GPIO_ROUTE_SWOPEN;
    /* enable push-pull for SWO pin F2 */
    GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK);
    GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL;

    /* Enable debug clock AUXHFRCO */
    CMU->OSCENCMD = CMU_OSCENCMD_AUXHFRCOEN;
    while(!(CMU->STATUS & CMU_STATUS_AUXHFRCORDY));

    /* reset previous channel settings */
    ITM->LAR  = 0xC5ACCE55;
    ITM->TCR  = ITM->TER = 0x0;
    /* wait for debugger to connect */
    while(!((ITM->TCR & ITM_TCR_ITMENA_Msk) && (ITM->TER & (1<<CHANNEL_DEBUG))));
#endif/*CHANNEL_DEBUG*/

    /* register SVC call interface */
    svc_init();

    /* setup MPU */
    mpu_init();
}

static void secure_uvisor(void)
{
    uint32_t stack_start;

    /* calculate stack start according to linker file */
    stack_start = ((uint32_t)&__stack_start__) - RAM_MEM_BASE;
    /* round stack start to next stack guard band size */
    stack_start = (stack_start+(STACK_GUARD_BAND_SIZE-1)) & ~(STACK_GUARD_BAND_SIZE-1);
    /* sanity check of stack size */
    if((stack_start>=STACK_POINTER) || ((STACK_POINTER-stack_start)<STACK_MINIMUM_SIZE))
        halt_error("Invalid Stack size");

    /* configure MPU */

    dprintf("STACK_POINTER=0x%X stack_start=0x%X MSP=0x%X\n",STACK_POINTER,stack_start,__get_MSP());
    /* calculate guard bands */
    //t = (1<<(stack_start/STACK_GUARD_BAND_SIZE)) | 0x80;
    /* protect uvisor RAM, punch holes for guard bands above the stack
     * and below the stack */
    mpu_set(7,
        (void*)RAM_MEM_BASE,
        UVISOR_SRAM_SIZE,
        MPU_RASR_AP_PRW_UNO|MPU_RASR_CB_WB_WRA|MPU_RASR_XN // |MPU_RASR_SRD(t)
    );

    /* set uvisor RAM guard bands to RO & inaccessible to all code */
    mpu_set(6,
        (void*)RAM_MEM_BASE,
        UVISOR_SRAM_SIZE,
        MPU_RASR_AP_PNO_UNO|MPU_RASR_CB_WB_WRA|MPU_RASR_XN
    );

    /* protect uvisor flash from unprivileged code */
    mpu_set(5,
        (void*)FLASH_MEM_BASE,
        UVISOR_FLASH_SIZE,
        MPU_RASR_AP_PRO_UNO|MPU_RASR_CB_WT
    );

    /* allow access to full flash */
    mpu_set(4,
        (void*)FLASH_MEM_BASE,
        FLASH_SIZE,
        MPU_RASR_AP_PRO_URO|MPU_RASR_CB_WT
    );

    /* allow access to unprivileged user SRAM */
    mpu_set(3,
        (void*)RAM_MEM_BASE,
        SRAM_SIZE,
        MPU_RASR_AP_PRW_URW|MPU_RASR_CB_WB_WRA
    );

//    MPU->CTRL = MPU_CTRL_ENABLE_Msk|MPU_CTRL_PRIVDEFENA_Msk;

    /* dump MPU settings */
    mpu_debug();
}

void main_entry(void)
{
    /* initialize hardware */
    hardware_init();

    dprintf("HALT\n");
    while(1);

    /* secure uvisor memories */
    secure_uvisor();

    dprintf("HALT\n");
    while(1);

    /* switch stack to unprivileged */
    __set_PSP(RAM_MEM_BASE+SRAM_SIZE);

    __set_CONTROL(__get_CONTROL()|3);
    __ISB();
    __DSB();

    while(1);
}
