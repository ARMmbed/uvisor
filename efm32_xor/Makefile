PROJECT:=efm32_box
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024

APP_DEFINES:=-DLIB_CLIENT
APP_CFLAGS:=-Iconfig -Isrc $(APP_DEFINES)
APP_SRC:=\
	src/main.c

include ../core/Makefile.rules

# override flash offset to locate firmware after uVisor
FLASH_OFFSET:=0x40000
