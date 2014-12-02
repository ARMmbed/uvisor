PROJECT:=k64f_uvisor
ARCH:=MK64F
CPU:=$(ARCH)N1M0XXX12
CONFIG:=-DNOSYSTEM

# mbed libray files
PREFIX_DIR:=../../release/
TARGET_DIR:=source/$(CPU)
TARGET_S:=$(TARGET_DIR)/uvisor-GCC_ARM.s
TARGET_BIN:=$(TARGET_DIR)/uvisor-$(CPU).box

APP_CLEAN:=$(TARGET_LIB) $(PREFIX_DIR)$(TARGET_S) $(PREFIX_DIR)$(TARGET_BIN) $(mbed/uvisor.a)
APP_CFLAGS:=-DUVISOR $(CONFIG) -Iconfig -Isrc
APP_SRC:=\
	src/svc.c \
	src/vmpu.c \
	src/main.c

include ../../core/Makefile.rules

release: src/mbed/uvisor-gcc-input.S $(PROJECT).bin
	rm -rf $(PREFIX_DIR)$(TARGET_DIR)
	mkdir -p $(PREFIX_DIR)$(TARGET_DIR)
	cp $(PROJECT).bin $(PREFIX_DIR)$(TARGET_BIN)
	$(CPP) -w -P -I$(CORE) $(APP_CFLAGS) -DP-D$(CPU) -D$(ARCH) -DTARGET_BIN=\"$(TARGET_BIN)\" $< -o$(PREFIX_DIR)$(TARGET_S)

test: release $(PREFIX_DIR)$(TARGET_S)
	$(CC) $(CFLAGS) -c -o $@ $(PREFIX_DIR)$(TARGET_S)
