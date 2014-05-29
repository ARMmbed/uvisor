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

    /* setup MPU & halt on MPU configuration errors */
    if(mpu_init())
        halt_error("MPU configuration error");

    /* dump MPU settings */
#ifdef  DEBUG
    mpu_debug();
#endif/*DEBUG*/
}

void main_entry(void)
{
    uint32_t t;

    /* initialize hardware */
    hardware_init();

    /* FIXME add signature verification of unprivileged uvisor box
     * client. Use stored ACLs to set access privileges for box
     * (allowed peripherals etc.)
     */
    t = *(uint32_t*)(FLASH_MEM_BASE+UVISOR_FLASH_SIZE);
    if(t==0xFFFFFFFF)
        halt_error("please install unprivileged firmware");
    if(t!=(FLASH_MEM_BASE+UVISOR_FLASH_SIZE))
        halt_error("incorrectly relocated unprivileged firmware");

    /* switch stack to unprivileged */
    __set_PSP(RAM_MEM_BASE+SRAM_SIZE);

    dprintf("uVisor switching to unprivileged mode\n");

    /* switch to unprivileged mode - this is possible as uvisor code
     * is readable by unprivileged code and only the key-value database
     * is protected from the unprivileged mode */
    __set_CONTROL(__get_CONTROL()|3);
    __ISB();
    __DSB();

    /* run unprivileged box */
    (*((UnprivilegedBoxEntry*)(FLASH_MEM_BASE+UVISOR_FLASH_SIZE+4)))();

    /* should never happen */
    while(1)
        __WFI();
}
