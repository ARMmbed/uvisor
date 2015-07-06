PROJECT:=k64f_uvisor
ARCH:=MK64F
CPU:=$(ARCH)N1M0XXX12
CONFIG:=-DNOSYSTEM

APP_CLEAN:=
APP_CFLAGS:=-DUVISOR $(CONFIG) -Iconfig -Isrc -Ilib -Idebug
APP_SRC:=\
	src/k64f_halt.c \
	src/k64f_svc.c \
	src/k64f_vmpu.c \
	src/k64f_vmpu_aips.c \
	src/k64f_vmpu_mem.c \
	debug/k64f_debug.c \
	debug/k64f_memory_map.c \

include ../../core/Makefile.rules
