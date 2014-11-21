PROJECT:=efm32_uvisor
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024
JLINK_SWO_PARAM:=-cpufreq 14000000

APP_DEFINES:=-DUVISOR
APP_CFLAGS:=-Iconfig -Isrc -Isrc/helper $(APP_DEFINES)
APP_SRC:=\
	src/main.c \
	src/mpu.c \
	src/svc.c

include ../../core/Makefile.rules
