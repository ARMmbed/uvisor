# The uVisor

The uVisor is a self-contained software hypervisor that creates independent secure domains on ARM Cortex-M3 and M4 micro-controllers. Its function is to increase resilience against malware and to protect secrets from leaking even among different modules of the same application. You can find a [high level overview here](http://www.slideshare.net/FoolsDelight/resilient-iot-security-the-end-of-flat-security-models).

> Are you excited about security and interested in working on uVisor? [Join us](http://careers.peopleclick.com/careerscp/client_arm/external/jobDetails.do?functionName=getJobDetail&jobPostId=23423&localeCode=en-us) at ARM in Cambridge!

## Overview

To start using uVisor you will need to include it as a library in your design. We release the uVisor library periodically in the form of a yotta module in the [`uvisor-lib`](https://github.com/ARMmbed/uvisor-lib) repository.

If you want to learn more about the uVisor security model and get an overview of its features this is the right place. In this document you will read about:
* The uVisor design philosophy.
* A technical overview of the uVisor features:
    * Memory layout.
    * Secure boot.
    * Context switching.

Other documents you might be interested in:

| I want to...                               | Document                                                                                      |
|--------------------------------------------|-----------------------------------------------------------------------------------------------|
| Start using uVisor on a supported platform | [`uvisor-lib` quick-start guide](https://github.com/ARmmbed/uvisor-lib/blob/master/README.md) |
| Know everything I can do with uVisor       | [API documentation](https://github.com/ARmmbed/uvisor-lib/blob/master/DOCUMENTATION.md)       |
| Enable debug messages                      | [Debugging uVisor](docs/DEBUGGING.md)                                                         |
| Port uVisor to my platform                 | [Porting guide](docs/PORTING.md)                                                              |
| Test and experiment with uVisor            | [Developing with uVisor locally](docs/DEVELOPING_LOCALLY.md)                                        |
| Contribute to uVisor                       | [Contributing to uVisor](CONTRIBUTING.md)                                                     |

### Word of caution

This version of the uVisor is an early technology preview with an **incomplete implementation of the security features** of the final product. Future versions of uVisor will add these functions.

> Due to a re-structuring of the uVisor sources for greater portability, the current head of uVisor does not support LED blinking upon fault — you can find [more information here](https://github.com/ARMmbed/uvisor/issues/160)

Some of the open uVisor issues in progress are listed here:

* [Known issues](https://github.com/ARMmbed/uvisor/issues?q=is%3Aopen+is%3Aissue+label%3Aissue)
* [FIXMEs](https://github.com/ARMmbed/uvisor/search?utf8=%E2%9C%93&q=FIXME)

### Further reading:

* [Resilient IoT Security: The end of flat security models](http://www.slideshare.net/FoolsDelight/resilient-iot-security-the-end-of-flat-security-models): ARM TechCon 2015 presentation of the uVisor design philosophy and technical overview.
* [Q&A with ARM](http://eecatalog.com/IoT/2015/08/18/qa-with-arm-securing-the-iot-using-arm-cortex-processors-and-a-growing-mbed-platform-suite/): Securing the IoT using ARM Cortex processors, and a growing mbed platform suite.

### Supported platforms

The following platforms are currently supported:

* [NXP FRDM-K64F](http://developer.mbed.org/platforms/FRDM-K64F/)
* [STMicorelectronics STM32F429I-DISCO](http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/PF259090)

The uVisor pre-linked binary images are built with the Launchpad [GCC ARM Embedded](https://launchpad.net/gcc-arm-embedded) toolchain. Currently only applications built with the same toolchain are supported.

## The uVisor design philosophy

The need for security features applies across a wide range of today’s IoT products. We at ARM are convinced that many IoT security problems can be solved with standardised building blocks.

The uVisor is one of these basic building blocks – complementary to other important blocks like robust communication stacks, safe firmware updates and secure crypto libraries.

The design philosophy of uVisor is to provide hardware-enforced compartments (sandboxes) for individual code blocks by limiting access to memories and peripherals using the existing hardware security features of the Cortex-M micro-controllers.

Breaking the established flat security model of micro-controllers into compartmentalised building blocks results in high security levels, as the reach of flaws or external attacks can be limited to less sensitive function blocks.

A basic example of the uVisor features is preventing unauthorised access to flash memory from faulty or compromised code. This not only prevents malware from getting resident on the device, but also enables protection of device secrets like cryptographic keys.

Services built on top of our security layer can safely depend on an unclonable trusted identity, secure access to internet services and benefit from encryption key protection.

## Technical overview

The uVisor:

* Is initialised right after device start-up.
* Runs in privileged mode.
* Sets up a protected environment using a Memory Protection Unit (the ARM Cortex-M MPU or a vendor-specific alternative). In particular:
    * Its own memories and the security-critical peripherals are protected from the unprivileged code.
    * Unprivileged access to selected hardware peripherals and memories is limited through Access Control Lists (ACLs).
* Allows interaction from the unprivileged code by exposing SVCall-based APIs.
* Forwards and de-privileges interrupts to the unprivileged handler that has been registered for them.
* Prevents registers leakage when switching execution between privileged and unprivileged code and between mutually untrusted unprivileged modules.
* Forces access to some security-critical peripherals (like DMA) through SVCall-based APIs.

## The unprivileged code

All the code that is not explicitly part of the uVisor is generally referred to as unprivileged code. The unprivileged code:

* Runs in unprivileged mode.
* Has direct memory access to unrestricted unprivileged peripherals.
* Can require exclusive access to memories and peripherals.
* Can register for unprivileged interrupts.
* Cannot access privileged memories and peripherals.

The unprivileged code can be made of mutually untrusted isolated modules (or boxes). This way, even if all are running with unprivileged permissions, different modules can protect their own secrets and execute critical code securely.

For more details on how to setup a secure box and protect memories and peripherals, please read the [`uvisor-lib` quick-start guide](https://github.com/ARMmbed/uvisor-lib/blob/master/README.md).

### Memory layout

Different memory layouts can be used on different platforms, depending on the implemented memory protection scheme and on the MPU architecture. The following figure shows the memory layout of a system where the uVisor shares the SRAM module with the operating system (ARMv7-M MPU).

![uVisor memory layout](docs/img/memory_layout.png)

The uVisor secures two main memory blocks, in Flash and SRAM respectively. In both cases, it protects its own data and the data of the secure boxes it manages for the unprivileged code.

All the unprivileged code that is not protected in a secure domain is referred to as the *main box*.

The main memory sections that the uVisor protects are detailed in the following table:

<table>
  <tbody>
    <tr>
      <th>Memory section</th>
      <th>Description</th>
    </tr>
    <tr>
      <td>uVisor code</td>
      <td>The uVisor code is readable and executable by unprivileged code, so that code sharing is facilitated and privileged-unprivileged transitions are easier.
      </td>
    </tr>
    <tr>
      <td>uVisor data / BSS / stack</td>
      <td>The uVisor places all its constants, initialised and uninitialised data and the stack in secured areas of memory, separated from the unprivileged code.
      </td>
    </tr>
    <tr>
      <td>Secure boxes data / BSS / stack</td>
      <td>Through a configuration process, unprivileged code can set up a secure box for which data and stack can be secured by the uVisor and placed in isolated and protected memory areas.
      </td>
    </tr>
    <tr>
      <td>Vector table</td>
      <td>Interrupt vectors are relocated to the SRAM but protected by the uVisor. Access to them is made through specific APIs.
      </td>
    </tr>
  </tbody>
</table>

If you want to know how to use the uVisor APIs to setup a secure box please refer to the [`uvisor-lib` quick-start guide](https://github.com/ARMmbed/uvisor-lib/blob/master/README.md) and to the full [API documentation](https://github.com/ARMmbed/uvisor-lib/blob/master/DOCUMENTATION.md).

### The boot process

The uVisor is initialised right after device start-up and takes ownership of its most critical assets, like privileged peripherals, the vector table and memory management. The boot process involves the following steps, in this order:

1. Several sanity checks are performed, to verify integrity of the memory structure as expected by the uVisor.
2. The uVisor `bss` section is zeroed, the uVisor `data` section initialised.
3. The uVisor takes ownership of the vector table
4. The virtual Memory Protection Unit (vMPU) is initialized:
    * Secure boxes are loaded. For each of them:
        * The `bss` section is zeroed.
        * Access Control Lists (ACLs) are registered.
        * Stacks are initialised.
        * A private box context is initialized, if required.
    * The MPU (ARM or third-party) is configured.
5. Privileged and unprivileged stack pointers are initialised.
6. Execution is de-privileged and handed over to the unprivileged code.

From this moment on, the operating system/application runs in unprivileged mode and in the default context, which is the one of the main box.

### Context switching

The uVisor is able to set up a secure execution environment for itself and for each configured secure box. Whenever an event or an explicit call must be run in a specific environment, a context switch is triggered.

During a context switch, the uVisor stores the state of the previous context and then:
* It re-configures the stack pointer and the box context pointer.
* It re-configures the MPU and the peripherals protection.
* It hands the execution to the target context.

A context switch might be triggered automatically every time an interrupt belonging to a different box is served while another context is active. Context switches can also be manually triggered by using a so-called *secure gateway*.

For more details on how to use the secure gateway please read the [`uvisor-lib` quick-start guide](https://github.com/ARMmbed/uvisor-lib/blob/master/README.md).
