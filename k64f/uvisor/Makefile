PROJECT:=k64f_uvisor
ARCH:=MK64F
CPU:=$(ARCH)N1M0XXX12
CONFIG:=-DNOSYSTEM

# mbed libray files
UVISOR_BIN:=uvisor-$(CPU).box
TARGET_BASE:=../../release
TARGET_SRC:=$(TARGET_BASE)/source
TARGET_SRC_HW:=$(TARGET_SRC)/$(CPU)
TARGET_INC:=$(TARGET_BASE)/uvisor-lib
TARGET_S:=$(TARGET_SRC_HW)/uvisor-GCC_ARM.s
TARGET_BIN:=$(TARGET_SRC_HW)/$(UVISOR_BIN)
TARGET_VER:=$(TARGET_SRC_HW)/version.txt

APP_CLEAN:=$(TARGET_LIB) $(TARGET_S) $(TARGET_BIN) $(TARGET_VER)
APP_CFLAGS:=-DUVISOR $(CONFIG) -Iconfig -Isrc -Ilib -Idebug
APP_SRC:=\
	src/halt.c \
	src/svc.c \
	src/svc_cx.c \
	src/vmpu.c \
	src/main.c \
	debug/debug.c \
	debug/memory_map.c \

include ../../core/Makefile.rules

release: lib/source/uvisor-gcc-input.S $(PROJECT).bin
	rm -rf $(TARGET_SRC_HW)
	rm -f $(TARGET_INC)/*.h
	rm -f $(TARGET_SRC)/*.cpp
	mkdir -p $(TARGET_SRC_HW)
	echo "$(PROGRAM_VERSION)" > $(TARGET_VER)
	cp $(PROJECT).bin $(TARGET_BIN)
	cp lib/uvisor-lib/*.h $(TARGET_INC)/
	cp lib/source/*.cpp $(TARGET_SRC)/
	find ../.. -name "*_exports.h" -not -path "$(TARGET_BASE)/*"\
	     -exec cp {} $(TARGET_INC)/ \;
	cp -f lib/source/uvisor-gcc-header.S $(TARGET_S)
	$(CPP) -w -P -I$(CORE) $(APP_CFLAGS) -DP-D$(CPU) -D$(ARCH)\
	     -DTARGET_BIN=\"$(UVISOR_BIN)\" $< >> $(TARGET_S)

test: release $(TARGET_S)
	$(CC) $(CFLAGS) -c -o $@ $(TARGET_S)
