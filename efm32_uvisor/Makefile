PROJECT:=efm32_uvisor
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024

APP_CFLAGS:=-Iconfig -Isrc -Isrc/helper
APP_SRC:=\
	src/main.c \
	src/mpu.c \
	src/svc.c \
	src/debug.c

include ../core/Makefile.rules
