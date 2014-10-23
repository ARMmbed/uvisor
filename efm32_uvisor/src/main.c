#include <iot-os.h>
#include "mpu.h"
#include "svc.h"
#include "config_efm32.h"

void halt_error(const char* msg)
{
    dprintf("\nERROR: %s\n", msg);
    while(1);
}

void default_handler(void)
{
    halt_error("Spurious IRQ");
}

/* thunk function for context switch */
uint32_t thunk(uint32_t res)
{
    return CONTEXT_SWITCH_OUT(res);
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
}

static void update_client_acl(void)
{
    int res;

    /* allow peripheral access for selected peripherals
     *
     * FIXME: read dynamically from signed ACL provided along with
     *        client box
     */
    res  = mpu_acl_set(CMU,     sizeof(*CMU), 0, MPU_RASR_AP_PRW_URW|MPU_RASR_XN);
    res |= mpu_acl_set(UART1, sizeof(*UART1), 0, MPU_RASR_AP_PRW_URW|MPU_RASR_XN);
    res |= mpu_acl_set(GPIO,   sizeof(*GPIO), 0, MPU_RASR_AP_PRW_URW|MPU_RASR_XN);
    res |= mpu_acl_set((void *) ((uint32_t) DEVINFO - (uint32_t) 0xB0), 0x100, 0, MPU_RASR_AP_PRW_URW|MPU_RASR_XN);

    /* should never happen */
    if(res)
        halt_error("MPU client box configuration error");

    mpu_acl_debug();
}

void main_entry(void)
{
    uint32_t t;

    /* initialize hardware */
    hardware_init();

    /* check correct firmware relocation */
    mpu_check_fw_reloc(CLIENT_BOX);

    /* switch stack to unprivileged client box */
    __set_PSP((uint32_t) g_box_stack[CLIENT_BOX]);

    /* setting up peripheral access ACL for client box */
    update_client_acl();

    /* dump latest MPU settings */
#ifdef  DEBUG
    mpu_debug();
#endif /*DEBUG*/

    dprintf("uVisor switching to unprivileged mode\n");

    /* temporary var needed as BOX_ENTRY macro is not available
     * in unprivileged code */
    t = (uint32_t) BOX_ENTRY(CLIENT_BOX);

    /* switch to unprivileged mode
     * this is possible as uvisor code is readable by unprivileged
     * code and only the key-value database is protected from the
     * unprivileged mode */
    __set_CONTROL(__get_CONTROL()|3);
    __ISB();
    __DSB();

    /* run unprivileged box */
    (*((UnprivilegedBoxEntry*)(t)))();

    /* should never happen */
    while(1)
        __WFI();
}
