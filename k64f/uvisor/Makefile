PROJECT:=k64f_uvisor
ARCH:=MK64F
CPU:=$(ARCH)N1M0VLL12
TARGET_LIB:=mbed.a

APP_CLEAN:=$(TARGET_LIB)
APP_CFLAGS:=-DUVISOR -Iconfig -Isrc
APP_SRC:=\
	src/main.c

include ../../core/Makefile.rules

$(TARGET_LIB): mbed.o
	$(AR) rcs $@ $<

mbed.o: $(PROJECT).bin
	$(CC) $(CFLAGS) -c -o $@ mbed/uvisor.S
