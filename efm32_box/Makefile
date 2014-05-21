PROJECT:=efm32_box
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024

APP_DEFINES:=-DUVISOR_CLIENT
APP_CFLAGS:=-Iconfig -Isrc $(APP_DEFINES)
APP_SRC:=\
	src/main.c \
	src/debug.c

include ../core/Makefile.rules

# override flash offset
FLASH_OFFSET:=$(shell grep UVISOR_FLASH_SIZE $(CORE)/uvisor-config.h | sed 's/^.* \(0x[0-9a-f]\+\).*$\/\1/i')
