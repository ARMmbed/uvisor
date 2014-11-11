#include <uvisor.h>

#define TIMER_WAIT     0xFFF
#define LED_PORT     4
#define LED_PIN     2

volatile uint16_t g_counter = 0;

static void __timer_irq(void)
{
    /* clear timer overflow bit */
    TIMER1->IFC = 1;

    /* increment counter */
    g_counter++;
}

void main_entry(void)
{
    dprintf("timer initialized!\n");

    /* enable GPIO and peripheral clock for TIMER1 */
    CMU->HFPERCLKEN0 = (1 << 13) | (1 << 6);
    GPIO->P[LED_PORT].MODEL = 4 << 8;

    /* register for interrupt */
    isr_set_unpriv(TIMER1_IRQn, &__timer_irq);
}

static void timer_start(void)
{
    TIMER1->TOP = TIMER_WAIT;
    TIMER1->IEN = 1;
    TIMER1->CMD = 0x1;

    /* turn led on */
    GPIO->P[LED_PORT].DOUTTGL = 1 << LED_PIN;
}

static void timer_stop(void)
{
    /* turn led off after TIMER_WAIT */
    while(g_counter < TIMER_WAIT);
    GPIO->P[LED_PORT].DOUTTGL = 0 << LED_PIN;
    g_counter = 0;

    /* disable interrupt */
    TIMER1->IEN = 0;
}

/* functions export table */
static void * const g_export_table[] =
{
    reset_handler,        // 0 - initialize
    timer_start,        // 1 - start timer
    timer_stop,        // 1 - stop timer
};

/* header table */
const HeaderTable g_hdr_tbl = {
    0xDEAD,
    0,
    0,
    sizeof(g_export_table) / sizeof(g_export_table[0]),
    (uint32_t *) g_export_table,
    {0x00, 0x11, 0x22, 0x33,\
     0x00, 0x11, 0x22, 0x33,\
     0x88, 0x99, 0xAA, 0xBB,\
     0x88, 0x99, 0xAA, 0xBB},
};
