# The uVisor

## Overview

The uVisor is a self-contained software hypervisor that creates independet
secure domains on ARM Cortex-M3 and M4 micro-controllers (M0+ will follow). Its
target is to increase resilience against malware and to protect secrets from
leaking even among different modules of the same application.

Currently the only supported platform is:
- Freescale FRDM-K64F (GCC ARM Embedded toolchain).


## The uVisor design philosophy

The need for security features applies across a wide range of today’s IoT
products. We at ARM are convinced that many IoT security problems can be solved
with standardized building blocks.

The uVisor project is one of these basic building blocks – complementary to
other important blocks like robust communication stacks, safe firmware update
and secure crypto libraries.

The design philosophy of uVisor is to provide hardware-enforced compartments
(sandboxes) for individual code blocks by limiting access to memories and
peripherals using the existing hardware security features of Cortex-M
micro-controllers.

Breaking the established flat security model of micro-controllers apart into
compartmentalized building blocks results in high security levels as the reach
of flaws or external attacks can be limited to less sensitive function blocks.
Convenient remote recovery from malware infection or faulty code can be
guaranteed as recovery code and security checks can be protected even against
local attackers that can run code on the device.

A basic example for uVisor is preventing unauthorized access to flash memory
from faulty or compromised code. This not only prevents malware from getting
resident on the device, but also enables protection of device secrets like
cryptographic keys.

Services built on top of our security layer can safely depend on an unclonable
trusted identity, secure access to internet services and benefit from
encryption key protection.

### Hardware-enforced IoT security

It is obvious to developers that security-critical functions like SSL libraries
need thorough vetting when security matters are involved. It is less obvious
that the same level of attention is needed for all other components in the
system, too. In the traditional micro-controller security model bugs in a
single system component compromise all the other components in the system.

When comparing the huge amount of code involved in maintaining WiFi connections
or enabling ZigBee or BLE communication, it becomes obvious that the resulting
attack surface is impossible to verify and thus compromises device security –
especially as important parts of the stack are often available in binary format
only.

To make things even harder, the recovery from a common class of security flaws
– the execution of arbitrary code by an attacker – can lead to a situation
where remote recovery becomes impossible. The attacker can run code on the
target device which infects further updates for the malware to become resident.

Even a hardware-enforced root of trust and a secure boot loader will not fix
that problem. The resident malware can run safely from RAM and can decide to
block commands for resetting the device or erasing the flash as part of a
denial-of-service attack.

The same is true for extraction of security keys by an attacker; it is
impossible to rotate security keys safely, as an attacker will see key updates
in real time and plain-text once running code on the device.

To fix that situation to our advantage, we need to reduce the attack surface as
much as we can by using uVisor to shield critical peripherals from most of the
code base. Attackers can now compromise the untrusted side containing the
application logic and communication stack without affecting security on the
private side, which holds basic crypto functions and the actual keys. As the
private side is now impervious to attacks and cannot be compromised, it can
safely reason about the security state of the public side.

By utilizing a server-bound crypto watchdog, the private side can enforce
communication to the server, even through malware-infested code on the public
side. In case of doubt, the private side or the remote server can reliably
reset or re-flash the public side to a clean state.

This safety network enables fast innovation, as security flaws in the public
state are safely recoverable after the product is shipped. Security keys are
protected and can keep their validity and integrity even after a malware attack
has been successfully stopped.

Code for the public state can be developed rapidly and does not have to go
through the same level of security reasoning as the code in the private state,
resulting in quick innovations and a faster time to market.

Recovery from security bugs becomes painless as firmware updates can be
reliably enforced.


## Technical details

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

### Memory layout

The following figure shows the memory layout of a system where security is
enforced by the uVisor.

![uVisor memory layout](/k64f/docs/memory_layout.png)

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

1. As a compact system
2. As a yotta module

When used as a yotta module on top of mbed, the uVisor comes as a
pre-compiled binary blob which is then included in the rest of the system. In
this way the integrity of the uVisor, and hence its security model, are
guardanteed. Each official release of uvisor-lib will then deliver an approved
build of the code tree here presented, also exposing the APIs for the Client
application.

### As a compact system

   A custom start-up code for the target platform is used;
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

### As a yotta module

   The uVisor is compiled and packaged to be included as a
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

## Compile debug version of uvisor
The following recipe is used to enable the single wire debug debug (SWD) version of uvisor with SWO output using a SEGGER JLink debugger:
```bash
# create custom directory
mkdir debug
cd debug/

# clone & link private uvisor-lib repository
git clone git@github.com:ARMmbed/uvisor-lib-private.git
cd uvisor-lib-private/
yt link
cd ..

# build JLink SWO debug console version & copy binary to private uvisor-lib
git clone git@github.com:ARMmbed/uvisor-private.git
cd uvisor-private/k64f/uvisor/
make OPT= TARGET_BASE=../../../uvisor-lib-private clean release
cd ../../../

# compile hello world example 
git clone git@github.com:ARMmbed/uvisor-helloworld-private.git
cd uvisor-helloworld-private/
yt link uvisor-lib

# flash firmware
make clean flash

# start debug output
make swo
```

## Software and hardware requirements

* One of the available target boards

To build:
* Latest [GCC ARM Embeeded toolchain](https://launchpad.net/gcc-arm-embedded)

To debug:
* A JLink debugger (JLink LITE CortexM at least)
* Latest [SEGGER JLink software & documentation  pack](https://www.segger.com/jlink-software.html) for the target platform
