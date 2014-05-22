# crypto.box - a Cortex-M TEE Prototype

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
uVisor switching[unprivileged entry!] to unprivileged mode
MMFAR: 0x400C8000
CFSR : 0x00000082
```
