#include <uvisor.h>
#include "mpu.h"
#include "svc.h"
#include "config_efm32.h"

void halt_error(const char* msg)
{
    dprintf("\nERROR: %s\n", msg);
    while(1);
}

/* table of unprivileged-registered isr */
TIsrVector g_isr_vector_unpriv[MAX_ISR_VECTORS];

/* default unprivileged-registered interrupts handler */
void static __attribute__((naked)) hdlr_unpriv(uint32_t hdlr)
{
    asm volatile(
        "push    {r4, lr}\n"
        "mov    r12, #3\n"    // SVC# is in LSByte of r12
        "svc    3\n"        // de-privilege via NONBASETHRDENA
        "blx    r0\n"        // unprivileged handler
        "mov    r12, #4\n"    // SVC# is in LSByte of r12
        "svc    4\n"        // re-privilege via NONBASETHRDENA
        "pop    {r4, pc}\n"
    );
}

/* default handler for unprivilege-registered interrupts */
void redirect_handler(void)
{
    uint32_t ipsr, irqn;
    TIsrVector hdlr;

    /* IPSR register */
    asm volatile(
        "mrs    %0, IPSR\n"
        : "=r" (ipsr)
    );

    /* ISR number */
    irqn = ipsr & 0x3F;

    /* if the unprivileged-registered ISR table has the entry
     * for the requested ISR */
    if((hdlr = g_isr_vector_unpriv[irqn]))
    {
        /* handle it with unprivileged handler */
        hdlr_unpriv((uint32_t) hdlr);
        return;
    }
    /* else halt */
    else
        halt_error("Spurious IRQ");
}

/* default handler for unregistered interrupts */
void default_handler(void)
{
    halt_error("Spurious IRQ");
}

/* thunk function for context switch
 *     a function call that crosses a secure boundary
 *     is converted into a context switch that triggers
 *     MPU re-configuration.
 *     On function return configurations are reverted
 *     through this routine */
uint32_t thunk(uint32_t res)
{
    return context_switch_out(res);
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
    res  = mpu_acl_set(CMU,       sizeof(*CMU), 0, MPU_RASR_AP_PRW_URW|MPU_RASR_XN);
    res |= mpu_acl_set(UART1,   sizeof(*UART1), 0, MPU_RASR_AP_PRW_URW|MPU_RASR_XN);
    res |= mpu_acl_set(GPIO,     sizeof(*GPIO), 0, MPU_RASR_AP_PRW_URW|MPU_RASR_XN);
    res |= mpu_acl_set(TIMER1, sizeof(*TIMER1), 0, MPU_RASR_AP_PRW_URW|MPU_RASR_XN);
    res |= mpu_acl_set((void *) ((uint32_t) DEVINFO - (uint32_t) 0xB0), 0x100, 0, MPU_RASR_AP_PRW_URW|MPU_RASR_XN);

    /* should never happen */
    if(res)
        halt_error("MPU client box configuration error");

    mpu_acl_debug();
}

/* look for box header tables in flash and load their information
 * in a global structure */
uint32_t load_boxes(void)
{
    uint32_t box_num, span;
    uint32_t found_box_mask = 0;
    HeaderTable *hdr_tbl;

    /* initialize stack of active boxes (by default the
     * client box only is active) */
    *(g_act_box = ACT_BOX_STACK_PTR - 1) = ACT_BOX_STACK_INIT;

    /* uVisor box information */
      //g_boxes[0].guid                // n.a.
    g_boxes[0].span = 1;            // uVisor spans in 1 subregion
    g_boxes[0].sp = BOX_STACK(0);        // uVisor sp

    for(box_num = 1; box_num < MPU_REGION_SPLIT; box_num++)
    {
        /* default location of expected header table */
        hdr_tbl = BOX_TABLE(box_num);

        /* check module relocation through magic value */
        if(hdr_tbl->magic != BOX_MAGIC_VALUE)
        {
            if(box_num == CLIENT_BOX)
                halt_error("incorrectly relocated unprivileged firmware");
            else
                break;
        }

        /* calculate number of regions spanned by box (from 0) */
        span = hdr_tbl->length / FLASH_SUBREGION_SIZE;

        /* populate box information */
        g_boxes[box_num].span = span + 1;                    // spanned regions (from 1)
        memcpy((void *) g_boxes[box_num].guid, hdr_tbl->guid, sizeof(Guid));    // GUID
        g_boxes[box_num].sp = BOX_STACK(box_num);                // sp

        /* populate mask of found boxes for later initialization */
        found_box_mask |= (1 << box_num);

        /* if module spans over more than one subregion avoid
         * false positives */
        box_num += span;
    }

    return found_box_mask;
}

/* all boxes (except main app) are initialized */
void init_boxes(uint32_t found_box_mask)
{
    int box_num;

    /* initialize boxes (except main app) from mask */
    for(box_num = MPU_REGION_SPLIT - 1; box_num > 1; box_num--)
        if(found_box_mask & (1 << box_num))
            context_switch_in(0, box_num);

}

void main_entry(void)
{
    uint32_t found_box_mask;

    /* initialize hardware */
    hardware_init();

    /* load all boxes */
    found_box_mask = load_boxes();

    /* switch stack to unprivileged client box */
    __set_PSP((uint32_t) g_boxes[CLIENT_BOX].sp);

    /* setting up peripheral access ACL for client box */
    update_client_acl();

    /* dump latest MPU settings */
#ifdef  DEBUG
    mpu_debug();
#endif /*DEBUG*/

    dprintf("uVisor switching to unprivileged mode\n");

    /* switch to unprivileged mode
     * this is possible as uvisor code is readable by unprivileged
     * code and only the key-value database is protected from the
     * unprivileged mode */
    __set_CONTROL(__get_CONTROL()|3);
    __ISB();
    __DSB();

    /* initialize all unprivileged boxes */
    init_boxes(found_box_mask);

    /* run unprivileged box */
    (*((UnprivilegedBoxEntry*)((uint32_t) BOX_ENTRY(CLIENT_BOX))))();

    /* should never happen */
    while(1)
        __WFI();
}
