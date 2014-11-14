PROJECT:=k64f_uvisor
ARCH:=MK64F
CPU:=$(ARCH)N1M0VLL12

# mbed libray files
TARGET_S:=mbed/uvisor-gcc.s
TARGET_BIN:=mbed/uvisor.bin
TARGET_LIB:=mbed/uvisor.a

APP_CLEAN:=$(TARGET_LIB) $(TARGET_S) $(TARGET_BIN) $(mbed/uvisor.a)
APP_CFLAGS:=-DUVISOR -Iconfig -Isrc
APP_SRC:=\
	src/main.c

include ../../core/Makefile.rules

release: mbed/uvisor-input.S $(PROJECT).bin
	cp $(PROJECT).bin $(TARGET_BIN)
	$(CPP) -w -P -I$(CORE) $(APP_CFLAGS) -D$(CPU) -D$(ARCH) $< -o$(TARGET_S)

$(TARGET_LIB): mbed.o
	$(AR) rcs $@ $<

mbed.o: $(PROJECT).bin
	$(CC) $(CFLAGS) -c -o $@ mbed/uvisor.S
