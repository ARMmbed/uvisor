#include <iot-os.h>
#include "debug.h"

#define DMA_SNIFF_CTRL DMA_CTRL_DST_INC_BYTE|\
        DMA_CTRL_DST_SIZE_BYTE|\
        DMA_CTRL_DST_PROT_NON_PRIVILEGED|\
        DMA_CTRL_SRC_INC_NONE|\
        DMA_CTRL_SRC_SIZE_BYTE|\
        DMA_CTRL_SRC_PROT_NON_PRIVILEGED|\
        DMA_CTRL_R_POWER_1|\
        DMA_CTRL_CYCLE_CTRL_PINGPONG|\
        ((DMA_BUFFER_SIZE-1)<<_DMA_CTRL_N_MINUS_1_SHIFT);

typedef struct {
    DMA_DESCRIPTOR_TypeDef pri[16];
    DMA_DESCRIPTOR_TypeDef alt[DMA_CHAN_COUNT];
} TDMActrl  __attribute__((aligned(0x100)));

static TDMActrl g_dma_ctrl;
static uint8_t g_buffer_pri[DMA_BUFFER_SIZE];
static uint8_t g_buffer_alt[DMA_BUFFER_SIZE];

static void sniff_dma(void)
{
    uint32_t reason = DMA->IF;

    dprintf("DMA->IFC = 0x%08X\r\n", DMA->IFC);

    DMA->IFC = reason;
}

static inline void sniff_init(void)
{
    DMA_DESCRIPTOR_TypeDef *ch;

    /* Avoid false start by setting outputs as high */
    GPIO->P[3].DOUTCLR = 0xD;
    GPIO->P[3].MODEL =
        GPIO_P_MODEL_MODE0_INPUTPULL|
        GPIO_P_MODEL_MODE2_INPUTPULL|
        GPIO_P_MODEL_MODE3_INPUTPULL;
    USART1->ROUTE =
        USART_ROUTE_TXPEN |
        USART_ROUTE_CSPEN |
        USART_ROUTE_CLKPEN|
        USART_ROUTE_LOCATION_LOC1;

    /* start USART1 */
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART1;

    /* setup DMA */
    DMA->CH[SPI_SNIFF_DMA_CH].CTRL =
        DMA_CH_CTRL_SOURCESEL_USART1|
        DMA_CH_CTRL_SIGSEL_USART1RXDATAV;

    /* setup primary DMA channel SPI_SNIFF_DMA_CH */
    ch = &g_dma_ctrl.pri[SPI_SNIFF_DMA_CH];
    ch->SRCEND = (void*)&USART1->RXDATA;
    ch->DSTEND = &g_buffer_pri[DMA_BUFFER_SIZE-1];
    ch->CTRL = DMA_SNIFF_CTRL;

    /* setup alternate DMA channel SPI_SNIFF_DMA_CH */
    ch = &g_dma_ctrl.alt[SPI_SNIFF_DMA_CH];
    ch->SRCEND = (void*)&USART1->RXDATA;
    ch->DSTEND = &g_buffer_alt[DMA_BUFFER_SIZE-1];
    ch->CTRL = DMA_SNIFF_CTRL;

    /* start DMA */
    DMA->CHUSEBURSTS = 1<<SPI_SNIFF_DMA_CH;
    DMA->CHREQMASKC = 1<<SPI_SNIFF_DMA_CH;
    DMA->CHALTC = 1<<SPI_SNIFF_DMA_CH;
    DMA->CHENS = 1<<SPI_SNIFF_DMA_CH;
    DMA->IEN |= 1<<SPI_SNIFF_DMA_CH;
}

#if 0
static inline void vga_init(void)
{
    /* enable time clock */
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_TIMER0|CMU_HFPERCLKEN0_TIMER1;

    TIMER0->ROUTE = TIMER_ROUTE_LOCATION_LOC0
    TIMER0->TOP = SCREEN_HTOTAL;
    TIMER0->CTRL =

    TIMER1->TOP = SCREEN_VTOTAL;
    TIMER1->

}
#endif

static inline void hardware_init(void)
{
    /* Enable clocks for peripherals */
    CMU->HFPERCLKDIV = CMU_HFPERCLKDIV_HFPERCLKEN;
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

    /* DIV4 clock for frequencies above 48MHz */
    CMU->HFCORECLKDIV = CMU_HFCORECLKDIV_HFCORECLKLEDIV;

    /* Set calibration for 28MHz band crystal */
    CMU->HFRCOCTRL = CMU_HFRCOCTRL_BAND_28MHZ |
        ((DEVINFO->HFRCOCAL1 & _DEVINFO_HFRCOCAL1_BAND28_MASK)
        >> _DEVINFO_AUXHFRCOCAL1_BAND28_SHIFT);
    while(!(CMU->STATUS & CMU_STATUS_HFRCORDY));

    /* Enable output */
    DEBUG_init();

    /* start DMA */
    CMU->HFCORECLKEN0 |= CMU_HFCORECLKEN0_DMA;
    DMA->CTRLBASE = (uint32_t)&g_dma_ctrl;
    /* enable DMA irq */
    ISR_SET(DMA_IRQn, &sniff_dma);

    /* Enable sniffer */
    sniff_init();

    /* start DMA */
    DMA->CONFIG = DMA_CONFIG_EN;
}

void main(void)
{
    uint32_t i;

    /* initialize hardware */
    hardware_init();

    i=0;
    while(true)
        dprintf("Hello World 12345 %06d\r\n",i++);
}
