# Developing with uVisor locally in mbed OS

The uVisor is distributed as a pre-linked binary blob. Blobs for different mbed platforms are released in a yotta module called `uvisor-lib`, which you can obtain from the yotta registry. This guide will show you a different way of obtaining those binaries, that is, by building them locally.

There are several reasons why you might want to build uVisor locally:
* Reproduce a publicly released build.
* Make modifications and test them before contributing to uVisor on GitHub. See [Contributing to uVisor](../CONTRIBUTING.md) for more details.
* Preview the latest uVisor features before they are packaged and released.
* Play and experiment with uVisor.

You will need the following:
* The Launchpad GCC ARM embedded toolchain.
* GNU make.
* yotta.
* git.

## Build the uVisor locally

We will assume that your development directory is `~/code`. First, you need to clone the `uvisor-lib` repository:

```bash
$ cd ~/code
$ git clone --recursive git@github.com:ARMmbed/uvisor-lib.git
```

The `--recursive` option ensures that the [ARMmbed/uvisor](https://github.com/ARMmbed/uvisor) submodule is fetched as well. This repository contains the uVisor core code-base and the uVisor APIs.

Build uVisor. All configurations, for all family, both for debug and release will be built with a single `make` rule:

```bash
$ cd ~/code/uvisor-lib/uvisor
$ make
```

A rule `make fresh` is also provided, that cleans everything before building. This might be useful while you develop.

New release libraries have been created, which are located in the `api/lib` folder inside `~/code/uvisor-lib/uvisor`. We only need to tell yotta to link locally to this module.

```bash
$ cd ~/code/uvisor-lib
$ yotta link
```

In this way yotta applications can refer to this local version of `uvisor-lib` instead of the one published in the registry. This version uses the locally built uVisor libraries.


## Link your application

If you are just starting to look at uVisor and don't have your own mbed app, we suggest you use our Hello World example:

```bash
$ cd ~/code
$ git clone git@github.com:ARMmbed/uvisor-helloworld.git
```

In either case, move to the app folder and select the target:

```bash
$ cd ~/code/${your_app}           # or ~/code/uvisor-helloworld
$ yotta target ${your_target}
```

If you are porting uVisor to your platform, `${your_target}` will be your own target, whereas otherwise you can only use the [officially supported targets](../README.md#supported-platforms) for uVisor.

You can now tell yotta to use your local version of uVisor, and finally build the application:

```bash
$ yotta link uvisor-lib
$ yotta build
```

Whenever you want to make a change to uVisor, just run `make` again in the `~/code/uvisor-lib/uvisor` folder. The change will be automatically propagated to this application through the locally linked version of `uvisor-lib`.

Your application binary will be located in `build/${your_target}/source/${your_app}.bin`. You can drag & drop it to the board if it supports it, or use an external debugger to flash the device.

## Debugging uVisor

The above procedure allows you to run a locally built version of uVisor on your device. If you want to enable debug you must build with the `-d` option:

```bash
$ yotta build -d
```

Debugging uVisor requires a debugger to be connected to the board. Please read the [Debugging uVisor](DEBUGGING.md) guide for further details.
