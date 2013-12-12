#include <iot-os.h>
#include "debug.h"

#define DMA_SNIFF_CTRL \
        DMA_CTRL_DST_INC_BYTE|\
        DMA_CTRL_DST_SIZE_BYTE|\
        DMA_CTRL_DST_PROT_PRIVILEGED|\
        DMA_CTRL_SRC_INC_NONE|\
        DMA_CTRL_SRC_SIZE_BYTE|\
        DMA_CTRL_SRC_PROT_PRIVILEGED|\
        DMA_CTRL_R_POWER_1|\
        DMA_CTRL_CYCLE_CTRL_PINGPONG|\
        ((DMA_BUFFER_SIZE-1)<<_DMA_CTRL_N_MINUS_1_SHIFT);

#define SPISCREEN_WIDTH 128
#define SPISCREEN_BUFSIZE (SPISCREEN_WIDTH+2)

typedef struct {
    DMA_DESCRIPTOR_TypeDef pri[16];
    DMA_DESCRIPTOR_TypeDef alt[DMA_CHAN_COUNT];
} TDMActrl  __attribute__((aligned(0x100)));

static TDMActrl g_dma_ctrl;
static uint8_t g_pri_alt;
static volatile uint8_t g_screen_row, g_screen_col, g_buffer_pos;
static uint8_t g_buffer_line[SPISCREEN_WIDTH+2];
static uint8_t g_buffer_pri[DMA_BUFFER_SIZE];
static uint8_t g_buffer_alt[DMA_BUFFER_SIZE];

static void sniff_process_line(const uint8_t* buffer)
{
}

static void sniff_process_buffer(const uint8_t* buffer, uint16_t size)
{
    uint8_t t,count;

    if(g_buffer_pos)
    {
        /* get missing byte count */
        t = SPISCREEN_BUFSIZE-g_buffer_pos;

        /* calculate remaining bytes in transaction */
        count = (size>t)?t:size;

        /* assemble into single buffer */
        memcpy(&g_buffer_line[g_buffer_pos], buffer, count);
        g_buffer_pos+=count;

        /* line not complete yet */
        if(g_buffer_pos<SPISCREEN_BUFSIZE)
            return;

        /* remove data from buffer */
        buffer+=count;
        size-=count;

        /* process finished line */
        sniff_process_line(g_buffer_line);
        g_buffer_pos = 0;
    }

    /* send down full lines for processing */
    while(size>=SPISCREEN_BUFSIZE)
    {
        sniff_process_line(buffer);

        buffer+=SPISCREEN_BUFSIZE;
        size-=SPISCREEN_BUFSIZE;
    }

    /* remember remaining data */
    if(size)
    {
        memcpy(g_buffer_line, buffer, size);
        g_buffer_pos = size;
    }
}

static void sniff_dma(void)
{
    DMA_DESCRIPTOR_TypeDef *ch;
    uint8_t pri;
    uint32_t reason = DMA->IF;

    ch = g_pri_alt ?
        &g_dma_ctrl.alt[SPI_SNIFF_DMA_CH]:
        &g_dma_ctrl.pri[SPI_SNIFF_DMA_CH];
    g_pri_alt ^= 1;

    /* process buffer */
    sniff_process_buffer((uint8_t*)ch->USER, DMA_BUFFER_SIZE);

    /* re-activate channel */
    ch->CTRL = DMA_SNIFF_CTRL;

    DMA->IFC = reason;
}

static inline void sniff_init(void)
{
    DMA_DESCRIPTOR_TypeDef *ch;

    /* reset variables */
    g_pri_alt = 0;
    g_screen_row = g_screen_col = g_buffer_pos = 0;

    /* Avoid false start by setting outputs as high */
    GPIO->P[3].DOUTCLR = 0xD;
    GPIO->P[3].MODEL =
        GPIO_P_MODEL_MODE0_INPUTPULL|
        GPIO_P_MODEL_MODE2_INPUTPULL|
        GPIO_P_MODEL_MODE3_INPUTPULL;

    /* start USART1 */
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART1;

    USART1->ROUTE =
        USART_ROUTE_RXPEN |
        USART_ROUTE_CSPEN |
        USART_ROUTE_CLKPEN|
        USART_ROUTE_LOCATION_LOC1;
    USART1->CTRL = USART_CTRL_SYNC | USART_CTRL_CSINV;
    USART1->FRAME = USART_FRAME_DATABITS_EIGHT;
    USART1->CMD =
        USART_CMD_MASTERDIS |
        USART_CMD_RXEN |
        USART_CMD_TXTRIEN;

    /* setup DMA */
    DMA->CH[SPI_SNIFF_DMA_CH].CTRL =
        DMA_CH_CTRL_SOURCESEL_USART1|
        DMA_CH_CTRL_SIGSEL_USART1RXDATAV;

    /* setup primary DMA channel SPI_SNIFF_DMA_CH */
    ch = &g_dma_ctrl.pri[SPI_SNIFF_DMA_CH];
    ch->SRCEND = (void*)&USART1->RXDATA;
    ch->DSTEND = &g_buffer_pri[DMA_BUFFER_SIZE-1];
    ch->CTRL = DMA_SNIFF_CTRL;
    ch->USER = (uint32_t)&g_buffer_pri;

    /* setup alternate DMA channel SPI_SNIFF_DMA_CH */
    ch = &g_dma_ctrl.alt[SPI_SNIFF_DMA_CH];
    ch->SRCEND = (void*)&USART1->RXDATA;
    ch->DSTEND = &g_buffer_alt[DMA_BUFFER_SIZE-1];
    ch->CTRL = DMA_SNIFF_CTRL;
    ch->USER = (uint32_t)&g_buffer_alt;

    /* start DMA */
    DMA->CHUSEBURSTS = 1<<SPI_SNIFF_DMA_CH;
    DMA->CHREQMASKC = 1<<SPI_SNIFF_DMA_CH;
    DMA->CHALTC = 1<<SPI_SNIFF_DMA_CH;
    DMA->CHENS = 1<<SPI_SNIFF_DMA_CH;
    DMA->IEN |= 1<<SPI_SNIFF_DMA_CH;
    DMA->RDS |= 1<<SPI_SNIFF_DMA_CH;
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
    NVIC_EnableIRQ(DMA_IRQn);

    /* Enable sniffer */
    sniff_init();

    /* start DMA */
    DMA->CONFIG = DMA_CONFIG_EN | DMA_CONFIG_CHPROT;
}

void main(void)
{
    uint32_t i;
    volatile uint32_t t;

    /* initialize hardware */
    hardware_init();

    i=0;
    while(true)
    {
        for(t=0;t<1000000;t++);
        dprintf("pos=%03i (%i)\r\n",g_buffer_pos,i++);
    }
}
