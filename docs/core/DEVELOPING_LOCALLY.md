# Developing with uVisor locally on mbed OS

When an application uses uVisor, mbed OS ensures that the uVisor static libraries link with the rest of the program. These libraries include the user-facing uVisor APIs and a prelinked binary blob, the uVisor *core*.

It is the responsibility of the uVisor library owner to compile uVisor from its main repository, [ARMmbed/uvisor](https://github.com/ARMmbed/uvisor), and to distribute the resulting libraries to the target operating system. In mbed OS, we periodically compile the uVisor core and release it as a static library for each of the supported targets. These libraries are in the uVisor folder in the [ARMmbed/mbed-os](https://github.com/ARMmbed/mbed-os) repository.

This guide will show you a different way of obtaining the uVisor libraries, by building them locally.

There are several reasons to build uVisor locally:

- Reproduce a publicly released build.
- Make modifications and test them before contributing to uVisor on GitHub. Read the [contribution guidelines](../../CONTRIBUTING.md) for more details.
- Preview the latest uVisor features before they are packaged and released.
- Play and experiment with uVisor.

You will need:

- A [target](../../README.md#supported-platforms) uVisor supports on mbed OS. If your target is not supported yet, you can follow the [uVisor porting guide for mbed OS](PORTING.md).
- The Launchpad [GNU ARM Embedded](https://launchpad.net/gcc-arm-embedded) Toolchain.
- GNU Make.
- Git.

## Prepare your application

If you are just starting to look at uVisor and don't have your own application for mbed OS, you can use the example app:

```bash
$ cd ~/code
$ mbed import https://github.com/ARMmbed/mbed-os-example-uvisor
```

The command above will import all the example dependencies, as well, including the mbed OS codebase and the uVisor libraries.

If you already have an application that you want to use for this guide, make sure it is ready to work with uVisor enabled. Follow the [Getting started guide for uVisor on mbed OS](../api/QUICKSTART.md) for more details.

In either case, move to the app folder:

```bash
$ cd ~/code/${your_app}           # or ~/code/mbed-os-example-uvisor
```

## Build uVisor locally

By default, the version of mbed OS that you download when you import a program or clone the [ARMmbed/mbed-os](https://github.com/ARMmbed/mbed-os) repository contains a prelinked version of the uVisor core and public APIs. The uVisor module comes with an importer script, though, that you can run to download the latest uVisor version and compile it. You can run the importer script by running `make` from the app folder:

```bash
$ make -C ~/code/${your_app}/mbed-os/features/FEATURE_UVISOR/importer
```

The script:

- Fetches the `HEAD` of the release branch of uVisor.
- Compiles the downloaded version of uVisor for all targets and all configurations, in both debug and release mode.
- Copies the libraries and relevant header files from the uVisor repository to the mbed OS folders.

You now have built the uVisor libraries locally with the latest uVisor version.

## Build the application

Go back to the application folder:

```bash
$ cd ~/code/${your_app}
```

You now can build the application using the latest uVisor version:

```bash
$ mbed compile -m ${your_target} -t GCC_ARM
```

Whenever you want to make a change to uVisor, just run `make` again in the importer folder. The change automatically propagates to the application:

```bash
$ make -C ~/code/${your_app}/mbed-os/core/features/FEATURE_UVISOR/importer
```

Your application binary is located in `BUILD/${your_target}/GCC_ARM/${your_app}.bin`. You can drag and drop it to the board if it supports it or use an external debugger to flash the device.

## Debugging uVisor

You can also compile the application using the uVisor debug build:

```bash
$ mbed compile -m ${your_target} -t GCC_ARM --profile mbed-os/tools/profiles/debug.json
```

The uVisor debug build gives you access to runtime messages and fault blue screens, which are useful in understanding the uVisor protection mechanisms, but it requires a debugger to be connected to the board. Please read [Debugging uVisor on mbed OS](../api/DEBUGGING.md) for further details.