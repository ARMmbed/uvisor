PROJECT:=efm32_boot
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024

APP_CFLAGS:=-Isrc -Isrc/helper
APP_SRC:=src/main.c \
         src/helper/usart.c

#APP_SRC:=src/autobaud.c \
#         src/boot.c \
#         src/crc.c \
#         src/debug.c \
#         src/debuglock.c \
#         src/flash.c \
#         src/usart.c \
#         src/xmodem.c

include ../core/Makefile.rules
