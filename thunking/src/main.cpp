#include <iot-os.h>
#include "debug.h"
#include "CThunk.h"

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

	/* Enable output */
	DEBUG_init();
}

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
{
	counter = 0;
}

void CTest::callback1(void)
{
	dprintf("callback1 called\n");

    /* increment member variable */
//    counter++;
}

void CTest::callback2(void* context)
{
	dprintf("Called with context value 0x%08X\n", context);

	/* increment member variable */
//    counter = (uint32_t)context;
}

void CTest::callback3(void* context)
{
	dprintf("Called by ticker object 0x%08X: \n", context);
}

void CTest::callback4(uint32_t r0, uint32_t r1)
{
    dprintf("callback4: r0=0x%08X r1=0x%08X\n",r0, r1);
}

void hexdump(const void* data, int length)
{
	int i;

	dprintf("Dump %u bytes from 0x%08Xn", length, data);

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

void main_entry(void)
{
	CThunkEntry entry;
	CTest test;

	/* initialize hardware */
	hardware_init();

	/* get 32 bit entry point pointer from thunk */
	entry = test.thunk;

    /* TEST1: */

	/* assign callback1 to thunk - no context needed */
	test.thunk = (void*)0xDEADBEEF;
	test.thunk = &CTest::callback1;
	hexdump((const void*)entry, CTHUNK_OPCODES_SIZE+3*4);
	/* call entry point */

	dprintf("before entry\n");
	entry();
	dprintf("after entry\n");

	/* TEST2: */

	/* assign a context ... */
	test.thunk = 0xDEADBEEF;
	/* and switch callback to callback2 */
	test.thunk = &CTest::callback2;
	/* call entry point */
	entry();

	while(1)
		__WFI();
}
