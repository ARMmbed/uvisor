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
	* limit unprivileged access to selected hardware periphals and memories
* allow interaction from unprivileged code by exposing a SVC call based API
* forward and de-privilege interrupts to unprivileged code
	* prevent register leakage of privileged code to unprivileged code
* force access to security-critical peripherals like DMA through the SVC api

### The crypto.box Client
* runs in unprivileged mode
* access to privileged memories and peripherals is prevented by the uVisor
* has direct memory access to unprivileged peripherals
* can register for unprivileged interrupts

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
#### Flash both uvisor and client:
```Bash
# rebuild cryptobox uvisor, erase chip and flash uvisor
make -C efm32_uvisor clean all erase flash
 
# rebuild cryptobox guest, keep uvisor, flash guest, reset CPU
# and start SWD terminal with debug output of uvisor and guest
make -C efm32_box    clean all flash reset swo
```

#### Example output:
```AsciiDoc
Target CPU is running @ 14000 kHz.
Receiving SWO data @ 900 kHz.
Showing data from stimulus port(s): 0, 1
-----------------------------------------------
uVisor s[unprivileged entry!]witching to unprivileged mode
cb_test_param0()
cb_test_param0()
cb_test_param1(0x11111111)
cb_test_param2(0x11111111,0x22222222)
cb_test_param3(0x11111111,0x22222222,0x33333333)
cb_test_param4(0x11111111,0x22222222,0x33333333,0x44444444)
MMFAR: 0x400C8000
CFSR : 0x00000082
```
