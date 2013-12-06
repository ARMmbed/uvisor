#include <iot-os.h>
#include "debug.h"

#define ALIGNx100  __attribute__((aligned(0x100)))
#define DMA_SNIFF_CTRL DMA_CTRL_DST_INC_BYTE|\
        DMA_CTRL_DST_SIZE_BYTE|\
        DMA_CTRL_DST_PROT_NON_PRIVILEGED|\
        DMA_CTRL_SRC_INC_NONE|\
        DMA_CTRL_SRC_SIZE_BYTE|\
        DMA_CTRL_SRC_PROT_NON_PRIVILEGED|\
        DMA_CTRL_R_POWER_1|\
        DMA_CTRL_CYCLE_CTRL_BASIC|\
        ((DMA_BUFFER_SIZE-1)<<_DMA_CTRL_N_MINUS_1_SHIFT);

typedef DMA_DESCRIPTOR_TypeDef TDMAmap[DMA_CHAN_COUNT];

typedef struct {
    TDMAmap pri ALIGNx100;
    TDMAmap alt ALIGNx100;
} TDMActrl ALIGNx100;

static TDMActrl g_dma_ctrl;
static uint8_t g_buffer_pri[DMA_BUFFER_SIZE];
static uint8_t g_buffer_alt[DMA_BUFFER_SIZE];

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

    /* start USART1 & DMA */
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART1|CMU_HFCORECLKEN0_DMA;

    /* setup DMA */
    DMA->CTRLBASE = (uint32_t)&g_dma_ctrl;
    DMA->CH[SPI_SNIFF_DMA_CH].CTRL =
        DMA_CH_CTRL_SOURCESEL_USART1|
        DMA_CH_CTRL_SIGSEL_USART1RXDATAV;

    /* setup DMA channel SPI_SNIFF_DMA_CH */
    ch = &g_dma_ctrl.pri[SPI_SNIFF_DMA_CH];
    ch->SRCEND = (void*)&USART1->RXDATA;
    ch->DSTEND = &g_buffer_pri;
    ch->CTRL = DMA_SNIFF_CTRL;

    ch = &g_dma_ctrl.alt[SPI_SNIFF_DMA_CH];
    ch->SRCEND = (void*)&USART1->RXDATA;
    ch->DSTEND = &g_buffer_alt;
    ch->CTRL = DMA_SNIFF_CTRL;

    /* start DMA */
    DMA->CONFIG = DMA_CONFIG_EN;
    DMA->CHUSEBURSTS = 1<<SPI_SNIFF_DMA_CH;
    DMA->CHREQMASKC = 1<<SPI_SNIFF_DMA_CH;
    DMA->CHALTC = 1<<SPI_SNIFF_DMA_CH;
    DMA->CHENS = 1<<SPI_SNIFF_DMA_CH;
}

static inline void hardware_init(void)
{
    /* Enable clocks for peripherals */
    CMU->HFPERCLKDIV = CMU_HFPERCLKDIV_HFPERCLKEN;
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

    /* Set calibration for 28MHz band crystal */
    CMU->HFRCOCTRL = CMU_HFRCOCTRL_BAND_28MHZ |
        ((DEVINFO->HFRCOCAL1 & _DEVINFO_HFRCOCAL1_BAND28_MASK)
        >> _DEVINFO_AUXHFRCOCAL1_BAND28_SHIFT);
    while(!(CMU->STATUS & CMU_STATUS_HFRCORDY));

    /* Enable output */
    DEBUG_init();

    /* Enable sniffer */
    sniff_init();
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
