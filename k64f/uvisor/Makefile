PROJECT:=k64f_uvisor
ARCH:=MK64F
CPU:=$(ARCH)N1M0VLL12

APP_CFLAGS:=-DUVISOR -Iconfig -Isrc
APP_SRC:=\
	src/main.c

include ../../core/Makefile.rules

mbed: mbed.a

mbed.a: mbed.o
	$(AR) rcs $@ $<

mbed.o: $(PROJECT).bin
	$(AS) -o $@ mbed/uvisor.S
