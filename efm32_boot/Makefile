PROJECT:=efm32_boot
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024

APP_CFLAGS:=-Iconfig -Isrc -Isrc/helper
APP_SRC:=\
	src/main.c \
	src/helper/autobaud.c \
	src/helper/boot.c \
	src/helper/crc.c \
	src/helper/debug.c \
	src/helper/debuglock.c \
	src/helper/flash.c \
	src/helper/usart.c \
	src/helper/xmodem.c

include ../core/Makefile.rules
