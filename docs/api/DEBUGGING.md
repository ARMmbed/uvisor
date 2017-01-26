# Debugging uVisor on mbed OS

The uVisor is distributed as a pre-linked binary blob. Blobs for different mbed platforms are released in the mbed OS repository, [ARMmbed/mbed-os](https://github.com/ARMmbed/mbed-os), and are linked to your application when you build it. Two classes of binary blobs are released for each version — one for release and one for debug.

If you only want to use the uVisor debug features on an already supported platform, you do not need to clone it and build it locally. If you instead want to make modifications to uVisor (or port it to your platform) and test the modifications locally with your app, please follow the [Developing with uVisor Locally on mbed OS](../core/DEVELOPING_LOCALLY.md) guide first.

In this quick guide we will show you how to enable the default debug features on uVisor, and how to instrument it to get even more debug information. You will need the following:

* A GDB-enabled board (and the related tools).
* A [target supported](../../README.md#supported-platforms) by uVisor on mbed OS. If your target is not supported yet, you can follow the [uVisor Porting Guide for mbed OS](../core/PORTING.md).
* The Launchpad [GCC ARM Embedded](https://launchpad.net/gcc-arm-embedded) toolchain.
* GNU make.
* git.

## Debug capabilities

The uVisor provides two main sources of debug information:

1. Runtime messages. These are delivered through [semihosting](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0471l/pge1358787045051.html), which requires a debugger to be connected to the device. Currently debug messages are used to instrument some of the security-critical features of uVisor, like boot and start-up configuration, interrupts management, and context switching. A post-mortem screen is also output when the system is halted due to a fault.

1. Debug box drivers. We call a *debug box* a secure box that registers with uVisor to handle debug events and messages. The uVisor provides a predefined function table that describes the driver and its capabilities. Different debug boxes can implement these handlers differently, independently from uVisor. All handlers are executed in unprivileged mode.

Runtime messages and debug box handlers are independent from each other. Even if an application does not include a debug box, the uVisor is still able to deliver basic runtime messages. Conversely, an application that includes a debug box will handle debug events even if the release build of uVisor is used and possibly even without a debugger connected.

## Enabling runtime messages

If you want to observe the uVisor runtime messages you need to have a debugger connected to your board. We will use our Hello World application for this guide, built for the NXP FRDM-K64F target:

```bash
$ cd ~/code
$ mbed import https://github.com/ARMmbed/mbed-os-example-uvisor
$ cd mbed-os-example-uvisor
```

Of course any other application can be used, provided that it is correctly set up to use uVisor. See [Developing with uVisor Locally on mbed OS](../core/DEVELOPING_LOCALLY.md) for more details.
Runtime messages are silenced by default. In order to enable them, you need to build your application linking to the debug version of uVisor. The uVisor libraries that we publish in mbed OS provide both the release and debug builds of uVisor, so you only need to run the following command:

```bash
$ mbed compile -m ${your_target} -t GCC_ARM -c --profile mbed-os/tools/profiles/debug.json
```

The `--profile mbed-os/tools/profiles/debug.json` option ensures that the build system enables debug symbols and disables optimizations. In addition, it ensures that the debug build of uVisor is selected, which enables the uVisor runtime messages.

Now start the GDB server. This step changes depending on which debugger you are using. In case you are using a J-Link debugger, run:

```bash
$ JLinkGDBServer -device ${device_name} -if ${interface}
```

In the command above `${device_name}` and `${interface}` are J-Link-specific. Please check out the [J-Link documentation](https://www.segger.com/admin/uploads/productDocs/UM08001_JLink.pdf) and the list of [supported devices](https://www.segger.com/jlink_supported_devices.html).

Time to flash the device! We will use GDB for that, even if your device allows drag & drop flashing. This allows us to potentially group several commands together into a start-up GDB script.

You need to connect to the GDB server for that. Here we use `arm-none-eabi-gdb`, which comes with the Launchpad GCC ARM Embedded toolchain. Other equivalent tools will work similarly but are not covered by this guide.

```
$ arm-none-eabi-gdb
GNU gdb (GNU Tools for ARM Embedded Processors) 7.8.0.20150604-cvs
Copyright (C) 2014 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
(gdb) ...
```

The following is the minimum set of commands you need to send to the device to flash and run the binary (replace '2331' by '3333' if you use pyOCD or OpenOCD as a debug server):

```bash
(gdb) target remote localhost:2331
(gdb) monitor endian little
(gdb) monitor reset
(gdb) monitor halt
(gdb) monitor semihosting enable
(gdb) monitor speed 1000
(gdb) monitor loadbin BUILD/${target}/${your_app}.bin 0
(gdb) monitor flash device = ${device_name}
(gdb) load BUILD/${target}/${your_app}.elf
(gdb) file BUILD/${target}/${your_app}.elf
```

From here on if you send the `c` command the program will run indefinitely. Of course you can configure other addresses/ports for the target. Please refer to the [GDB documentation](http://www.gnu.org/software/gdb/documentation/) for details on the GDB commands.

You can also group these commands in a script and pass it directly to `arm-none-eabi-gdb`:

```bash
$ arm-none-eabi-gdb -x gdb.script
```

You can observe the debug messages using `netcat` or any other equivalent program (replace '2333' by '4444' if you use pyOCD or OpenOCD as a debug server):

```bash
$ nc localhost 2333
```

Similarly to the GDB server, the port can be changed, if you want.

Currently the following messages are printed:

* Start-up and initialization routines.
* Runtime assertions (failed sanity checks).
* vIRQ operations: Registering, enabling, disabling, and releasing interrupts.
* Faults: For all type of faults a default blue screen is printed. It contains the following information:
    * MPU configurations.
    * Relevant fault status registers.
    * Faulting address.
    * Exception stack frame.
* Specific fault handlers might include additional information relevant to the fault.

### Limitations

* There is currently only one level of verbosity available, which prints all possible messages.

* Debug messages are functionally blocking, meaning that if uVisor runs with debug enabled and a debugger is not connected the system will halt waiting for a semihosting connection.

* Debug messages might interfere with timing constraints, as they are shown while running in the highest priority level. Applications that have very strict timing requirements might show some unexpected behaviour.

## The debug box

> **Warning**: The debug box feature is an early prototype. The APIs and procedures described here might change several times in non-backwards-compatible ways.

The uVisor code is instrumented to output debug information when it is relevant. In order to keep the uVisor as simple and hardware-independent as possible, some of this information is not handled and interpreted directly by uVisor.

Instead, debug events and messages are forwarded to a special unprivileged box, called a *debug box*. A debug box is configured just like any other secure box, but it registers with uVisor to handle debug callbacks. These callbacks must adhere to a format that is provided by uVisor, in the form of a  debug box driver.

The debug box driver is encoded in a standard table (a C `struct`) that must be populated by a debug box at initialization time. A debug box can decide to implement only some of the available handlers, although they must all exist at least as empty functions, otherwise the program behaviour might be unpredictable.

Currently, only one debug handler is provided. A debug box driver will always expect a `get_version()` handler in position 0 of the function table:

```C
typedef struct TUvisorDebugDriver {
  uint32_t (*get_version)(void);      /* 0. Return the implemented driver version. */
  void (*halt_error)(int);            /* 1. Halt on error. Reboot upon return. */
}
```

The following is an example of how to implement and configure a debug box.

```C
#include "mbed.h"
#include "uvisor-lib/uvisor-lib.h"

struct box_context {
    uint32_t unused;
};

static const UvisorBoxAclItem acl[] = {
};

static void box_debug_main(const void *);

/* Configure the debug box. */
UVISOR_BOX_NAMESPACE(NULL);
UVISOR_BOX_CONFIG(box_debug, UVISOR_BOX_STACK_SIZE);
UVISOR_BOX_MAIN(box_debug_main, osPriorityNormal, UVISOR_BOX_STACK_SIZE);
UVISOR_BOX_CONFIG(box_debug, acl, UVISOR_BOX_STACK_SIZE, box_context);

static uint32_t get_version(void) {
    return 0;
}

static void halt_error(int reason) {
    printf("We halted with reason %i\r\n", reason);
    /* We will now reboot. */
}

static void box_debug_main(const void *)
{
    /* Debug box driver -- Version 0 */
    static const TUvisorDebugDriver driver = {
        get_version,
        halt_error
    };

    /* Register the debug box with uVisor. */
    uvisor_debug_init(&driver);
}
```

## Platform-specific details

Currently the following platforms are officially supported by uVisor:

* [NXP FRDM-K64F](http://developer.mbed.org/platforms/FRDM-K64F/).
* [STMicorelectronics STM32F429I-DISCO](http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/PF259090).

This section provides details on how to enable debug on these platforms.

#### NXP FRDM-K64F

The board features both a GDB-enabled on-board USB controller and a JTAG port. If you want to use the on-board USB, you must make sure to have the latest bootloader with the OpenSDA v2.0 firmware.

* [OpenSDA bootloader](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/opensda-serial-and-debug-adapter:OPENSDA).
* [Instructions](https://developer.mbed.org/handbook/Firmware-FRDM-K64F) on how to re-flash the bootloader.

The OpenSDA port provides a debugger interface which is compatible with the Segger J-Link Lite debugger. This means that you can use the [Segger J-Link tools](https://www.segger.com/jlink-software.html) and use the examples provided in this guide.

If you use the JTAG port, instead, you will need to download the tools and drivers for the debugger of your choice.

#### STMicorelectronics STM32F429I-DISCO

The board provides both an on-board proprietary debugging port (ST-LINK) and a JTAG port. The latter is spread out across the GPIO pins on the board.

If you are using ST-LINK please refer to the [STMicorelectronics website](http://www.st.com/web/catalog/tools/FM146/CL1984/SC724/SS1677/PF251168?sc=internet/evalboard/product/251168.jsp) for information on the tools and drivers needed. Please note that this debugger has not been tested with uVisor.

If instead you want to connect your debugger to the JTAG port you must wire the needed pins to your connector. This [guide](https://www.segger.com/admin/uploads/evalBoardDocs/AN00015_ConnectingJLinkToSTM32F429Discovery.pdf) explains how to do that in details. The guide is specific to the J-Link connectors, but it should be easily generalized to other connectors.
