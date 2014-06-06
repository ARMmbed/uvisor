PROJECT:=efm32_box
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024

APP_DEFINES:=-DUVISOR_CLIENT
APP_CFLAGS:=-Iconfig -Isrc $(APP_DEFINES)
APP_SRC:=\
	src/main.c \
	src/debug.c

include ../core/Makefile.rules

# override flash offset to locate firmware after uVisor
FLASH_OFFSET:=$(UVISOR_FLASH_SIZE)
