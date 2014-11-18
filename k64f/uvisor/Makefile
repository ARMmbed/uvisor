PROJECT:=k64f_uvisor
ARCH:=MK64F
CPU:=$(ARCH)N1M0VLL12

# mbed libray files
TARGET_S:=mbed/uvisor-GCC_ARM.s
TARGET_BIN:=mbed/uvisor-$(CPU).bin
TARGET_LIB:=mbed/uvisor.a

APP_CLEAN:=$(TARGET_LIB) $(TARGET_S) $(TARGET_BIN) $(mbed/uvisor.a)
APP_CFLAGS:=-DUVISOR -Iconfig -Isrc
APP_SRC:=\
	src/vmpu.c \
	src/main.c

include ../../core/Makefile.rules

release: mbed/uvisor-gcc-input.S $(PROJECT).bin
	cp $(PROJECT).bin $(TARGET_BIN)
	$(CPP) -w -P -I$(CORE) $(APP_CFLAGS) -D$(CPU) -D$(ARCH) -DTARGET_BIN=\"$(TARGET_BIN)\" $< -o$(TARGET_S)

lib: mbed/uvisor.o
	$(AR) rcs $(TARGET_LIB) $<

mbed/uvisor.o: release $(TARGET_S)
	$(CC) $(CFLAGS) -c -o $@ $(TARGET_S)
