# uVisor Porting Guide

The uVisor is a low-level security module that creates sandboxed environments on ARM Cortex-M3 and ARM Cortex-M4 devices. Each sandbox can get hold of a set of private resources for which uVisor grants exclusive access. Calls into sandboxed APIs are only possible through uVisor-managed entry points.

The uVisor is released in the form of a static library. This library contains the implementation of all the uVisor APIs and the uVisor core, which is a pre-linked binary component.

This guide will help you to port uVisor to your platform and integrate its library in mbed OS.

## Overview

This document will cover the following:

* A quick presentation of the uVisor [repository structure](#repository-structure).
* A [step-by-step](#porting-steps) porting guide to bring uVisor to your platform.
* How to [integrate](#integrate-uvisor-in-mbed-os) your new port of uVisor into mbed OS.

For the remainder of this guide we will assume that you are porting a whole device family to uVisor, called `${family}`. We strongly suggest to abstract and generalize your port as much as possible. Since a static library is generated for each of your family configurations, a more generalized uVisor port result in the generation — and maintenance — of fewer release libraries. You will also enjoy the nice benefit of going through the porting process only once for a large set of devices.


You will need the following:
* The Launchpad GCC ARM embedded toolchain.
* GNU make.
* git.

## Repository Structure
[Go to top](#overview)

The uVisor is developed in the [ARMmbed/uvisor](https://github.com/ARMmbed/uvisor) repository. Most of your porting efforts will happen there.

Although uVisor is highly self-contained, it still requires some support from the target operating system. There are two files in particular that need to be changed in your code-base:

1. **Library glue layer**. [ARMmbed/uvisor](https://github.com/ARMmbed/uvisor) provides release and debug libraries for all family configurations. The logic to pick the right build for the right target is OS-specific and must be provided by a glue layer.
1. **Linker script**. It contains specific memory regions and symbols that uVisor relies on.
1. **Start-up script**. uVisor boots right after the system basic initialization, and before the C/C++ library initialization. The start-up script needs to call `uvisor_init()` in between those two.

If you are porting uVisor to mbed OS, the library glue layer is implemented in [ARMmbed/uvisor-lib](https://github.com/ARMmbed/uvisor-lib), the linker script is located in the platform yotta target, while the start-up script lives in the target-specific mbed HAL yotta module. We will guide you through the modifications needed in those modules later in this guide.

Finally, we provide an example application in [ARMmbed/uvisor-helloworld](https://github.com/ARMmbed/uvisor-helloworld) that shows the main uVisor features on our supported targets. It can be used during the porting process as a quick way of testing that the uVisor is working as expected on your new platform.

## Porting steps
[Go to top](#overview)

We will assume that you are developing in `~/code/`. Clone the uVisor GitHub repository locally:

```bash
$ cd ~/code
$ git clone git@github.com:ARMMbed/uvisor.git
```

### The uVisor configurations
[Go to top](#overview)

A single family of micro-controllers might trigger different releases of uVisor. Although we strive to keep uVisor as hardware-agnostic as possible, there are still some hardware-specific features that we need to take into account. These features are described in the table below.

| Symbol             | Description                                                                |
|------------------- |----------------------------------------------------------------------------|
| `FLASH_ORIGIN`     | Origin of the physical flash memory where uVisor code is placed            |
| `FLASH_OFFSET`     | Offset in flash at which uVisor is located                                 |
| `SRAM_ORIGIN`      | Origin of the physical SRAM memory where uVisor .bss and stacks are placed |
| `SRAM_OFFSET`      | Offset in SRAM at which uVisor .bss and stacks are located                 |
| `FLASH_LENGTH_MIN` | min( [`FLASH_LENGTH(i)` for `i` in family's devices] )                     |
| `SRAM_LENGTH_MIN`  | min( [`SRAM_LENGTH(i)` for `i` in family's devices] )                      |
| `NVIC_VECTORS`     | max( [`NVIC_VECTORS(i)` for `i` in family's devices] )                     |
| `CORE_*` *         | Core version (e.g `CORE_CORTEX_M3`)                                        |

**Table 1**. Hardware-specific features that differentiate uVisor builds

A uVisor *configuration* is defined as the unique combination of the parameters of Table 1. When porting your family to uVisor, you need to make sure that as many library releases as the possible configurations are generated. Let's use an example.

---

#### Example

Let's assume for simplicity that the `${family}` that you want to port is made of only 4 devices. These have the following values from Table 1:

| Symbol            | `${device0}` | `${device1}` | `${device2}` | `${device3}` |
|-------------------|--------------|--------------|--------------|--------------|
| `FLASH_ORIGIN`    | 0x0          | 0x0          | 0x0          | 0x0          |
| `FLASH_OFFSET`    | 0x400        | 0x400        | 0x400        | 0x400        |
| `SRAM_ORIGIN`     | 0x20000000   | 0x20000000   | 0x1FFF0000   | 0x1FFF0000   |
| `SRAM_OFFSET`     | 0x200        | 0x200        | 0x400        | 0x400        |
| `FLASH_LENGTH(i)` | 0x100000     | 0x100000     | 0x80000      | 0x80000      |
| `SRAM_LENGTH(i)`  | 0x10000      | 0x20000      | 0x10000      | 0x20000      |
| `NVIC_VECTORS(i)` | 86           | 122          | 86           | 122          |
| `CORE_`           | `CORTEX_M4`  | `CORTEX_M4`  | `CORTEX_M4`  | `CORTEX_M4`  |

**Table 2**. Example uVisor configuration values

Following the descriptions of Table 1, some values are common among the 4 devices:

* `NVIC_VECTORS` is the maximum `NVIC_VECTORS(i)`, hence it is 122.
* `FLASH_LENGTH_MIN` and `SRAM_LENGTH_MIN` are 0x80000 and 0x10000, respectively.
* `FLASH_ORIGIN` and `FLASH_OFFSET` are the same for all the devices, so they will be common to all configurations. The same applies to the core version, so `CORE_CORTEX_M4` will be defined.

The remaining values must be combined to form distinct configurations. In this case we only need to combine `SRAM_ORIGIN` and `SRAM_OFFSET`. If you look at the table above, you will see that they appear in 2 out of the 4 possible value combinations. Hence, we have a total of 2 uVisor configurations:

```bash
CONFIGURATION_${family}_1 = {0x0, 0x400, 0x20000000, 0x200, 0x80000, 0x10000, 122, CORE_CORTEX_M4}
CONFIGURATION_${family}_2 = {0x0, 0x400, 0x1FFF0000, 0x400, 0x80000, 0x10000, 122, CORE_CORTEX_M4}
```

`${device0}` and `${device1}` belong to `CONFIGURATION_${family}_1`; `${device2}` and `${device3}` belong to `CONFIGURATION_${family}_2`.

---

### Memory architecture
[Go to top](#overview)

TODO.

The uVisor expects a distinction between at least two memories: Flash and SRAM. If more than one memory fits this description, you should make an architectural decision about where to put uVisor. We suggest that if you have fast memories you should use those, provided that they offer security features at least equal to those of the regular memories.

### Platform-specific code
[Go to top](#overview)

Look at the `uvisor` folder structure:

```bash
uvisor
├── ...
│
├── core                       # uVisor main codebase; it is hardware-independent.
│
├── platform                   # Hardware-specific folder
│   ├── ...
|   └── ${family}              # NEW SUPPORTED FAMILY: You will develop here.
│       ├── [*] Makefile.configurations
│       ├── docs
│       └── inc
│           ├── [*] config.h
│           └── [*] configurations.h
│
└── api
    ├── ...
    └── lib
        ├── ...
        └── ${family}          # The release libraries end up here.
```

Each `[*]` indicates a file you must create during the porting process. Details for each of them are presented below. The snippets that we show refer to the configurations discussed in the previous example.

---

```bash
~/code/uvisor/platform/${family}/inc/configurations.h
```

This file contains the uVisor configurations for your family. Remember that each configuration triggers a separate library release. This file should be auto-generated. Symbols that are common to all devices in the family should go first. Configuration-specific symbols are conditionally defined. The condition is based on a macro definition of the form `CONFIGURATION_${CONFIGURATION_NAME}`.

**Example**
```C
#ifndef __CONFIGURATIONS_H__
#define __CONFIGURATIONS_H__

#define FLASH_ORIGIN     0x0
#define FLASH_OFFSET     0x400
#define FLASH_LENGTH_MIN 0x80000
#define SRAM_LENGTH_MIN  0x10000
#define NVIC_VECTORS     122
#define CORE_CORTEX_M4

#if defined(CONFIGURATION_${family}_1)
#define SRAM_ORIGIN 0x20000000
#define SRAM_OFFSET 0x400
#endif

#if defined(CONFIGURATION_${family}_2)
#define SRAM_ORIGIN 0x1FFF0000
#define SRAM_OFFSET 0x200
#endif

#endif /* __CONFIGURATIONS_H__ */
```

---

```bash
~/code/uvisor/platform/${family}/inc/config.h
```

This file contains uVisor customizations that are not hardware-specific but can be chosen by each family (e.g. the default stack size).

The symbols that you can specify here are listed in the table below.

| Symbol                        | Description |
|-------------------------------|-------------|
| `STACK_GUARD_BAND`            | TODO        |
| `STACK_SIZE`                  | TODO        |
| `NDEBUG`                      | TODO        |
| `DEBUG_MAX_BUFFER`            | TODO        |
| `CHANNEL_DEBUG`               | TODO        |
| `MPU_MAX_PRIVATE_FUNCTIONS`   | TODO        |
| `MPU_REGION_COUNT`            | TODO        |
| `ARMv7M_MPU_REGIONS`          | TODO        |
| `ARMv7M_ALIGNMENR_BITS`       | TODO        |
| `ARMv7M_MPU_RESERVED_REGIONS` | TODO        |
| `UVISOR_MAX_ACLS`             | TODO        |

**Table 3**. Optional hardware-specific `config.h` symbols

**Note**: You must always have a separate `configurations.h` file, even if the remaining `config.h` is empty. This ensures that `configurations.h` (which is possibly auto-generated by a script of yours) does not need to know anything more than the features of Table 1.

**Example**
```C
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define STACK_SIZE 2048
#include "configurations.h"

#endif /* __CONFIG_H__ */
```

---

```bash
~/code/uvisor/platform/${family}/Makefile.configurations
```

This file configures the build system for your device family. The table below shows the symbols that can be defined.

| Symbol           | Description                                                               |
|------------------|---------------------------------------------------------------------------|
| `ARCH_MPU`       | MPU architecture (if absent defaults to `ARMv7M`; alternative: `KINETIS`) |
| `CONFIGURATIONS` | List of uVisor configurations, as defined in `config.h`.                  |

**Table 4**. Platform-specific configuration symbols

**Example**

```bash
# MPU architecture
ARCH_MPU:=ARMv7M

# Family configurations
CONFIGURATIONS:=\
    CONFIGURATION_${family}_1 \
    CONFIGURATION_${family}_2
```

### Build
[Go to top](#overview)

You can finally generate all the uVisor library releases:

```bash
$ cd ~/code/uvisor
$ make
```

The build process generates as many static libraries (`*.a` files) as your family configurations, multiplied by 2 (debug and release builds). You can find them in `~/code/uvisor/api/lib/${family}`.


## Integrate uVisor in mbed OS
[Go to top](#overview)

You now need to integrate uVisor in your code-base. As anticipated above, this involves changing two files, the linker script and the start-up script. We already implemented the uVisor library glue layer for mbed OS; you can find it at [ARMmbed/uvisor-lib](https://github.com/ARMmbed/uvisor-lib). You only need to add one file to that module to make sure your family is correctly recognised.

### Start-up script
[Go to top](#overview)

This usually lives in an `mbed-hal-$family` module, if you already ported your platform to mbed. The start-up code must call the function `uvisor_init()` right after system initialization (usually called `SystemInit()`) and right before the C/C++ library initialization.

```C
ResetHandler:
    ...
    ldr r0, =SystemInit
    blx r0
    ldr r0, =uvisor_init    /* [*] Insert this. */
    blx r0                  /* [*] Insert this. */
    ldr r0, =__start
    bx  r0
    ...
```

### Linker script
[Go to top](#overview)

To enforce the security model, we need to place uVisor at a specific location in memory, and give uVisor specific information about the rest of the memory map. These symbols live in the `ld` linker script.

The snippet below can be used as a template for your `ld` linker script.

```
MEMORY
{
    VECTORS_FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x00000400
    FLASH (rx)         : ORIGIN = 0x00000400, LENGTH = 0x00100000 - 0x00000400
    VECTORS_RAM (rx)   : ORIGIN = 0x1FFF0000, LENGTH = 0x00000400
    RAM (rwx)          : ORIGIN = 0x1FFF0400, LENGTH = 0x00040000 - 0x00000400
}

ENTRY(ResetHandler)

SECTIONS
{
    .isr_vector :
    {
        __vector_table = .;
        KEEP(*(.vector_table))
         . = ALIGN(4);
    } > VECTORS

    /* Ensure that the uVisor bss is at the beginning of the flash memory. */
    .uvisor.bss (NOLOAD):
    {
        . = ALIGN(32);
        __uvisor_bss_start = .;

        /* Protected uVisor main bss */
        . = ALIGN(32);
        __uvisor_bss_main_start = .;
        KEEP(*(.keep.uvisor.bss.main))
        . = ALIGN(32);
        __uvisor_bss_main_end = .;

        /* Protected uVisor secure boxes bss */
        . = ALIGN(32);
        __uvisor_bss_boxes_start = .;
        KEEP(*(.keep.uvisor.bss.boxes))
        . = ALIGN(32);
        __uvisor_bss_boxes_end = .;

        . = ALIGN(32);
        __uvisor_bss_end = .;
    } > RAM

    .text :
    {
        /* uVisor code and data */
        . = ALIGN(4);
        __uvisor_main_start = .;
        *(.uvisor.main)
        __uvisor_main_end = .;

        *(.text*)
        ...
    } > FLASH

    ...

    /* uVisor configuration data */
    .uvisor.secure :
    {
        . = ALIGN(32);
        __uvisor_secure_start = .;

        /* uVisor secure boxes configuration tables */
        . = ALIGN(32);
        __uvisor_cfgtbl_start = .;
        KEEP(*(.keep.uvisor.cfgtbl))
        . = ALIGN(32);
        __uvisor_cfgtbl_end = .;

        /* Pointers to the uVisor secure boxes configuration tables */
        /* Note: Do not add any further alignment here, as we need to have the
         *       exact list of pointers. */
        __uvisor_cfgtbl_ptr_start = .;
        KEEP(*(.keep.uvisor.cfgtbl_ptr_first))
        KEEP(*(.keep.uvisor.cfgtbl_ptr))
        __uvisor_cfgtbl_ptr_end = .;

        . = ALIGN(32);
        __uvisor_secure_end = .;
    } >FLASH

    /* Section for uninitialized data
     * It is not needed by uVisor but still highly recommended for testing
     * reasons. */
    .uninitialized (NOLOAD):
    {
        . = ALIGN(32);
        __uninitialized_start = .;
        *(.uninitialized)
        KEEP(*(.keep.uninitialized))
        . = ALIGN(32);
        __uninitialized_end = .;
    } > RAM

    ...

    /* Provide physical memory boundaries for uVisor. */
    __uvisor_flash_start = ORIGIN(VECTORS_FLASH);
    __uvisor_flash_end = ORIGIN(FLASH) + LENGTH(FLASH);
    __uvisor_sram_start = ORIGIN(VECTORS_RAM);
    __uvisor_sram_end = ORIGIN(RAM) + LENGTH(RAM);
}
```

As shown in the snippet above, the uVisor needs the following three regions:

<table>
  <tbody>
    <tr>
      <th>Region</th>
      <th>Description</th>
    </tr>
    <tr>
      <td>
        <code>.uvisor.main</code>
      </td>
      <td>
        Holds the monolithic uVisor core binary. Its location determines the entry point of uVisor, since <code>uvisor_init</code> points to the first opcode in this binary blob. You should make sure that the linker script has this symbol at the same offset that you specified as <code>FLASH_OFFSET</code> in your configuration.
      </td>
    </tr>
    <tr>
      <td>
        <code>.uvisor.bss</code>
      </td>
      <td>
        Contains both uVisor's own memories (coming from <code>uvisor-lib</code>, in <code>.keep.uvisor.bss.main</code>) and the secure boxes' protected memories (in <code>.keep.uvisor.bss.boxes</code>). You must make sure that this region (and hence its first sub-region, <code>.keep.uvisor.bss.boxes</code>) is positioned in SRAM at the same offset that you specified in <code>SRAM_OFFSET</code>. To avoid having data loaded from flash ending up before uVisor, we strongly suggest to put this section at the top of the linker script.
      </td>
    </tr>
    <tr>
      <td>
        <code>.uvisor.secure</code>
      </td>
      <td>
        Contains constant data in flash that describes the configuration of the secure boxes. It comprises the configuration tables (<code>.keep.uvisor.cfgtbl</code>) and the pointers to them (<code>.keep.uvisor.cfgtbl_ptr[_first]</code>), which are used by uVisor for box enumeration.
      </td>
    </tr>
  </tbody>
</table>

All these sections and their sub-regions must be preceded and followed by symbols that describe their boundaries, as shown in the template. Note that all symbols prefixed with `.keep` must end up in the final memory map even if they are not explicitly used. Please make sure to use the correct `ld` commands (`KEEP(...)`).

Once uVisor is active it maintains its own vector table, which the rest of the code can only interact with through uVisor APIs. We still suggest to leave the standard vector table in flash, so that if uVisor is disabled the classic `NVIC_{S,G}etVector` functions can still work and relocate it to SRAM. This is the same reason why we need an `SRAM_OFFSET` as well, as it reserves the space for relocation of the original OS vector table.

Finally, note that at the end of the linker script the physical boundaries for the memories (flash/SRAM) populated by uVisor are also provided.

### Library glue layer
[Go to top](#overview)

The `uvisor-lib` yotta module ensures that the correct uVisor library release is selected depending on the yotta target, as determined from the built-in `TARGET_LIKE_*` pre-processor symbols.

You only need to add one file to this module. First, clone its repository:

```bash
$ cd ~/code
$ git clone git@github.com/ARMmbed/uvisor-lib.git
```

This repository is configured to have [ARMmbed/uvisor](https://github.com/ARMmbed/uvisor) as a submodule, so that the uVisor libraries and API headers can be included directly without duplicating code. In this case we suggest you do *not* initialize the submodule. Instead, link you local version of [ARMmbed/uvisor](https://github.com/ARMmbed/uvisor) to the current folder:

```bash
$ cd ~/code/uvisor-lib
$ ln -s ~/code/uvisor .
```

The only file that you need to add is named `support_${vendor}.cmake`, and must be located in the `source` folder inside `~/code/uvisor-lib`. This file selects the correct configuration based on your target.

**Example**

```bash
if(TARGET_LIKE_${family})
    set(UVISOR_FAMILY "${family}")

    if(TARGET_LIKE_${family}${device0} OR TARGET_LIKE_${family}${device1})
        set(UVISOR_CONFIGURATION "CONFIGURATION_${family}_1")
    endif()

    if(TARGET_LIKE_${family}${device2} OR TARGET_LIKE_${family}${device3})
        set(UVISOR_CONFIGURATION "CONFIGURATION_${family}_2")
    endif()
endif()
```

### Next steps
[Go to top](#overview)

If you have followed all the steps in this guide, this is what you should have:
* Your whole family ported to the [ARMmbed/uvisor](https://github.com/ARMmbed/uvisor) code-base.
* All the uVisor library releases built for your family, saved in `~/code/uvisor/api/lib/${family}`.
* The uVisor libraries integrated in mbed OS:
    * The linker script in `target-${your_target}`.
    * The start-up script in `mbed-hal-${your_target}`.
    * The CMake logic to include the uVisor libraries in `uvisor-lib`.

You can now activate and test uVisor. This can be done by setting the `uvisor.present` yotta configuration to `1`, in your target's `target.json` file:

```bash
"config": {
  ...
  "uvisor": {
    "present": 1
  },
  ...
}
```

You are now ready to test the new port of uVisor with our [ARMmbed/uvisor-helloworld](https://github.com/ARMmbed/uvisor-helloworld) application. Please follow the [Developing with uVisor locally](DEVELOP_LOCALLY.md) document for a step-by-step guide.
