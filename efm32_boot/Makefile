PROJECT:=efm32gg_usart
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024
PREFIX:=arm-none-eabi
CC:=$(PREFIX)-gcc

CORE:=../core/
CFLAGS:=-I$(CORE) -I$(CORE)src -Iconfig

APP_SRC:=src/autobaud.c \
         src/boot.c \
         src/crc.c \
         src/debug.c \
         src/debuglock.c \
         src/flash.c \
         src/usart.c \
         src/xmodem.c

OBJS:=$(SOURCES:.c=.o)

all: $(PROJECT).bin

$(PROJECT).bin: $(OBJS)
	$(GCC) -o $@ $<

include ../core/Makefile.rules