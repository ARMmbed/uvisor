PROJECT:=k64f_uvisor
ARCH:=MK64F
ARCH_MPU:=$(ARCH)
CPU:=$(ARCH)N1M0XXX12
CONFIG:=

APP_CLEAN:=
APP_CFLAGS:=-DUVISOR $(CONFIG) -Iconfig -Isrc -Ilib -Idebug
APP_SRC:=\
	src/k64f_halt.c \
	debug/k64f_memory_map.c \

include ../../core/Makefile.rules
