#include <iot-os.h>
#include "CThunk.h"
#include <config_efm32.h>

class CTest
{
public:
    CTest(void);
    void callback1(void);
    void callback2(void* context);
    void callback3(void* context);
    static void callback4(uint32_t r0, uint32_t r1);

    uint32_t counter;

    CThunk<CTest> thunk;
};

CTest::CTest(void)
    :thunk(this, &CTest::callback1)
{
    counter = 0;
}

void CTest::callback1(void)
{
    dprintf("callback1 called (this=0x%0X)\n", this);

    /* increment member variable */
    counter++;
}

void CTest::callback2(void* context)
{
    dprintf("Called with context value 0x%08X\n", context);

    /* increment member variable */
    counter+=2;
}

void hexdump(const void* data, int length)
{
    int i;

    dprintf("Dump %u bytes from 0x%08X\n", length, data);

    for(i=0; i<length; i++)
    {
        if((i%16) == 0)
            dprintf("\n");
        else
            if((i%8) == 0)
                dprintf(" - ");
        dprintf("0x%02X ", ((uint8_t*)data)[i]);
    }
    dprintf("\n");
}

static void mpu_fault_usage(void)
{
	while(1);
}

static void mpu_fault_memory_management(void)
{
	while(1);
}

static void mpu_fault_bus(void)
{
	while(1);
}

static void mpu_fault_hard(void)
{
	while(1);
}

static void mpu_fault_debug(void)
{
	while(1);
}

void halt_error(const char* msg)
{
	dprintf("\nERROR: %s\n", msg);
	while(1);
}

void default_handler(void)
{
	halt_error("Spurious IRQ");
}

static inline void led_set(uint8_t led)
{
	GPIO->P[DEBUG_PORT].DOUTSET = led;
}

static inline void led_clr(uint8_t led)
{
	GPIO->P[DEBUG_PORT].DOUTCLR = led;
}

static inline void hardware_init(void)
{
	/* Enable clocks for peripherals */
	CMU->HFPERCLKDIV = CMU_HFPERCLKDIV_HFPERCLKEN;
	CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

	CONFIG_DebugGpioSetup();
	led_clr(DEBUG_LED0|DEBUG_LED1);

	/* Initialize SWO debug settings */
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
//	while(!((ITM->TCR & ITM_TCR_ITMENA_Msk) && (ITM->TER & (1<<CHANNEL_DEBUG))));

	/* setup security "bluescreen" */
	ISR_SET(BusFault_IRQn,         &mpu_fault_bus);
	ISR_SET(UsageFault_IRQn,       &mpu_fault_usage);
	ISR_SET(MemoryManagement_IRQn, &mpu_fault_memory_management);
	ISR_SET(HardFault_IRQn,        &mpu_fault_hard);
	ISR_SET(DebugMonitor_IRQn,     &mpu_fault_debug);

	/* enable mem, bus and usage faults */
	SCB->SHCSR |= 0x70000;
}

static void test(void)
{
    CThunkEntry entry;
    CTest test;

    /* get 32 bit entry point pointer from thunk */
    entry = test.thunk;
    /* TEST1: */

    /* callback function has been set in the CTest constructor */
    hexdump((const void*)entry, 16);
    /* call entry point */

    dprintf("before entry 1 (counter=%i)\n", test.counter);
    led_set(DEBUG_LED0);
    entry();
    led_clr(DEBUG_LED0);
    dprintf("after entry 1  (counter=%i)\n", test.counter);

    /* TEST2: */

    /* assign a context ... */
    test.thunk.context(0xDEADBEEF);
    /* and switch callback to callback2 */
    test.thunk.callback(&CTest::callback2);

    /* call entry point */
    dprintf("before entry 2 (counter=%i)\n", test.counter);
	led_set(DEBUG_LED1);
    entry();
	led_clr(DEBUG_LED1);
    dprintf("after entry 2  (counter=%i)\n", test.counter);
}

void main_entry(void)
{
    hardware_init();

    dprintf("Test 1\n");
    /* run tests */
    test();

    /* turn both LED's on */
	led_set(DEBUG_LED0|DEBUG_LED1);
    while(1)
        __WFI();
}
