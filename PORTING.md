uVisor Porting Guide
====================

This is a porting guide to bring uVisor to your platform.

uVisor is a low-level security module that creates sandboxed environments on ARM Cortex-M3 and M4 devices. Each sandbox can get hold of a set of private resources for which uVisor grants exclusive access. Calls into sandboxed APIs are only possible through uVisor-managed entry points.

uVisor is released in the form of a binary component, encapsulated in the `uvisor-lib` yotta module. It can be used in all ported mbed platforms, although fallback implementations of low-level APIs are provided for unsupported ones.

Overview
--------

This document will cover the following:
- [Setting up](#setting-up) your environment for development with uVisor and mbed.
- A quick presentation of the uVisor [repository structure](#repository-structure).
- A [step-by-step](#porting-steps) guide to porting `uvisor` to your platform.
- [Integrating](#making-your-modules-uvisor-aware) uVisor into your codebase.
- Linking and [building with yotta](#building-with-yotta).
- [Debugging](#debugging-uvisor) uVisor.

For the remainder of this guide we will assume your platform is called:
```
awesome42-XXX-YYYYY
|_______| |_| |___|
    |      |    |
   ARCH FAMILY CHIP
```
We will expect that devices in the `$ARCH` range only differ in a very few elements, although every `$(ARCH)$(FAMILY)$(CHIP)` combination can potentially result in a separate uVisor release blob. The code tree should help you capture these differences in a hierarchical way, so that a single porting can involve many devices.

Setting Up
----------
[Go to top](#overview)

Make sure you have the following:
- A gdb-ready development platform. Although it is not strictly necessary, we strongly suggest this as debug messages are critical in the porting process. Currently uVisor uses semihosting to print its debugging messages, which blocks execution until the debugger is disconnected. We use the [Segger JLink tools](https://www.segger.com/jlink-debug-probes.html) to connect to our boards.

- A supported toolchain. Please check out the [updated list](https://github.com/ARMmbed/uvisor-lib#supported-platforms). We use the [Launchpad GCC ARM](https://launchpad.net/gcc-arm-embedded) toolchain to build uVisor; other toolchains can be supported to include `uvisor-lib` in mbed applications.

- An up-to-date version of yotta, the tool we use to build, test and debug our mbed applications. Please check out its [documentation](http://yottadocs.mbed.com/) for instructions on how to install and use it.

- An up-to-date version of git. This is not necessary for the porting of uVisor itself, but if you want to contribute to the mbed official codebase then pull requests on [GitHub](https://github.com/ARMMbed/mbed-os) are the best (and official) way.

Please note that all the instructions and commands shown here have been tested on Linux (Ubuntu 15.04) and Mac OS X (10.9). The process should be very similar on Windows, although immediate support is not guaranteed in that case.

Repository Structure
--------------------
[Go to top](#overview)

We use three main GitHub repositories for developing, releasing and debugging uVisor:

<table>
  <tbody>
    <col width="170">
    <tr>
      <th>Repository</th>
      <th>Description</th>
    </tr>
    <tr>
      <td>
        <a href="https://github.com/ARMmbed/uvisor">
          <code>uvisor</code>
        </a>
      </td>
      <td>
        This is the repository where the uVisor code lives. We do most of our development here, then propagate it to the release module <code>uvisor-lib</code>.
      </td>
    </tr>
    <tr>
      <td>
        <a href="https://github.com/ARMmbed/uvisor-lib">
          <code>uvisor-lib</code>
        </a>
      </td>
      <td>
        Binary release of uVisor, in the form of a yotta module. The release process generates a pre-linked image to ensure that uVisor is fully self-contained and does not depend on untrusted libraries. We don't usually develop code here directly, except for changes that are yotta-specific. A set of unit tests is available to check that uVisor is working properly.
      </td>
    </tr>
    <tr>
      <td>
        <a href="https://github.com/ARMmbed/uvisor-helloworld">
          <code>uvisor-helloworld</code>
        </a>
      </td>
      <td>
        A Hello World application showing uVisor features through a simple challenge that involves the protection of a secret. You can use this application during porting to quickly verify that uVisor is working properly.
      </td>
  </tbody>
</table>

Other components are important for the integration of uVisor in the target platform. They don't necessarily need to be git repositories, although usually each separate yotta target, module and application lives in its own repository.

<table>
  <tbody>
    <col width="290">
    <tr>
      <th>Component</th>
      <th>Description</th>
    </tr>
    <tr>
      <td>
        <code>target-*</code>
      </td>
      <td>
        This is a yotta target, which contains the linker script and toolchain flags for the target platform. uVisor relies on specific memory regions and symbols, and we must define these in the linker script.
      </td>
    </tr>
    <tr>
      <td>
        <code>mbed-hal-*</code>
      </td>
      <td>
        This yotta module contains the startup code for the target platform. uVisor needs to boot right after the system basic initialization routine (usually <code>SytemInit()</code>) and before the C/C++ library initialization.
      </td>
    </tr>
  </tbody>
</table>

You will only have these modules if you have already started porting your platform to mbed OS. If you don't have the modules then the changes affecting the two components above, which we will describe shortly, should be made in the corresponding files and folders in your existing codebase.


Porting Steps
-------------
[Go to top](#overview)

Assuming you are developing in `~/code/`, we suggest you clone two of the three GitHub repositories locally:
```bash
cd ~/code
git clone --recursive git@github.com:ARMMbed/uvisor.git
git clone git@github.com:ARMMbed/uvisor-helloworld.git
```
The `--recursive` option ensures that `uvisor-lib`, a sub-module of `uvisor`, is also cloned. When you run the release script, uVisor will be built from `uvisor` into a binary release that is automatically copied into `uvisor-lib`. When the release script finished, you can use yotta to link the built version of `uvisor-lib` to `uvisor-helloworld`, so that `uvisor-helloworld` uses `uvisor-lib` instead of the official version in the public registry. This allows you to test your own code with Hello World.

**Tip:** If you are not familiar with the process of linking in yotta, please refer to [this guide](http://yottadocs.mbed.com/reference/commands.html#yotta-link); we will show [examples later in this guide](#building-with-yotta).

We only update the pointer to `uvisor-lib` at the time of a uVisor release, so to make sure that `uvisor` is pointing to the `HEAD` of `uvisor-lib`, please rebase it:

```bash
cd ~/code/uvisor/release
git pull --rebase
```
**Note:** Your `git status` will list the `uvisor/release` folder as changed. You can ignore these changes.

Now look at the `uvisor` folder structure:

**Note:** Each `[*]` indicates a file you'll need to edit during the porting process.

```bash
uvisor
├── ...
│
├── core                           # uVisor main codebase
│   ├── Makefile.rules             # main Makefile script; it is hardware-independent
│   ├── Makefile.scripts
│   │
│   ├── arch
│   │   ├── ...
│   │   ├── [*]
│   │   └── STM32F4
│   │       ├── inc/system.h       # hardware-specific interrupts routines prototypes
│   │       ├── inc/*.h            # hardware-specific header files
│   │       └── src/system.c       # hardware-specific interrupts routines aliases to default handler
│   │
│   ├── cmsis                      # CMSIS header files
│   │
│   ├── debug                      # hardware-independent debug utilities
│   │
│   ├── lib                        # external libraries
│   │   └── printf
│   │
│   ├── linker
│   │   ├── ...
│   │   ├── [*]
│   │   ├── STM32F$29XX.h          # hardware-specific symbols for default.h
│   │   └── default.h              # default, pre-processed linker script (uses hardware-specific symbols)
│   │
│   ├── mbed                       # mbed-specific release files for uvisor-lib
│   │   ├── source
│   │   └── uvisor-lib
│   │       ├── ...
│   │       └── [*] platforms.h    # list of supported platforms; unsupported ones get fallback APIs/macros
│   │
│   ├── system                     # the actual uVisor code base; it is hardware-independent
│   │
│   ├── uvisor-device.h [*]        # conditional inclusion of hardware-specific header files
│   │
│   └── uvisor.h                   # central header file (its inclusion triggers inclusion of all others)
│
├── stm32f4                        # hardware-specific folder
│   ├── docs                       #   hardware-specific peculiarities should be documented here
│   └── uvisor                     #   hardware-specific code; implements functions declared in main code base
│       ├── Makefile
│       ├── config
│       │   └── config.h
│       ├── debug
│       │   └── stm32f4_memory_map.c
│       └── src
│           └── stm32f4_halt.c
│
├── [*] awesome42                  # hardware-specific code for your platform goes here
│   ├── docs
│   └── uvisor
│       ├── Makefile
│       ├── config
│       │   └── config.h
│       ├── debug
│       │   └── awesome42_memory_map.c
│       └── src
│           └── awesome42_halt.c
│
└── release                        # the auto-generated uvisor-lib yotta module ends up here
    ├── ...
    └── source
        ├── ...
        └── [*] CMakeLists.txt     # includes source files conditionally (supported/unsupported)

```

Please perform the following steps:

**Repository: `uvisor`**

1. Add your up-to-date header files to `~/code/uvisor/core/arch/AWESOME42`. uVisor does not use any HAL APIs, so you should keep the header files to a minimum, providing only basic symbols and macros for the platform. If your header files are not hierarchical yet, we strongly suggest that you use symbols to allow `$(FAMILY)`-specific header files to be conditionally included in a wider `$(ARCH)`-specific header file. This folder also needs two files, to define interrupt handlers prototypes (`inc/system.h`) and to implement them as a unique default handler (`src/system.c`, using `UVISOR_ALIAS(...)`.

2. Add the file `~/code/uvisor/core/linker/AWESOME42.h`, containing address boundaries for SRAM and Flash (symbols `{FLASH, SRAM}_{ORIGIN, LENGTH}`). These symbols are used to relocate uVisor to a specific address (Flash) and to protect the uVisor stack and BSS (SRAM). Please note that if you have multiple memories that could be SRAM/Flash, specify the symbols for those where you expect uVisor to be. You can write scripts that auto-generate this file depending on the actual device used, but please remember that you should not rely on any additional preprocessing, as this file is used as input for a linker script and some symbols might be incompatible (for example, using numbers ending in `UL`); only use numbers explicitly.

3. Conditionally include your platform-specific header files in `uvisor/core/uvisor-device.h`. For consistency, use a symbol that resembles the architecture name (`#if defined(AWESOME42)`); this symbol will be defined at compile time from the platform-specific Makefile. If you followed the suggestions from point 1, you should only need to include one `$(ARCH)`-specific file, which in turn includes other files hierarchically.

4. Add a folder with the hardware-specific code: `mkdir ~/code/uvisor/awesome42`. This folder contains hardware-specific configurations and function implementations. All functions are declared in the hardware-independent uVisor codebase, so a missing implementation leads to a link-time error.
    - The `Makefile` contains definitions that are used in the main `Makefile.rules` script (see Table 1); you can also add custom rules if you like. You will always perform the complete build process from `~/code/uvisor/$ARCH/uvisor/`.
    - The `src` folder contains the implementation of functions that are hardware-specific. Currently this includes only one function, `void halt_led(THaltError reason)`, that blinks the on-platform LED (a red LED, if available) with a pattern that depends on the error enumeration. This mechanism is used to give the user a quick feedback on the cause of failure that halted the system (on uVisor's decision).
    - The `debug` folder contains the implementation of hardware-specific debugging functions. Currently this include only one function, `const MemMap *memory_map_name(uint32_t addr)`, that maps an address to a platform memory map object. `MemMap` objects are `struct`s that contain a short string identifying the peripheral and its start and end addresses. This information is crucial for the debugging process, so that access faults can be extensively documented.
    - The `config` folder contains a single header file, `config.h`, that is expected at compile time. This header defines hardware characteristics that are used by the uVisor linker script (see Table 2).

---

| Symbol       | Description                                                                   |
|--------------|-------------------------------------------------------------------------------|
| `PROJECT`    | Project name                                                                  |
| `ARCH`       | Architecture of your device (`awesome42`)                                     |
| `FAMILY`     | Family of your device (`XX`)                                                  |
| `CHIP`       | Your chip (`YYYYY`)                                                           |
| `ARCH_MPU`   | MPU architecture (default `ARMv7M`); some vendors implemented their own       |
| `MCU`        | Full MCU name of your device (`awesome42-XX-YYYYY`)                           |
| `MCU_JLINK`  | Full MCU name as used by JLink                                                |
| `CONFIG`     | Custom definitions or configurations to pass with `CFLAGS`                    |
| `APP_CLEAN`  | Custom additional clean script for this platform                              |
| `APP_CFLAGS` | Custom additional `CFLAGS` for this platform; include additional search paths |
| `APP_SRC`    | Hardware-specific source files to include in compilation                      |
**Table 1**. Hardware-specific `Makefile` rules and symbols

---

| Symbol             | Description                                                         |
|--------------------|---------------------------------------------------------------------|
| `RESERVED_FLASH`   | Offset of uVisor code (uVisor entry point in mbed)                  |
| `RESERVED_SRAM`    | Offset of uVisor own memories (not the secure boxes' memories)      |
| `USE_FLASH_SIZE`   | Maximum flash usage (allows link time checks)                       |
| `USE_SRAM_SIZE`    | Maximum SRAM usage (allows link time checks)                        |
| `STACK_SIZE`       | uVisor own stack size (not the secure boxes' stacks)                |
| `STACK_GUARD_BAND` | Size of the stack guard bands (usually depends on MPU architecture) |
**Table 2**. Hardware-specific configuration symbols

Making Your Modules uVisor-Aware
--------------------------------
[Go to top](#overview)

When you've ported all of the uVisor code to your new `awesome42-XX-YYYYY` platform, you can finally build uVisor:
`make clean all`

You are only a few steps away from creating your local `uvisor-lib` release, but first we need to make the mbed codebase aware of the presence of uVisor. This involves the porting of two additional files: the startup code and the linker script.

---

**Startup code**

This usually lives in an `mbed-hal-vendor-platform` module, if you already ported your platform to mbed. The startup code must call the function `uvisor_init()` right after system initialization (usually called `SystemInit()`) and right before the C/C++ library initialization.

In assembly, `startup.S` (GCC syntax):
```asm
ResetHandler:
  ...
  ldr r0, =SystemInit
  blx r0
  ldr r0, =uvisor_init    [*] insert this
  blx r0                  [*] insert this
  ldr r0, =__start
  bx  r0
```

In C, `startup.c`:
```C
void __attribute__((noreturn)) ResetHandler(void) {
  ...
  SystemInit();
  uvisor_init();
  __start()
}
```

---

**Linker script**

To enforce the security model, we need to place uVisor at a specific location in memory, and give uVisor specific information about the rest of the memory map. uVisor needs some memory sections in which to place protected resources. These symbols live in the ld linker script or in the armlink scatter file.

uVisor expects a distinction between at least two memories: Flash and SRAM. If more than one memory fits this description, you should make an architectural decision about where to put uVisor. We suggest that if you have fast memories you should use those, provided that they offer security features at least equal to those of the regular memories.

Please look at [this example](https://github.com/ARMmbed/target-frdm-k64f-gcc/blob/master/ld/K64FN1M0xxx12.ld) to get an idea of what the linker script should look like (taken from the Freescale FRDM-K64F board, built with GCC ARM). You can see that you need the following regions:

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
        Holds the monolithic uVisor binary; its location determines the entry point of uVisor, since <code>uvisor_init</code> points to the first opcode in this binary blob. You should make sure that the linker script has this symbol at the same offset that you specified in <code>RESERVED_FLASH</code> in <code>~/code/uvisor/awesome42/config/config.h</code>. This can be also done implicitly, for example if you know that uVisor will be positioned after the vector table and you know the size of the vector table.
      </td>
    </tr>
    <tr>
      <td>
        <code>.uvisor.bss</code>
      </td>
      <td>
        Contains both uVisor's own memories (coming from <code>uvisor-lib</code>, in <code>.keep.uvisor.bss.main</code>) and the secure boxes' protected memories (in <code>.keep.uvisor.bss.boxes</code>). You must make sure that this region (and hence its first sub-region, <code>.keep.uvisor.bss.boxes</code>) is positioned in SRAM at the same offset that you specified in <code>RESERVED_SRAM</code> in <code>~/code/uvisor/awesome42/config/config.h</code>. This can also be done implicitly, for example if you know that this section will be positioned right at the beginning of SRAM. Please note that to avoid having data loaded from Flash ending up before uVisor, we strongly suggest to put this section at the top of the linker script.
      </td>
    </tr>
    <tr>
      <td>
        <code>.uvisor.secure</code>
      </td>
      <td>
        Contains constant data in Flash that describes the configuration of the secure boxes. It comprises the configuration tables (<code>.keep.uvisor.cfgtbl</code>) and the pointers to them (<code>.keep.uvisor.cfgtbl_ptr[_first]</code>), which help with boxes enumeration.
      </td>
    </tr>
  </tbody>
</table>

All these sections and their sub-regions must be preceded and followed by symbols that describe their boundaries. Please follow this [golden example](https://github.com/ARMmbed/target-frdm-k64f-gcc/blob/master/ld/K64FN1M0xxx12.ld) to make sure that you define all symbols, as well as the specified alignment. All symbols prefixed with `.keep` must end up in the final memory map even if they are not explicitly used. Please make sure to use the correct ld commands (`KEEP(...)`) or the relevant armcc flag (`--keep=*(.keep*)`).

Once uVisor is active it maintains its own vector table, which the rest of the code can only interact with through uVisor APIs. We still suggest to leave the standard vector table in Flash, so that if uVisor is disabled the classic `NVIC_{S,G}etVector` functions can still work and relocate it to SRAM. In general, you can assume that if uVisor is included as a binary blob in your platform but is disabled, the code is still compatible and can run without problems.

If you are using armcc, then you have to re-create the same memory map in a scatter file. Additional armlink flags are needed to ensure important symbols are not discarded, and an additional steering file translates armcc-compatible linker symbols to `__uvisor`-namespaced ones. **More on this coming soon**.

Building with yotta
-------------------
[Go to top](#overview)

Now that your codebase is ready to accommodate and interact with uVisor, you need to explicitly specify that your platform is uVisor-enabled and then, finally, build your local `uvisor-lib` release.

You need to modify two files:

1. `~/code/uvisor/core/mbed/uvisor-lib/platforms.h`

You just need to include a condition similar to `... defined(TARGET_LIKE_AWESOME42_1234X_GCC` in the preprocessor macro that conditionally defines `UVISOR_PRESENT`. The ``uvisor-lib`` CMakeLists.txt file uses this symbol to determine which headers to include with `uvisor-lib` (we provide different macros as a fallback for unsupported platforms). yotta automatically generates symbols in the form of `TARGET_LIKE_*`. If you ported uVisor for the whole `awesome42` architecture, you might want to be less restrictive and use a symbol like `TARGET_LIKE_AWESOME42`, which automatically enables uVisor for all devices in that family. Do not specify any toolchain-dependent condition, as this applies to the file described next. We believe you don't need to specify this here, because uVisor is only pre-built with GCC ARM and then released as a toolchain-independent object file.

2. `~/code/uvisor/release/source/CMakeLists.txt`

This file also defines a `UVISOR_PRESENT` variable that is used to determine which files to compile. The criteria is very similar to the one used in `platforms.h`. Here you should also specify the toolchain compatibility. To add your platform, append a condition like the following:
```
...
elseif(TARGET_LIKE_AWESOME42)                 # in this example we enable the whole architecture class
    if(TARGET_LIKE_GCC)
        set(UVISOR_PRESENT true)
        set(UVISOR_DIR "AWESOME42")           # awesome42 architecture
    endif()
...
```

---

Finally, you can release your local version of `uvisor-lib`:
```bash
cd #/code/uvisor/awesome42/uvisor
make clean release
```
The `release` rule will carry out the release process, which generates object files for the `uvisor-lib` yotta module. This is the form in which uVisor will be included in the mbed codebase. The binary blob ensures that uVisor is shipped as a monolithic piece of code, which helps with integrity verification and ensures all functions and libraries called by uVisor are trusted.

For your application to use this local version instead of the one hosted in the public yotta registry, we need to create what is called a yotta link: a symbolic link that tells a yotta application to use a local version of a given module. With a yotta link all modules and their dependencies will use your version of `uvisor-lib`; if this leads to some versions mismatch, please contact the module owner to see if dependencies can be updated for that module.

Starting from the newly created `uvisor-lib` release:
```bash
cd ~/code/uvisor/release
yotta link
```
This creates a symbolic link in your yotta system folder. From the Hello World application:
```bash
cd ~/code/uvisor-helloworld
yotta target awesome42-XX-YYYYY-gcc
yotta link uvisor-lib
yotta install
```
You should now see yotta fetching and installing modules from the public registry. If you do:
```bash
yotta ls -a
```
You should see a tree representing all dependencies; `uvisor-lib` should be listed with a local path next to its name, meaning that your local release has been used. If you see a dependency mismatch compilation and linking might still work, although results are not guaranteed. yotta will always return a failure value to the shell in these cases.

Build the Hello World application:
```bash
yotta build
```
and copy the resulting binary `~/code/uvisor-helloworld/build/awesome42-XX-YYYYY-gcc/source/uvisor-helloworld` to your platform's Flash. If the platform is already mbed-enabled, you can drag and drop it, otherwise you can use gdb. We provide a script (`Makefile` in `uvisor-helloworld`) that can do the job for you:
```bash
# open gdb server in another shell
make ARCH=AWESOME42 FAMILY=XX CHIP=YYYYY TOOL=gcc gdbserver
...
# start the gdb session
make ARCH=AWESOME42 FAMILY=XX CHIP=YYYYY TOOL=gcc gdb
```
This rule automatically loads the binary to Flash, although you need to modify the `Makefile` to work with your platform. This step requires the [Segger JLink tools](https://www.segger.com/jlink-debug-probes.html); your platform must be listed among the [supported ones](https://www.segger.com/jlink_supported_devices.html).

Debugging uVisor
----------------
[Go to top](#overview)

By default the `make release` rule generates an optimized version of uVisor that does not contain any debugging messages. Instead, a blinking LED delivers feedback to the user about possible faults (when the target platform supports it). The user can asses the sevrity of the error by the blinking pattern, as indicated in [this list](https://github.com/ARMmbed/uvisor-lib/blob/master/DOCUMENTATION.md#error-patterns), but can't directly identify the cause of problem.

You can build a debug version of uVisor that disables all compile-time optimizations and includes debugging messages:
```bash
make OPT= release
```
These messages are delivered through [semihosting](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0471l/pge1358787045051.html), and you must connect a debugger to see them. Semihosting is achieved through a special instruction, `BKPT`, which results in SVCalls when no debugger is connected; since SVCs are widely used by uVisor to perform security-critical operations, not connecting a debugger will make the system hang, making the debugging messages blocking.

If you want to observe uVisor debugging messages, make sure that your `uvisor-lib` local release is linked to your application, as described above. You can then reuse the same rules:
```bash
# open gdb server in another shell
make ARCH=AWESOME42 FAMILY=XX CHIP=YYYYY  TOOL=gcc gdbserver
...
# start the gdb session
make ARCH=AWESOME42 FAMILY=XX CHIP=YYYYY  TOOL=gcc gdb
```
Remember that the `gdb` rule loads the binary to Flash automatically. You can use netcat, or any other similar program, to observe the output on port 2333:
```bash
nc localhost 2333
```

Currently there is only one level of verbosity: the highest one. What you probably will be mostly interested in are the *blue screens* that are outputted when a fault occurs. They provide many pieces of information about the state of the program prior to the fault, the reason of the fault and a possible memory section that triggered it, if any. You can use the last bit of information to include a new ACL in your application or to check the validity of the access.

`FIXME`s in This Guide
----------------------
[Go to top](#overview)

Please report any new bug you found in this documentation as a `uvisor` [issue](https://github.com/ARMmbed/uvisor/issues/new).

Known issues:
- ~~Most links are broken.~~
- ~~Missing porting instructions for `uvisor/arch/FAMILY/*/system.{c,h}`.~~
- ~~Add section on debugging.~~
- ~~Clarify semihosting features.~~
- Add information on coding style.
- Long names are not rendered correctly in tables (unwanted newlines).


