#include <uvisor.h>

/* timer exported functions */
typedef enum {init = 0, start, stop} TimerFunction;

/* timer GUID */
static const Guid timer_guid = {
    0x00, 0x11, 0x22, 0x33,
    0x00, 0x11, 0x22, 0x33,
    0x88, 0x99, 0xAA, 0xBB,
    0x88, 0x99, 0xAA, 0xBB
};

/* timer box number */
static uint32_t g_timer_box;

/* timer library loader */
static void timer_init(void)
{
    if(!(g_timer_box = get_box_num(timer_guid)))
        dprintf("Error. Box not found\n\r");
    return;
}

/* timer library loader (ptr) */
uint32_t __attribute__((section(".box_init"))) g_timer_init = (uint32_t) timer_init;

/* timer functions - start timer */
void timer_start(void)
{
    context_switch_in(start, g_timer_box);
}

/* timer functions - stop timer */
void timer_stop(void)
{
    context_switch_in(stop, g_timer_box);
}

