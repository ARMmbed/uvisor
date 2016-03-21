# Overview

The uVisor is a self-contained software hypervisor that creates independent secure domains on ARM Cortex-M3 and M4 micro-controllers. Its function is to increase resilience against malware and to protect secrets from leaking even among different modules of the same application.

The need for security features applies across a wide range of today’s IoT products. We at ARM are convinced that many IoT security problems can be solved using standardized building blocks.

The uVisor is one of these basic building blocks – complementary to other important blocks like robust communication stacks, safe firmware updates and secure crypto-libraries.

Breaking the established flat security model of micro-controllers into compartmentalised building blocks results in high security levels, as the reach of flaws or external attacks can be limited to less sensitive function blocks.

The design philosophy of uVisor is to provide hardware-enforced compartments (sandboxes) for individual code blocks by limiting access to memories and peripherals using the existing hardware security features of the Cortex-M micro-controllers. In this documentation we refer to the sandboxes as processes.

A useful example of using uVisor features is to prevent unauthorized write access to flash memory from faulty or compromised code. This not only prevents malware from getting resident on the device, but also enables protection of device secrets like cryptographic keys.

Services built on top of our security layer can safely depend on an unclonable trusted identity, secure access to internet services and benefit from encryption key protection.

You can find more information and a [high level overview here](http://www.slideshare.net/FoolsDelight/resilient-iot-security-the-end-of-flat-security-models).

## Supported platforms

The following hardware platform is currently supported for :

* [NXP FRDM-K64F](http://developer.mbed.org/platforms/FRDM-K64F/)

The uVisor pre-linked binary images are built with the Launchpad [GCC ARM Embedded](https://launchpad.net/gcc-arm-embedded) toolchain. Currently only applications using that toolchain can be built.
