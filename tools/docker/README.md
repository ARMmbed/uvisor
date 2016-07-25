# mbed Build System for Docker

## Quick Installation
1. Please install the [latest docker version](https://docs.docker.com/engine/installation/) for your system. 
1. By typing `docker -i -t meriac/mbed` to run the lastest image. When typing this command the first time, docker will automatically update the latest version.

## Running the build system
For using the image, you need to establish a shared directory between your docker image and your host operating system. Please place your project sources into that directory, so they can be edited natively on the host using your favorite editor. Also the compiled images can be exchange using that shared directory. 

### Windows

### Linux and OSX
```bash
mkdir mbed
docker run -i -t -v $PWD/mbed:/home/mbed/shared meriac/mbed
```
In case you have SELinux enabled, you might have to delegate access to your shared directory using the ``:Z`` flag after the shared directory:
```bash
docker run -i -t -v $PWD/mbed:/home/mbed/shared:Z meriac/mbed
```

## Example Usage

### Remote access via SSH
```bash
# run new docker instance of mbed build tools
docker run --name mbed-ssh -d -p 22022:22 meriac/mbed-ssh
# get SSH password
docker logs mbed-ssh
    4f34dd2daef71e091f09c9a0421d57481d3697f84b11da15e9582ed2e6e5d27b
    docker logs mbed-ssh
    ssh password for user mbed:'QLT2uEop4Wol9RsPCD3I'
    Changing password for user mbed.
    passwd: all authentication tokens updated successfully.

# log into the machine locally using the password printed by 'logs'
ssh -p 22022:22 mbed@localhost
# consider making this port available from the outside
```
### Interactive
```txt
# start docker image
docker run -i -t meriac/mbed

# you get a command prompt into the build system: import your project first
[mbed@c27efbccf7df ~]$ mbed import https://github.com/ARMmbed/example-mbed-mallocator

    [mbed] Importing program "example-mbed-mallocator" from "https://github.com/ARMmbed/example-mbed-mallocator/" at latest revision in the current branch
    Username for 'https://github.com': meriac                                  
    Password for 'https://meriac@github.com': 
    [mbed] Adding library "mbed-os" from "https://github.com/ARMmbed/mbed-os/" at latest revision in the current branch
    [mbed] Adding library "mbed-os/mbed" from "https://github.com/mbedmicro/mbed/" at rev #52e93aebd083b679a8fe7b0e47039f138fa8c224
    [...]

# compile example
[mbed@c27efbccf7df ~]$ cd example-mbed-mallocator/
[mbed@c27efbccf7df ~]$ mbed compile -m K64F -t GCC_ARM -j0

Building project example-mbed-mallocator (K64F, GCC_ARM)
Compile: arc4.c
Compile: aesni.c
Compile: asn1parse.c
Compile: base64.c
[...]
Link: example-mbed-mallocator
Elf2Bin: example-mbed-mallocator
+-----------------------------------+-------+-------+------+
| Module                            | .text | .data | .bss |
+-----------------------------------+-------+-------+------+
| Fill                              |   69  |   0   | 2342 |
| Misc                              | 10578 |  112  | 208  |
| frameworks/greentea-client        |  968  |   28  |  44  |
| frameworks/utest                  |  2307 |   0   | 276  |
| mbed/hal                          | 21154 |   16  | 1128 |
| mbed/rtos                         |  6982 |   24  | 2542 |
| net/C027Interface                 |  6967 |   4   |  24  |
| net/LWIPInterface                 |   96  |   0   |  48  |
| net/atmel-rf-driver               |  320  |   0   | 216  |
| net/mbed-client                   |   64  |   0   |  12  |
| net/mbed-client-classic           |   64  |   0   |  12  |
| net/nanostack-hal-mbed-cmsis-rtos |  100  |   0   |  56  |
| Subtotals                         | 49669 |  184  | 6908 |
+-----------------------------------+-------+-------+------+
Static RAM memory (data + bss): 7092
Heap: 65540
Stack: 32768
Total RAM memory (data + bss + heap + stack): 105400
Total Flash memory (text + data + misc): 50893
Image: ./.build/K64F/GCC_ARM/example-mbed-mallocator.bin

[mbed@c27efbccf7df example-mbed-mallocator]$ _
```

