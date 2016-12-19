# Developing with uVisor Locally on mbed OS

When an application uses the uVisor, mbed OS ensures that the uVisor static libraries are linked with the rest of the program. These libraries include the user-facing uVisor APIs and a pre-linked binary blob, which we call the uVisor *core*.

It is the responsibility of the uVisor library owner to compile uVisor from its main repository, [ARMmbed/uvisor](https://github.com/ARMmbed/uvisor), and to distribute the resulting libraries to the target operating system. In mbed OS, we periodically compile the uVisor core and release it as a static library for each of the supported targets. These libraries are located in the uVisor folder in the [ARMmbed/mbed-os](https://github.com/ARMmbed/mbed-os) repository.

This guide will show you a different way of obtaining the uVisor libraries, that is, by building them locally.

There are several reasons why you might want to build uVisor locally:

* Reproduce a publicly released build.
* Make modifications and test them before contributing to uVisor on GitHub. Read our [contribution guidelines](../../CONTRIBUTING.md) for more details.
* Preview the latest uVisor features before they are packaged and released.
* Play and experiment with uVisor.

You will need the following:

* A [target supported](../../README.md#supported-platforms) by uVisor on mbed OS. If your target is not supported yet, you can follow the [uVisor Porting Guide for mbed OS](PORTING.md).
* The Launchpad [GCC ARM Embedded](https://launchpad.net/gcc-arm-embedded) toolchain.
* GNU make.
* git.

## Prepare your application

If you are just starting to look at uVisor and don't have your own application for mbed OS, we suggest you use our example app:

```bash
$ cd ~/code
$ mbed import https://github.com/ARMmbed/mbed-os-example-uvisor
```

The command above will import all the example dependencies as well, including the mbed OS codebase and the uVisor libraries.

If you already have an application that you want to use for this guide, make sure that it is ready to work with uVisor enabled. You can follow the [Quick-Start Guide for uVisor on mbed OS](../api/QUICKSTART.md) for more details.

In either case, move to the app folder:

```bash
$ cd ~/code/${your_app}           # or ~/code/mbed-os-example-uvisor
```

## Build the uVisor locally

By default, the version of mbed OS that you download when you import a program or clone the [ARMmbed/mbed-os](https://github.com/ARMmbed/mbed-os) repository contains a pre-linked version of the uVisor core and public APIs. The uVisor module comes with an importer script, though, that can be run to download the latest uVisor version and compile it.

You can run the importer script by calling running `make` from the app folder:

```bash
$ make -C ~/code/${your_app}/mbed-os/features/FEATURE_UVISOR/importer
```

The script will perform the following operations:

* It fetches the `HEAD` of the release branch of uVisor.
* It compiles the downloaded version of uVisor for all targets, all configurations, in both debug and release mode.
* It copies the libraries and relevant header files from the uVisor repository to the mbed OS folders.

Your local version of the uVisor libraries is now built locally using the latest uVisor version.

## Build the application

Go back to the application folder:

```bash
$ cd ~/code/${your_app}
```

You can now build the application using the latest uVisor version:

```bash
$ mbed compile -m ${your_target} -t GCC_ARM
```

Whenever you want to make a change to uVisor, just run `make` again in the importer folder. The change will be automatically propagated to the application:

```bash
$ make -C ~/code/${your_app}/mbed-os/core/features/FEATURE_UVISOR/importer
```

Your application binary will be located in `BUILD/${your_target}/GCC_ARM/${your_app}.bin`. You can drag & drop it to the board if it supports it, or use an external debugger to flash the device.

## Debugging uVisor

You can also compile the application using the uVisor debug build:

```bash
$ mbed compile -m ${your_target} -t GCC_ARM --profile mbed-os/tools/profiles/debug.json
```

The uVisor debug build gives you access to runtime messages and fault blue screens, which are very useful to understand the uVisor protection mechanisms, but it requires a debugger to be connected to the board. Please read the [Debugging uVisor on mbed OS](../api/DEBUGGING.md) guide for further details.
