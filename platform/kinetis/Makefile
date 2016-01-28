# Path of the current Makefile
CURRENT_PATH:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# The project name is given by the top-level platform-specific folder
PROJECT:=$(shell basename $(CURRENT_PATH))

ARCH:=MK64F
ARCH_MPU:=$(ARCH)
CPU:=$(ARCH)N1M0XXX12
CONFIG:=

INCLUDE_PATHS:=-I. -Iinc -Iarch/MK64F -Iarch/MK64F/inc

APP_CLEAN:=
APP_CFLAGS:=-DUVISOR $(CONFIG) $(INCLUDE_PATHS)
APP_SRC:=\
	src/k64f_halt.c \
	src/k64f_memory_map.c \
	arch/MK64F/src/system.c

include ../../core/Makefile.rules
