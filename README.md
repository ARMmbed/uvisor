# crypto.box - a Cortex-M TEE Prototype

## Overview
The code in this tree creates two independent security domains on a
Cortex M3 micro controller (M0+ will follow). The target is to increase
resilience against malware and to protect cryptographic secrets from leaking.

### The crypto.box uVisor
* initialized first during device reset
* run in privileged mode
* set up a protected environment using the cortex-M Memory Protection Unit (MPU)
	* protect own memories and critical peripherals from unprivileged code
	* limit unprivileged access to selected hardware peripherals and memories
* allow interaction from unprivileged code by exposing a SVC call based API
* forward and de-privilege interrupts to unprivileged code
	* prevent register leakage of privileged code to unprivileged code
* force access to security-critical peripherals like DMA through the SVC api

### The crypto.box Client
* runs in unprivileged mode
* access to privileged memories and peripherals is prevented by the uVisor
* has direct memory access to unprivileged peripherals
* can register for unprivileged interrupts
* can be made of mutually untrusted components isolated by the uVisor

### Memory Layout
crypto.box code is readable and executable by unprivileged code. The reasons for that are:
* Easy transitions from privileged mode to unprivileged mode: de-privileging happens in crypto.box owned memory segments
* Saves one MPU region - more regions available for dynamic allocation
* Unprivileged code can directly call privileged helper functions without switching privilege levels: allows code-reuse to save flash memory. Helper functions can still decide to switch to privileged mode via the SVC call.
* Unprivileged code can verify the integrity of the privileged uVisor.
* Confidentiality of the Exception/Interrupt table is still guaranteed as it's kept in secured RAM.

![cryptobox MPU memory configuration](https://github.com/ARM-RD/cryptobox/raw/images/efm32_uvisor/docs/memory-map.png "cryptobox MPU memory configuration")

## Setting up the Hardware

* current tree supports the EFM32™ Giant Gecko Starter Kit (EFM32GG-STK3700)
* starter kit comes with an integrated J-Link SWD'er
* change power switch to "DBG" so the STK3700 is powered by the debugger
* you need to plug a mini-usb cable into the "DBG" USB port

## Software Installation

Development should work in Linux and Mac
* Get latest arm-none-eabi-gcc from https://launchpad.net/gcc-arm-embedded
* Download the latest SEGGER J-Link software & documentation pack for your platform

## Development

### Quickstart

Currently the Client application is made of two mutually untrusted modules:
* box - the main application
* xor - an example library to show modules isolation and secure context switch

#### Flash both uvisor and client:
```Bash
# rebuild cryptobox uvisor, erase chip and flash uvisor
make -C efm32_uvisor clean all erase flash
 
# keeping the uVisor, rebuild cryptobox guest application modules, 
# flash them, reset CPU and start SWD terminal with debug output
make -C efm32_timer  clean all flash 
make -C efm32_xor    clean all flash 
make -C efm32_box    clean all flash reset swo
```

#### Example output:
```AsciiDoc
Target CPU is running @ 14000 kHz.
Receiving SWO data @ 900 kHz.
Showing data from stimulus port(s): 0, 1
-----------------------------------------------
MPU region dump:
	R:0 RBAR=0x00000000 RASR=0x00000000
	R:1 RBAR=0x00000001 RASR=0x00000000
	R:2 RBAR=0x00000002 RASR=0x00000000
	R:3 RBAR=0x20000003 RASR=0x0301FC21
	R:4 RBAR=0x00000004 RASR=0x0602FC27
	R:5 RBAR=0x00000005 RASR=0x15010121
	R:6 RBAR=0x20000006 RASR=0x10010017
	R:7 RBAR=0x20000007 RASR=0x11018217
	CTRL=0x00000000
MPU ACL list - maximum of 8 entries:
  00: @0x400C8000 size=0x0080 pri=0
  01: @0x4000E400 size=0x0080 pri=0
  02: @0x40006000 size=0x0100 pri=0
  03: @0x0FE08100 size=0x0100 pri=0
uVisor switching to unprivileged mode
cb_test_param0()
cb_test_param0()
cb_test_param1(0x11111111)
cb_test_param2(0x11111111,0x22222222)
cb_test_param3(0x11111111,0x22222222,0x33333333)
cb_test_param4(0x11111111,0x22222222,0x33333333,0x44444444)
  caught access to 0x400C8000 (base=0x400C8000) - MPU reprogrammed
  caught access to 0x0FE081E0 (base=0x0FE08100) - MPU reprogrammed
  caught access to 0x4000E414 (base=0x4000E400) - MPU reprogrammed
  caught access to 0x40006030 (base=0x40006000) - MPU reprogrammed
hardware initialized, running main loop...

xor encryption...
arg: 0x0000FEED
ret: 0x0000FEC7
...done.
```
