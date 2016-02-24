# Debugging uVisor

The uVisor is distributed as a pre-linked binary blob. Blobs for different mbed platforms are released in a yotta module called `uvisor-lib`, and are linked to your application when you build it. Two classes of binary blobs are released for each version — one for release and one for debug.

If you only want to use the uVisor debug features on an already supported platform, you do not need to clone it and build it locally. If you instead want to make modifications to uVisor (or port it to your platform) and test the modifications locally with your app, please follow the [Developing with uVisor locally](DEVELOPING_LOCALLY.md) guide first.

In this quick guide we will show you how to enable debug on uVisor. You will need the following:
* A GDB-enabled board (and the related tools).
* yotta.

## Debug your application

Debug messages are delivered through [semihosting](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0471l/pge1358787045051.html), which requires a debugger to be connected to the device.

We will assume from now on that you have a debugger connected to your board. We will use our Hello World application for this example, but of course any other can be used, provided that it is correctly setup to use uVisor. See [Developing with uVisor locally](DEVELOPING_LOCALLY.md) for more details.
```bash
$ cd ~/code
$ yotta target frdm-k64f-gcc
$ yotta install uvisor-helloworld
$ cd uvisor-helloworld
```

Build your application:
```bash
$ yotta build -d
```
The `-d` option ensures that yotta enables debug symbols and disables optimizations. In addition, the `uvisor-lib` modules selects the debug build of uVisor, which enables the uVisor debug features.

Now start the GDB server. This step changes depending on which debugger you are using. In case you are using a J-Link debugger, run:
```bash
$ JLinkGDBServer -device ${device_name} -if ${interface}
```
where `${device_name}` and `${interface}` are J-Link-specific. Please check out the [J-Link documentation](https://www.segger.com/admin/uploads/productDocs/UM08001_JLink.pdf) and the list of [supported devices](https://www.segger.com/jlink_supported_devices.html).

Time to flash the device! We will use GDB for that, even if your device allows drag & drop flashing. This allows us to potentially group several commands together into a start-up GDB script.

You need to connect to the GDB server for that. Here we use `arm-none-eabi-gdb`, which comes with the Launchpad GCC ARM Embedded toolchain. Other equival:w
ent tools will work similarly but are not covered by this guide.
```
$ arm-none-eabi-gdb
GNU gdb (GNU Tools for ARM Embedded Processors) 7.8.0.20150604-cvs
Copyright (C) 2014 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
(gdb) ...
```

The following is the minimum set of commands you need to send to the device to flash and run the binary:
```bash
(gdb) target remote localhost:2331
(gdb) monitor reset
(gdb) monitor halt
(gdb) monitor semihosting enable
(gdb) monitor loadbin ./build/${target}/source/${your_app}.bin 0
(gdb) monitor flash device = ${device_name}
(gdb) load ./build/${target}/source/${your_app}
(gdb) file ./build/${target}/source/${your_app}
```

From here on if you send the `c` command the program will run indefinitely. Of course you can configure other addresses/ports for the target. Please refer to the [GDB documentation](http://www.gnu.org/software/gdb/documentation/) for details on the GDB commands.

## Debugging messages

You can observe the debug messages using `netcat` or any other equivalent program:
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

## Platform-specific details

Currently the following platforms are supported by uVisor:
* [NXP FRDM-K64F](http://developer.mbed.org/platforms/FRDM-K64F/).
* [STMicorelectronics STM32F429I-DISCO](http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/PF259090).

This section provides details on how to enable debug on these platforms.

#### NXP FRDM-K64F

The board features both a GDB-enabled on-board USB controller and a JTAG port. If you want to use the on-board USB, you must make sure to have the latest bootloader with the OpenSDA v2.1 firmware.

* [OpenSDA bootloader](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/opensda-serial-and-debug-adapter:OPENSDA).
* [Instructions](https://developer.mbed.org/handbook/Firmware-FRDM-K64F) on how to re-flash the bootloader.

The OpenSDA port provides a debugger interface which is compatible with the Segger J-Link Lite debugger. This means that you can use the [Segger J-Link tools](https://www.segger.com/jlink-software.html) and use the examples provided in this guide.

If you use the JTAG port, instead, you will need to download the tools and drivers for the debugger of your choice.

#### STMicorelectronics STM32F429I-DISCO

The board provides both an on-board proprietary debugging port (ST-LINK) and a JTAG port. The latter is spread out across the GPIO pins on the board.

If you are using ST-LINK please refer to the [STMicorelectronics website](http://www.st.com/web/catalog/tools/FM146/CL1984/SC724/SS1677/PF251168?sc=internet/evalboard/product/251168.jsp) for information on the tools and drivers needed. Please note that this debugger has not been tested with uVisor.

If instead you want to connect your debugger to the JTAG port you must wire the needed pins to your connector. This [guide](https://www.segger.com/admin/uploads/evalBoardDocs/AN00015_ConnectingJLinkToSTM32F429Discovery.pdf) explains how to do that in details. The guide is specific to the J-Link connectors, but it should be easily generalized to other connectors.
