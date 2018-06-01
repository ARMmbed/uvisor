# The Arm Mbed uVisor

[![Build Status](https://travis-ci.org/ARMmbed/uvisor.svg?branch=master)](https://travis-ci.org/ARMmbed/uvisor)

<span class="warnings">**Warning**: uVisor is superseded by the Secure Partition Manager (SPM) defined in the ARM Platform Security Architecture (PSA). uVisor is deprecated as of Mbed OS 5.10, and being replaced by a native PSA-compliant implementation of SPM.</span>

The uVisor is a self-contained software hypervisor that creates independent secure domains on Arm Cortex&reg;-M3 and Cortex&reg;-M4 microcontrollers. It increases resilience against malware and protects secrets from leaking even among different modules of the same application. You can find a [high-level overview here](http://www.slideshare.net/FoolsDelight/practical-realtime-operating-system-security-for-the-masses) ([Download PDF](https://github.com/ARMmbed/uvisor/raw/docs/uVisorSecurity-TechCon2016.pdf)).

To start using uVisor, you need to include it as a library in your design. We release the uVisor library periodically into the Mbed OS repository, [ARMmbed/mbed-os](https://github.com/ARMmbed/mbed-os).

You can find most of the uVisor documentation in the [docs](docs) folder. Please look at the [getting started guide](docs/lib/QUICKSTART.md) for an introduction to uVisor application development. If you are interested in uVisor internals, please refer to the [OS-level introduction](https://github.com/ARMmbed/uvisor/raw/docs/uvisor-rtos-docs.pdf) and the [uVisor API docs](docs/lib/API.md).

Contributions to this repository in the form of issue reporting and pull requests are welcome! Please read our [contribution guidelines](CONTRIBUTING.md) first.

### Getting started examples

- The [basic uVisor example](https://github.com/ARMmbed/mbed-os-example-uvisor) shows how secure interrupts and C++ objects are instantiated in the context of secure boxes.
- The [uVisor threaded example](https://github.com/ARMmbed/mbed-os-example-uvisor-thread) demonstrates the configuration of multiple boxes containing secure threads.
- The [secure number store example](https://github.com/ARMmbed/mbed-os-example-uvisor-number-store/) demonstrates secure communication between boxes and implementation of secure APIs. The example shows how a called box can infer the caller's identity and ownership of secure objects across boots and firmware updates.

### Word of caution

This version of the uVisor is an early technology preview with an **incomplete implementation of the security features** of the final product. Future versions of uVisor will add these functions.

You can find some of the open uVisor issues here:

- [Known issues](https://github.com/ARMmbed/uvisor/issues?q=is%3Aopen+is%3Aissue+label%3Aissue).
- [FIXMEs](https://github.com/ARMmbed/uvisor/search?utf8=%E2%9C%93&q=FIXME).

### Further reading

- [Practical real-time operating system security for the masses](https://github.com/ARMmbed/uvisor/raw/docs/uVisorSecurity-TechCon2016.pdf): Arm TechCon 2016 PDF presentation of the uVisor design philosophy and technical overview.
- [Mbed uVisor integration in Mbed OS](https://github.com/ARMmbed/uvisor/raw/docs/uvisor-rtos-docs.pdf) in PDF format.
- [Q&A with Arm](http://eecatalog.com/IoT/2015/08/18/qa-with-arm-securing-the-iot-using-arm-cortex-processors-and-a-growing-mbed-platform-suite/): Securing the IoT using Arm Cortex&reg; processors and a growing mbed platform suite.

### Supported platforms

The uVisor core supports the following platforms:

- [NXP FRDM-K64F](http://developer.mbed.org/platforms/FRDM-K64F/).
- [STMicroelectronics STM32F429I-DISCO](http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/PF259090).
- [Silicon Labs EFM32 Gecko](http://www.silabs.com/products/mcu/32-bit/efm32-gecko/pages/efm32-gecko.aspx) (Cortex&reg;-M3 and Cortex&reg;-M4 devices).

To use uVisor on a specific OS, you must complete the porting process for that OS. This requires an additional porting step, which the [uVisor porting guide for Mbed OS](docs/core/PORTING.md) documents. 

Currently, uVisor only supports Mbed OS.

The Launchpad [GNU Arm Embedded](https://launchpad.net/gcc-arm-embedded) Toolchain builds the uVisor prelinked binary images. Currently, uVisor only supports applications built with this toolchain. The minimum required version is 5.4-2016q3.

## The uVisor design philosophy

The need for security features applies across a wide range of today’s IoT products. Many IoT security problems can be solved with standardized building blocks. The uVisor is one of these basic building blocks – complementary to other important blocks such as robust communication stacks, safe firmware updates and secure crypto libraries.

uVisor provides hardware-enforced compartments (sandboxes) for individual code blocks by limiting access to memories and peripherals using the existing hardware security features of the Cortex&reg;-M microcontrollers.

Breaking the established flat security model of microcontrollers into compartmentalized building blocks results in high security levels because the reach of flaws and external attacks can be limited to less sensitive function blocks.

The [uVisor example applications](#getting-started-examples) demonstrate features to prevent unauthorized access to flash memory from faulty or compromised code and interrupts. This prevents malware from getting resident on the device and enables protection of device secrets such as cryptographic keys.

Services built on top of the security layer can safely depend on an unclonable trusted identity, secure access to internet services and benefit from encryption key protection.

## Technical overview

The uVisor:

- Is initialized right after device startup.
- Runs in privileged mode.
- Sets up a protected environment using a Memory Protection Unit (MPU), such as the Arm Cortex&reg;-M MPU or a vendor-specific alternative. In particular:
    - Its own memories and the security-critical peripherals are protected from the unprivileged code.
    - Access Control Lists (ACLs) limit unprivileged access to selected hardware peripherals and memories.
- Allows interaction from the unprivileged code by exposing SVCall-based APIs.
- Forwards and deprivileges interrupts to the unprivileged handler that has been registered for them.
- Prevents registers leakage when switching execution between privileged and unprivileged code and between mutually untrusted unprivileged modules.
- Forces access to some security-critical peripherals (such as DMA) through SVCall-based APIs.

## The unprivileged code

All the code that is not explicitly part of the uVisor is generally referred to as unprivileged code. The unprivileged code:

- Runs in unprivileged mode.
- Has direct memory access to unrestricted unprivileged peripherals.
- Can require exclusive access to memories and peripherals.
- Can register for unprivileged interrupts.
- Cannot access privileged memories and peripherals.

The unprivileged code can be made of mutually untrusted isolated modules (or boxes). This way, even if all are running with unprivileged permissions, different modules can protect their own secrets and execute critical code securely.

For more details about how to setup a secure box and protect memories and peripherals, please read the [getting started guide](docs/lib/QUICKSTART.md).

### Memory layout

Different memory layouts can be used on different platforms, depending on the implemented memory protection scheme and the MPU architecture. The following figure shows the memory layout of a system where the uVisor shares the SRAM module with the operating system (ARMv7-M MPU).

[uVisor memory layout](docs/img/memory_layout.png)

The uVisor secures two main memory blocks, in flash and SRAM respectively. In both cases, it protects its own data and the data of the secure boxes it manages for the unprivileged code. For a more detailed view, please refer to the interactive [linker section visualization](https://meriac.github.io/mbed-os-linker-report/).

All the unprivileged code that is not protected in a secure domain is referred to as the *public box*.

This table details the main memory sections that the uVisor protects:

<table>
  <tbody>
    <tr>
      <th>Memory section</th>
      <th>Description</th>
    </tr>
    <tr>
      <td>uVisor code</td>
      <td>Unprivileged code can read and execute the uVisor code, so code sharing is facilitated, and privileged-unprivileged transitions are easier.
      </td>
    </tr>
    <tr>
      <td>uVisor data / BSS / stack</td>
      <td>The uVisor places all its constants, initialized and uninitialized data and the stack in secured areas of memory, separated from the unprivileged code.
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

To use the uVisor APIs to set up a secure box, please refer to the [getting started guide](docs/lib/QUICKSTART.md) and the full [uVisor API documentation](docs/lib/API.md).

### The boot process

The uVisor is initialized right after device startup and takes ownership of its most critical assets, such as privileged peripherals, the vector table and memory management. The boot process involves the following steps, in this order:

1. Several sanity checks verify integrity of the memory structure as expected by the uVisor.
1. The uVisor `bss` section is zeroed, the uVisor `data` section initialized.
1. The uVisor takes ownership of the vector table.
1. The virtual Memory Protection Unit (vMPU) is initialized:
    - Secure boxes are loaded. For each of them:
        - The `bss` section is zeroed.
        - Access Control Lists (ACLs) are registered.
        - Stacks are initialized.
        - A private box context is initialized, if required.
    - The MPU (Arm or third-party) is configured.
1. Privileged and unprivileged stack pointers are initialized.
1. Execution is deprivileged and handed over to the unprivileged code.

From this moment on, the operating system/application runs in unprivileged mode and in the default context, which is that of the public box.

### Context switching

The uVisor can set up a secure execution environment for itself and for each configured secure box. Whenever an event or an explicit call must run in a specific environment, a context switch is triggered.

During a context switch, the uVisor stores the state of the previous context and then:

- Reconfigures the stack pointer and the box context pointer.
- Reconfigures the MPU and the peripherals protection.
- Hands the execution to the target context.

A context switch is triggered automatically every time the target of a function call or exception handling routine (interrupts) belongs to a different secure domain. This applies to user interrupt service routines, threads and direct function calls.
