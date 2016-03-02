###########################################################################
#
#  Copyright (c) 2013-2016, ARM Limited, All Rights Reserved
#  SPDX-License-Identifier: Apache-2.0
#
#  Licensed under the Apache License, Version 2.0 (the "License"); you may
#  not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
###########################################################################
# Toolchain
PREFIX:=arm-none-eabi-
CC:=$(PREFIX)gcc
CXX:=$(PREFIX)g++
OBJCOPY:=$(PREFIX)objcopy
OBJDUMP:=$(PREFIX)objdump

# Root folder
ROOT_DIR:=.

# Top-level folder paths
CORE_DIR:=$(ROOT_DIR)/core
PLATFORM_DIR:=$(ROOT_DIR)/platform
RELEASE_DIR:=$(ROOT_DIR)/release
TOOLS_DIR:=$(ROOT_DIR)/tools

# Core paths
CMSIS_DIR:=$(CORE_DIR)/cmsis
SYSTEM_DIR:=$(CORE_DIR)/system
MBED_DIR:=$(CORE_DIR)/mbed
DEBUG_DIR:=$(CORE_DIR)/debug
LIB_DIR:=$(CORE_DIR)/lib

# List of supported platforms
# Note: One could do it in a simpler way but this prevents spurious files in
# $(PLATFORM_DIR) from getting picked.
PLATFORMS = $(notdir $(realpath $(dir $(wildcard $(PLATFORM_DIR)/*/))))

# List of uVisor configurations for the given platform
ifdef PLATFORM
include $(PLATFORM_DIR)/$(PLATFORM)/Makefile.configurations
endif

# Platform-specific paths
PLATFORM_INC:=$(PLATFORM_DIR)/$(PLATFORM)/inc
PLATFORM_SRC:=$(wildcard $(PLATFORM_DIR)/$(PLATFORM)/src/*.c)

# Configuration-specific paths
CONFIGURATION_LOWER:=$(shell echo $(CONFIGURATION) | tr '[:upper:]' '[:lower:]')
CONFIGURATION_PREFIX:=$(PLATFORM_DIR)/$(PLATFORM)/$(BUILD_MODE)/$(CONFIGURATION_LOWER)

# mbed origin paths
MBED_SRC:=$(MBED_DIR)/source
MBED_INC:=$(MBED_DIR)/uvisor-lib
MBED_BIN:=$(MBED_SRC)/$(CONFIGURATION_LOWER).box
MBED_CONFIG:=$(PLATFORM_DIR)/$(PLATFORM)/mbed/get_configuration.cmake

# Release binaries' definitions
BINARY_ASM_HEADER:=$(MBED_SRC)/uvisor-header.S
BINARY_ASM_INPUT:=$(MBED_SRC)/uvisor-input.S
BINARY_ASM_OUTPUT:=$(CONFIGURATION_PREFIX).s
BINARY_RELEASE:=$(MBED_SRC)/$(PLATFORM)/$(BUILD_MODE)/$(CONFIGURATION_LOWER).o

# Released header files
RELEASE_SRC:=$(RELEASE_DIR)/source
RELEASE_INC:=$(RELEASE_DIR)/uvisor-lib
RELEASE_BIN:=$(RELEASE_SRC)/$(PLATFORM)/$(BUILD_MODE)/$(CONFIGURATION_LOWER).o

# make ARMv7-M MPU driver the default
ifeq ("$(ARCH_MPU)","")
ARCH_MPU:=ARMv7M
endif

# ARMv7-M MPU driver
ifeq ("$(ARCH_MPU)","ARMv7M")
MPU_SRC:=\
         $(SYSTEM_DIR)/src/mpu/vmpu_armv7m.c \
         $(SYSTEM_DIR)/src/mpu/vmpu_armv7m_debug.c
endif

# Freescale K64 MPU driver
ifeq ("$(ARCH_MPU)","KINETIS")
MPU_SRC:=\
         $(SYSTEM_DIR)/src/mpu/vmpu_freescale_k64.c \
         $(SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_debug.c \
         $(SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_aips.c \
         $(SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_mem.c
endif

SOURCES:=\
         $(SYSTEM_DIR)/src/benchmark.c \
         $(SYSTEM_DIR)/src/halt.c \
         $(SYSTEM_DIR)/src/main.c \
         $(SYSTEM_DIR)/src/stdlib.c \
         $(SYSTEM_DIR)/src/svc.c \
         $(SYSTEM_DIR)/src/svc_cx.c \
         $(SYSTEM_DIR)/src/unvic.c \
         $(SYSTEM_DIR)/src/system.c \
         $(SYSTEM_DIR)/src/mpu/vmpu.c \
         $(DEBUG_DIR)/src/debug.c \
         $(DEBUG_DIR)/src/memory_map.c \
         $(LIB_DIR)/printf/tfp_printf.c \
         $(MPU_SRC) \
         $(PLATFORM_SRC)

SYSLIBS:=-lgcc -lc -lnosys
DEBUG:=-g3
WARNING:=-Wall -Werror

# Select optimizations depending on the build mode.
ifeq ("$(BUILD_MODE)","debug")
OPT:=
else
OPT:=-Os -DNDEBUG
endif

# determine repository version
PROGRAM_VERSION:=$(shell git describe --tags --abbrev=4 --dirty 2>/dev/null | sed s/^v//)
ifeq ("$(PROGRAM_VERSION)","")
PROGRAM_VERSION:='unknown'
endif

# Read UVISOR_MAGIC and UVISOR_{FLASH, SRAM}_LENGTH from uvisor-config.h.
ifeq ("$(wildcard  $(CORE_DIR)/uvisor-config.h)","")
	UVISOR_MAGIC:=0
	UVISOR_FLASH_LENGTH:=0
	UVISOR_SRAM_LENGTH:=0
else
	UVISOR_MAGIC:=$(shell grep UVISOR_MAGIC $(CORE_DIR)/uvisor-config.h | sed -E 's/^.* (0x[0-9A-Fa-f]+).*$\/\1/')
	UVISOR_FLASH_LENGTH:=$(shell grep UVISOR_FLASH_LENGTH $(CORE_DIR)/uvisor-config.h | sed -E 's/^.* (0x[0-9A-Fa-f]+).*$\/\1/')
	UVISOR_SRAM_LENGTH:=$(shell grep UVISOR_SRAM_LENGTH $(CORE_DIR)/uvisor-config.h | sed -E 's/^.* (0x[0-9A-Fa-f]+).*$\/\1/')
endif

FLAGS_CM4:=-mcpu=cortex-m4 -march=armv7e-m -mthumb

LDFLAGS:=\
        $(FLAGS_CM4) \
        -T$(CONFIGURATION_PREFIX).linker \
        -nostartfiles \
        -nostdlib \
        -Xlinker --gc-sections \
        -Xlinker -M \
        -Xlinker -Map=$(CONFIGURATION_PREFIX).map

CFLAGS_PRE:=\
        $(OPT) \
        $(DEBUG) \
        $(WARNING) \
        -DARCH_MPU_$(ARCH_MPU) \
        -D$(CONFIGURATION) \
        -DPROGRAM_VERSION=\"$(PROGRAM_VERSION)\" \
        $(APP_CFLAGS) \
        -I$(PLATFORM_INC) \
        -I$(CORE_DIR) \
        -I$(CMSIS_DIR)/inc \
        -I$(SYSTEM_DIR)/inc \
        -I$(SYSTEM_DIR)/inc/mpu \
        -I$(DEBUG_DIR)/inc \
        -I$(LIB_DIR)/printf \
        -ffunction-sections \
        -fdata-sections

CFLAGS:=$(FLAGS_CM4) $(CFLAGS_PRE)
CPPFLAGS:=-fno-exceptions

OBJS:=$(foreach SOURCE, $(SOURCES), $(SOURCE).$(CONFIGURATION_LOWER).$(BUILD_MODE).o)

LINKER_CONFIG:=\
    -D$(CONFIGURATION) \
    -DUVISOR_FLASH_LENGTH=$(UVISOR_FLASH_LENGTH) \
    -DUVISOR_SRAM_LENGTH=$(UVISOR_SRAM_LENGTH) \
    -include $(PLATFORM_DIR)/$(PLATFORM)/inc/config.h

BINARY_CONFIG:=\
    -DUVISOR_FLASH_LENGTH=$(UVISOR_FLASH_LENGTH) \
    -DUVISOR_SRAM_LENGTH=$(UVISOR_SRAM_LENGTH) \
    -DUVISOR_MAGIC=$(UVISOR_MAGIC) \
    -DUVISOR_BIN=\"$(CONFIGURATION_PREFIX).bin\"

.PHONY: all fresh __platform/% __configurations __binaries CONFIGURATION_% clean source.c.tags

# Build both the release and debug versions for all platforms for all
# configurations.
all: $(foreach PLATFORM, $(PLATFORMS), __platform/$(PLATFORM))

# Same as all, but clean first.
fresh: clean all

# This target is used to iterate over all platforms. It builds both the release
# and debug versions of all the platform-specific configurations.
# Note: Do not use platform/% as target name to avoid collision wiht the folder
# name platform/*, which otherwise would be seen as an already met make
# prerequisite.
__platform/%:
	@echo
	@echo
	make BUILD_MODE=debug PLATFORM=$(@F) __configurations
	@echo
	make BUILD_MODE=release PLATFORM=$(@F) __configurations

# This middleware target is needed because the parent make does not know the
# configurations yet (they are platform-specific).
__configurations: $(CONFIGURATIONS)

# This target is used to iterate over all configurations for the current
# platform.
# Note: We need to remove the object files from a previous build or otherwise
# the old configurations will be used.
CONFIGURATION_%:
ifndef PLATFORM
	$(error "Missing platform. Use PLATFORM=<your_platform> make CONFIGURATION_<your_configuration>")
else
	make CONFIGURATION=$@ __binaries yotta
endif

# This middleware target is needed because the parent make does not know the
# name to give to the binary release yet (it is configuration-specific).
__binaries: $(BINARY_RELEASE)

# Generate the pre-linked uVisor binary for the given platform and configuration.
$(BINARY_RELEASE): $(CONFIGURATION_PREFIX).bin
	mkdir -p $(dir $(BINARY_RELEASE))
	cp -f $(BINARY_ASM_HEADER) $(BINARY_ASM_OUTPUT)
	$(CPP) -w -P $(BINARY_CONFIG) $(BINARY_ASM_INPUT) > $(BINARY_ASM_OUTPUT)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $(BINARY_RELEASE) $(BINARY_ASM_OUTPUT)
	rm -f $(BINARY_ASM_OUTPUT)

# Copy the release material to the uvisor-lib module.
yotta: $(BINARY_RELEASE)
	rm -f $(RELEASE_INC)/*.h
	rm -f $(RELEASE_SRC)/*.cpp
	rm -f $(RELEASE_OBJ) $(RELEASE_VER)
	mkdir -p $(RELEASE_INC)
	mkdir -p $(RELEASE_SRC)/$(PLATFORM)/$(BUILD_MODE)
	echo "$(PROGRAM_VERSION)" > $(RELEASE_SRC)/$(PLATFORM)/version.txt
	cp $(MBED_INC)/*.h   $(RELEASE_INC)/
	cp $(MBED_SRC)/*.cpp $(RELEASE_SRC)/
	find $(ROOT_DIR) -name "*_exports.h" -not -path "$(RELEASE_DIR)/*" -exec cp {} $(RELEASE_INC)/ \;
	cp $(MBED_CONFIG) $(RELEASE_SRC)/$(PLATFORM)/
	cp $(BINARY_RELEASE) $(RELEASE_BIN$)

$(CONFIGURATION_PREFIX).bin: $(CONFIGURATION_PREFIX).elf
	$(OBJCOPY) $< -O binary $@

$(CONFIGURATION_PREFIX).elf: $(OBJS) $(CONFIGURATION_PREFIX).linker
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(SYSLIBS)
	$(OBJDUMP) -d $@ > $(CONFIGURATION_PREFIX).asm

$(CONFIGURATION_PREFIX).linker: $(CORE_DIR)/linker/default.h
	mkdir -p $(dir $(CONFIGURATION_PREFIX))
	$(CPP) -w -P $(LINKER_CONFIG) $< -o $@

%.c.$(CONFIGURATION_LOWER).$(BUILD_MODE).o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

ctags: source.c.tags

source.c.tags: $(SOURCES)
	CFLAGS="$(CFLAGS_PRE)" geany -g $@ $^

clean:
	rm -f source.c.tags
	find $(ROOT_DIR) -iname '*.o' -delete
	find $(ROOT_DIR) -iname '*.asm' -delete
	find $(ROOT_DIR) -iname '*.bin' -delete
	find $(ROOT_DIR) -iname '*.elf' -delete
	find $(ROOT_DIR) -iname '*.map' -delete
	find $(ROOT_DIR) -iname '*.linker' -delete
	rmdir $(foreach MODE, debug release, $(wildcard $(PLATFORM_DIR)/*/$(MODE))) >/dev/null 2>&1 || true
	$(APP_CLEAN)
