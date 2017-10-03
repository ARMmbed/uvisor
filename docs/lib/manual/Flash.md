# Using nonvolatile storage from uVisor on Mbed OS

This guide will help you to read and write from nonvolatile storage in a secure box protected by uVisor on Mbed OS 5. If you haven't done so, first [set up an example uVisor program](../QUICKSTART.md). This guide is applicable for the NXP FRDM-K64F development board, but the same principles apply to other development boards.

## Adding the FreescaleIAP library

A quick way to find any issues with running your code under uVisor is by writing code as if uVisor is not present. If any issues arise, uVisor will print them out during [startup](../DEBUGGING.md). The de-facto library to deal with the nonvolatile storage on Kinetes boards is the [FreescaleIAP](https://developer.mbed.org/users/Sissors/code/FreescaleIAP/) library by [Erik Olieman](https://developer.mbed.org/users/Sissors/). Add this library by running the following command from your project folder:

```bash
$ mbed add https://developer.mbed.org/users/Sissors/code/FreescaleIAP/
```

Now use the library from the main function of your secure box:

```cpp
#include "FreescaleIAP.h"

typedef struct {
    int magic;
    char tag[3];
} MyFlashStorage;

/* Main thread for the secure box */
static void private_button_main_thread(const void *)
{
    /* Use the last block of the flash */
    int address = flash_size() - SECTOR_SIZE;

    /* Data structure to be stored on flash */
    MyFlashStorage data;
    data.magic = 42;
    data.tag[0] = 0x6a;
    data.tag[1] = 0x61;
    data.tag[2] = 0x6e;

    /* Erase the sector first */
    erase_sector(address);
    /* And write the flash */
    program_flash(address, (char*)&data, sizeof(data));

    printf("Writing to flash OK...\n");

    /* Retrieving the data structure from flash */
    MyFlashStorage *data_from_flash = (MyFlashStorage *) address;

    printf("Read from flash. magic=%d, tag=%c%c%c\n", data_from_flash->magic,
        data_from_flash->tag[0], data_from_flash->tag[1], data_from_flash->tag[2]);

    // ... Rest of your code
```

When you run this program, uVisor immediately throws a bus fault during startup (for information on how to see uVisor log messages, see the [debugging](../DEBUGGING.md) page), because our box does not have access to the peripheral at address `0x40020007`.

```
***********************************************************
                    BUS FAULT
***********************************************************

* Active Box ID: 1
* FAULT SYNDROME REGISTERS

  CFSR: 0x00008200
  BFAR: 0x40020007
  --> PRECISERR: precise data access.

* No MPU violation found

* EXCEPTION STACK FRAME

  lr: 0xFFFFFFFD
  --> FP stack frame not saved.
  --> Exception from NP mode.
  --> Return to PSP.

  sp: 0x1FFF34D0
      sp[07]: 0x21000000 | xPSR
      sp[06]: 0x0000D6FE | pc
      sp[05]: 0x0000D6F1 | lr
      sp[04]: 0x00000000 | r12
      sp[03]: 0x40020000 | r3
      sp[02]: 0x00000009 | r2
      sp[01]: 0x00000000 | r1
      sp[00]: 0x00000000 | r0

HALT_ERROR(./core/vmpu/src/mpu_kinetis/vmpu_kinetis.c#166): Access to restricted resource denied.
```

## Configuring the ACL for the FTFE driver

To find out which peripheral is responsible for this fault, you can look in the [peripheral access layer](https://github.com/ARMmbed/mbed-os/blob/e5ba1d2/targets/TARGET_Freescale/TARGET_MCUXpresso_MCUS/TARGET_MCU_K64F/device/MK64F12.h) for your development board. If you search here for `40020000` - peripherals are normally aligned - you find the `FTFE_BASE` address, which is the flash driver for the K64F.

```cpp
/** Peripheral FTFE base address */
#define FTFE_BASE                                (0x40020000u)
```

You can enable this peripheral for the secure box by adding the following definition to the ACL list:

```cpp
    {FTFE,             sizeof(*FTFE),  UVISOR_TACLDEF_PERIPH},
```

After recompiling the code, you can now see - over the serial port - that you can successfully write to flash. However, uVisor still halts when trying to read from flash.

```
***********************************************************
                    BUS FAULT
***********************************************************

* Active Box ID: 1
* FAULT SYNDROME REGISTERS

  CFSR: 0x00008200
  BFAR: 0x000FF000
  --> PRECISERR: precise data access.

* MPU FAULT:
  Slave port:       0
  Address:          0x000FF000
  Faulting regions:
    R00: 0x00000000 0xFFFFFFFF 0x000007D0 0x00000001
  Master port:      0
  Error attribute:  Data READ (user mode)

HALT_ERROR(./core/vmpu/src/kinetis/vmpu_kinetis.c#144): Access to restricted resource denied
```

## Configuring the ACL to read from flash

As you can see from the log message, the MPU prevents reading from address `0x000FF000`. This is the value of the `address` you try to read. The K64F uses a memory-mapped address space to access flash storage, which is mapped from `0x0` to `0x100000`, and you're using a sector size of `0x1000` bytes. To get around this you can add this specific part of the address space to the ACL list. A nice addition is that you can limit boxes to only part of the flash storage. It's not all or nothing.

To allow reading from the last sector of flash storage, add the following definition to the ACL list of the secure box:

```cpp
    {(void *)0xff000,   SECTOR_SIZE,    UVISOR_TACL_ACCESS | UVISOR_TACL_SHARED},
```

When you recompile, the program runs to completion, and you can both read and write to flash from your secure box.

## Conclusion

uVisor can be used not just for isolating memory between boxes, or to block usage of certain peripherals. You can also use uVisor to block off reading parts of the flash storage from unauthorized boxes. Unfortunately, because writing to flash goes through the FTFE peripheral, you do not have the same fine-grained control over writing to flash. You can solve this by adding a specific secure box which owns the FTFE peripheral, and use a [public secure entry point](../QUICKSTART.md#expose-public-secure-entry-points-to-the-secure-box) to control which boxes can write to (specific parts of) flash.
