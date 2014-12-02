PROJECT:=k64f_uvisor
ARCH:=MK64F
CPU:=$(ARCH)N1M0XXX12
CONFIG:=-DNOSYSTEM

# mbed libray files
UVISOR_BIN:=uvisor-$(CPU).box
TARGET_DIR:=../../release/source/$(CPU)
TARGET_S:=$(TARGET_DIR)/uvisor-GCC_ARM.s
TARGET_BIN:=$(TARGET_DIR)/$(UVISOR_BIN)
TARGET_VER:=$(TARGET_DIR)/version.txt

APP_CLEAN:=$(TARGET_LIB) $(TARGET_S) $(TARGET_BIN) $(TARGET_VER) $(mbed/uvisor.a)
APP_CFLAGS:=-DUVISOR $(CONFIG) -Iconfig -Isrc
APP_SRC:=\
	src/svc.c \
	src/vmpu.c \
	src/main.c

include ../../core/Makefile.rules

release: src/mbed/uvisor-gcc-input.S $(PROJECT).bin
	rm -rf $(TARGET_DIR)
	mkdir -p $(TARGET_DIR)
	echo "$(PROGRAM_VERSION)" > $(TARGET_VER)
	cp $(PROJECT).bin $(TARGET_BIN)
	$(CPP) -w -P -I$(CORE) $(APP_CFLAGS) -DP-D$(CPU) -D$(ARCH) -DTARGET_BIN=\"$(TARGET_BIN)\" $< -o$(TARGET_S)

test: release $(TARGET_S)
	$(CC) $(CFLAGS) -c -o $@ $(TARGET_S)
