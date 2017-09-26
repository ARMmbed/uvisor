#### uVisor

The uVisor is a self-contained software hypervisor that creates independent secure domains on ARM Cortex&reg;-M3 and Cortex&reg;-M4 microcontrollers. It increases resilience against malware and protects secrets from leaking even among different modules of the same application. You can find a [high-level overview here](http://www.slideshare.net/FoolsDelight/practical-realtime-operating-system-security-for-the-masses) ([Download PDF](https://github.com/ARMmbed/uvisor/raw/docs/uVisorSecurity-TechCon2016.pdf)).

*Note about interrupts:*
When the uVisor is enabled, all NVIC APIs are rerouted to the corresponding uVisor vIRQ APIs, which virtualize interrupts. The uVisor interrupt model has the following features:

- The uVisor owns the interrupt vector table.
- All ISRs are relocated to SRAM.
- Code in a box can only change the state of an IRQ (enable it, change its priority and so on) if the box registered that IRQ with uVisor through an IRQ ACL.
- You can only modify an IRQ that belongs to a box when that box context is active.

Although this behavior is different from that of the original NVIC, it is backward compatible. Legacy code (such as a device HAL) still works after uVisor is enabled.

##### Usage

1. Include uVisor library in your application.
1. Specify capabilities in mandatory access control list.
1. Add and configure *secure boxes*, special compartments with exclusive access to peripherals, memories and interrupts.
1. Create public entry points for your secure boxes.
1. Compile your application with uVisor enabled.

For detailed usage guides and best practices, look [here](https://github.com/ARMmbed/uvisor/blob/master/docs/README.md)

##### API

[API](https://github.com/ARMmbed/uvisor/blob/master/docs/lib/API.md)

[https://github.com/ARMmbed/uvisor/blob/master/docs/lib/API.md](https://github.com/ARMmbed/uvisor/blob/master/docs/lib/API.md)

##### Example

This is a simple example to show how to write a uVisor-secured threaded application with IRQ support. One LED blinks periodically from the public box main thread. A secure box exclusively owns the second LED, which toggles you press the user button.

###### main.cpp
[![View code](https://www.mbed.com/embed/?url=https://github.com/ARMmbed/mbed-os-example-uvisor)](https://github.com/ARMmbed/mbed-os-example-uvisor/blob/master/source/main.cpp)

###### main-hw.h
[![View code](https://www.mbed.com/embed/?url=https://github.com/ARMmbed/mbed-os-example-uvisor)](https://github.com/ARMmbed/mbed-os-example-uvisor/blob/master/source/main-hw.h)

###### led.cpp

[![View code](https://www.mbed.com/embed/?url=https://github.com/ARMmbed/mbed-os-example-uvisor)](https://github.com/ARMmbed/mbed-os-example-uvisor/blob/master/source/led.cpp)
