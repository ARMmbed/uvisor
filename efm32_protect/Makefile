PROJECT:=efm32_protect
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024

APP_CFLAGS:=-Iconfig -Isrc -Isrc/helper
APP_SRC:=\
	src/main.c \
	src/svc.c \
	src/debug.c

include ../core/Makefile.rules
