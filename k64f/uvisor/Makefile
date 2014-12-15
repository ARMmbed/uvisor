PROJECT:=k64f_uvisor
ARCH:=MK64F
CPU:=$(ARCH)N1M0XXX12
CONFIG:=-DNOSYSTEM

# mbed libray files
UVISOR_BIN:=uvisor-$(CPU).box
TARGET_BASE:=../../release
TARGET_DIR:=$(TARGET_BASE)/source/$(CPU)
TARGET_S:=$(TARGET_DIR)/uvisor-GCC_ARM.s
TARGET_BIN:=$(TARGET_DIR)/$(UVISOR_BIN)
TARGET_VER:=$(TARGET_DIR)/version.txt

APP_CLEAN:=$(TARGET_LIB) $(TARGET_S) $(TARGET_BIN) $(TARGET_VER)
APP_CFLAGS:=-DUVISOR $(CONFIG) -Iconfig -Isrc -Ilib
APP_SRC:=\
	src/svc.c \
	src/vmpu.c \
	src/main.c

include ../../core/Makefile.rules

release: lib/uvisor-gcc-input.S lib/uvisor-lib.h $(PROJECT).bin
	rm -rf $(TARGET_DIR)
	mkdir -p $(TARGET_DIR)
	echo "$(PROGRAM_VERSION)" > $(TARGET_VER)
	cp $(PROJECT).bin $(TARGET_BIN)
	cp lib/uvisor-lib.h $(TARGET_BASE)/uvisor-lib/uvisor-lib.h
	cp -f lib/uvisor-gcc-header.S $(TARGET_S)
	$(CPP) -w -P -I$(CORE) $(APP_CFLAGS) -DP-D$(CPU) -D$(ARCH) -DTARGET_BIN=\"$(UVISOR_BIN)\" $< >> $(TARGET_S)

test: release $(TARGET_S)
	$(CC) $(CFLAGS) -c -o $@ $(TARGET_S)
