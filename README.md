# The uVisor

## Overview
The uVisor is a self-contained software hypervisor that creates independet
secure domains on ARM Cortex-M3 and M4 micro-controllers (M0+ will follow). Its
target is to increase resilience against malware and to protect secrets from
leaking even among different modules of the same application.

Currently the only supported platform is:
- Freescale FRDM-K64F (GCC ARM Embedded toolchain).

### The uVisor
Independently from target-specific implementations, the uVisor:
* is initialized right after device start-up;
* runs in privileged mode;
* sets up a protected environment using a Memory Protection Unit (the ARM
  Cortex-M MPU or a vendor-specific alternative); in particular:
    * its own memories and the security-critical peripherals are protected from
      the unprivileged code;
    * unprivileged access to selected hardware peripherals and memories is
      limited through Access Control Lists (ACLs);
* allows interaction from the unprivileged code by exposing an SVCall-based
  API;
* forwards and de-privileges interrupts to the unprivileged code that has
  registered for them;
* prevents register leakage when switching execution between privileged and
  unprivileged code and between mutually untrusted unprivileged modules;
* forces access to some security-critical peripherals (like DMA) through an
  SVCall-based API.

### The Client
All the code that is not explicitly part of the uVisor is generally referred to
as the Client. In general, the Client:
* runs in unprivileged mode;
* is prevented access to privileged memories and peripherals by the uVisor;
* has direct memory access to unprivileged peripherals;
* can register for unprivileged interrupts;
* can be made of mutually untrusted modules (or boxes) isolated by the uVisor.
The Client can interact with the uVisor thorugh a limited set of APIs which
allow to configure a secure module, protect its secrets and call untrusted
functions securely.

### Memory Layout
The following figure shows the memory layout of a system where security is
enforced by the uVisor.

![uVisor memory
layout](https://github.com/ARMmbed/uvisor/raw/images/k64f/docs/memory_layout.png
"uVisor memory layout")

The main memory sections on which the uVisor relies are described in the
following table:

| Memory section | Description                                                |
|----------------|------------------------------------------------------------|
| uvisor.bss     | Bla                                                        |
| uvisor.bla     | Even more bla                                              |

The uVisor code is readable and executable by unprivileged code. The reasons
for that are:
* easy transitions from privileged to unprivileged mode: De-privileging happens
  in the uVisor-owned memory segments;
* unprivileged code can directly call privileged helper functions without
  switching privilege levels. Helper functions can still switch to privileged
  mode via an SVCall;
* unprivileged code can verify the integrity of the privileged uVisor;
* confidentiality of the exception/interrupt table is still guaranteed as it's
  kept in secured RAM.

## The uVisor as a system or as a yotta module

The uVisor can be used in two different ways:

1. As a compact system. A custom start-up code for the target platform is used;
   the uVisor then takes full ownership and control of the system,
   de-privileging execution to the Client application. In this case the whole
   application can be built as follows:
   ```bash
   # select the correct platform in the code tree
   cd /path/to/platform-specific/code

   # rebuild the uVisor together with its unprivileged applications
   # erase the chip and flash the uVisor
   make clean all erase flash CONFIG=
   ```

2. As a yotta module. The uVisor is compiled and packaged to be included as a
   dependency by applications built with yotta on top of mbed. The yotta module
   for the uVisor is called uvisor-lib and can be found
   [here](https://github.com/ARMmbed/uvisor-lib). It is suggested to always use
   the official module. For development, experimenting, or fun, the uVisor can
   also be manually turned into a yotta module and locally linked to yotta.
   First of all, build the yotta module:
   ```bash
   # select the correct platform in the code tree
   cd /path/to/platform-specific/code

   # build the uVisor and generate the yotta module
   make clean release
   ```
   Then link your local build of yotta to this version of the module:
   ```bash
   # the release folder is at the top level of the code tree
   cd /path/to/release/in/the/code/tree

   # link the module to yotta locally
   sudo yotta link

   # link your project to this version of uvisor-lib
   cd /path/to/project/including/uvisor-lib
   yotta link uvisor-lib
   ```
   Again, consider using the uvisor-lib directly if building with yotta, and
   refer to its [documentation](https://github.com/ARMmbed/uvisor-lib).

## Debugging

By default, debugging output is silenced and only occurs when the device is
halted due to faults or failures catched by the uVisor. To enable more verbose
debugging, just build as follows:
```bash
# uVisor as a whole system
make clean all DEBUG=

# uVisor as a yotta module
make clean release DEBUG=
```
Please note that the `DEBUG=` option prevents the execution from continuing
if an SWO viewer is not connected.

Debugging messages are output through the SWO port. An SWO viewer is needed to
access it. If using a JLink debugger, just use:
```bash
cd /path/to/supported/platform
make swo
```
or refer to `Makefile.rules` to import the `swo` rule in your build
environment.

If SWO is also used by the Client application for different purposes (for
example, for regular print functions), make sure to re-direct it to a different
channel (channel 0 used here).

## Software and Hardware Requirements

* One of the available target boards

To build:
* Latest [GCC ARM Embeeded toolchain](https://launchpad.net/gcc-arm-embedded)

To debug:
* A JLink debugger (JLink LITE CortexM at least)
* Latest [SEGGER JLink software & documentation pack](https://www.segger.com/jlink-software.html) for the target platform
