################################################################
#
#  This confidential and  proprietary  software may be used only
#  as authorised  by  a licensing  agreement  from  ARM  Limited
#
#             (C) COPYRIGHT 2013-2015 ARM Limited
#                    ALL RIGHTS RESERVED
#
#   The entire notice above must be reproduced on all authorised
#   copies and copies  may only be made to the  extent permitted
#   by a licensing agreement from ARM Limited.
#
################################################################
PROJECT:=stm32f4_uvisor
ARCH:=STM32F4
CPU:=$(ARCH)29xx
CONFIG:=-DNOSYSTEM
CPU_JLINK:=$(ARCH)29ZI

APP_CLEAN:=
APP_CFLAGS:=-DUVISOR $(CONFIG) -Iconfig -Isrc -Ilib -Idebug

APP_SRC:=\
	src/stm32f4_halt.c \
	src/stm32f4_svc.c \
	debug/stm32f4_debug.c \
	debug/stm32f4_memory_map.c \

include ../../core/Makefile.rules
