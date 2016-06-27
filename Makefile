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
AR:=$(PREFIX)ar

# Root folder
ROOT_DIR:=.

# Top-level folder paths
API_DIR:=$(ROOT_DIR)/api
CORE_DIR:=$(ROOT_DIR)/core
DOCS_DIR:=$(ROOT_DIR)/docs
PLATFORM_DIR:=$(ROOT_DIR)/platform
TOOLS_DIR:=$(ROOT_DIR)/tools

# Core paths
CORE_CMSIS_DIR:=$(CORE_DIR)/cmsis
CORE_DEBUG_DIR:=$(CORE_DIR)/debug
CORE_LIB_DIR:=$(CORE_DIR)/lib
CORE_LINKER_DIR:=$(CORE_DIR)/linker
CORE_SYSTEM_DIR:=$(CORE_DIR)/system

# List of supported platforms
# Note: One could do it in a simpler way but this prevents spurious files in
# $(PLATFORM_DIR) from getting picked.
PLATFORMS = $(notdir $(realpath $(dir $(wildcard $(PLATFORM_DIR)/*/))))

# List of uVisor configurations for the given platform
ifdef PLATFORM
include $(PLATFORM_DIR)/$(PLATFORM)/Makefile.configurations
endif

# Platform-specific source files
PLATFORM_SRC:=$(wildcard $(PLATFORM_DIR)/$(PLATFORM)/src/*.c)

# Configuration-specific paths
CONFIGURATION_LOWER:=$(shell echo $(CONFIGURATION) | tr '[:upper:]' '[:lower:]')
CONFIGURATION_PREFIX:=$(PLATFORM_DIR)/$(PLATFORM)/$(BUILD_MODE)/$(CONFIGURATION_LOWER)

# Release library definitions
API_ASM_HEADER:=$(API_DIR)/src/uvisor-header.S
API_ASM_INPUT:=$(API_DIR)/src/uvisor-input.S
API_ASM_OUTPUT:=$(API_ASM_INPUT).$(CONFIGURATION_LOWER).$(BUILD_MODE).s
API_RELEASE:=$(API_DIR)/lib/$(PLATFORM)/$(BUILD_MODE)/$(CONFIGURATION_LOWER).a
API_VERSION:=$(API_DIR)/lib/$(PLATFORM)/$(BUILD_MODE)/$(CONFIGURATION_LOWER).txt

# Release library source files
# Note: We explicitly remove unsupported.c from the list of source files. It
#       will be compiled by the host OS in case uVisor is not supported.
API_SOURCES:=$(wildcard $(API_DIR)/src/*.c)
API_SOURCES:=$(filter-out $(API_DIR)/src/unsupported.c, $(API_SOURCES))
API_OBJS:=$(foreach API_SOURCE, $(API_SOURCES), $(API_SOURCE).$(CONFIGURATION_LOWER).$(BUILD_MODE).o) $(API_ASM_OUTPUT:.s=.o)

# make ARMv7-M MPU driver the default
ifeq ("$(ARCH_MPU)","")
ARCH_MPU:=ARMv7M
endif

# ARMv7-M MPU driver
ifeq ("$(ARCH_MPU)","ARMv7M")
MPU_SRC:=\
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_armv7m.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_armv7m_debug.c
endif

# Freescale K64 MPU driver
ifeq ("$(ARCH_MPU)","KINETIS")
MPU_SRC:=\
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_freescale_k64.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_debug.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_aips.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_mem.c
endif

SOURCES:=\
         $(CORE_SYSTEM_DIR)/src/benchmark.c \
         $(CORE_SYSTEM_DIR)/src/box_init.c \
         $(CORE_SYSTEM_DIR)/src/context.c \
         $(CORE_SYSTEM_DIR)/src/halt.c \
         $(CORE_SYSTEM_DIR)/src/main.c \
         $(CORE_SYSTEM_DIR)/src/page_allocator.c \
         $(CORE_SYSTEM_DIR)/src/page_allocator_faults.c \
         $(CORE_SYSTEM_DIR)/src/register_gateway.c \
         $(CORE_SYSTEM_DIR)/src/stdlib.c \
         $(CORE_SYSTEM_DIR)/src/svc.c \
         $(CORE_SYSTEM_DIR)/src/unvic.c \
         $(CORE_SYSTEM_DIR)/src/export_table.c \
         $(CORE_SYSTEM_DIR)/src/system.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu.c \
         $(CORE_DEBUG_DIR)/src/debug.c \
         $(CORE_DEBUG_DIR)/src/memory_map.c \
         $(CORE_LIB_DIR)/printf/tfp_printf.c \
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
        -DUVISOR_PRESENT=1 \
        -DARCH_MPU_$(ARCH_MPU) \
        -D$(CONFIGURATION) \
        -DPROGRAM_VERSION=\"$(PROGRAM_VERSION)\" \
        $(APP_CFLAGS) \
        -I$(ROOT_DIR) \
        -I$(CORE_DIR) \
        -I$(CORE_CMSIS_DIR)/inc \
        -I$(CORE_DEBUG_DIR)/inc \
        -I$(CORE_LIB_DIR)/printf \
        -I$(PLATFORM_DIR)/$(PLATFORM)/inc \
        -I$(CORE_SYSTEM_DIR)/inc \
        -I$(CORE_SYSTEM_DIR)/inc/mpu \
        -ffunction-sections \
        -fdata-sections

CFLAGS:=$(FLAGS_CM4) $(CFLAGS_PRE)
CPPFLAGS:=-fno-exceptions

OBJS:=$(foreach SOURCE, $(SOURCES), $(SOURCE).$(CONFIGURATION_LOWER).$(BUILD_MODE).o)

LINKER_CONFIG:=\
    -D$(CONFIGURATION) \
    -I$(PLATFORM_DIR)/$(PLATFORM)/inc \
    -include $(CORE_DIR)/uvisor-config.h

API_CONFIG:=\
    -DUVISOR_BIN=\"$(CONFIGURATION_PREFIX).bin\" \
    -D$(CONFIGURATION) \
    -I$(PLATFORM_DIR)/$(PLATFORM)/inc \
    -include $(CORE_DIR)/uvisor-config.h

.PHONY: all fresh configurations release clean ctags

# Build both the release and debug versions for all platforms for all
# configurations.
all: $(foreach PLATFORM, $(PLATFORMS), platform-$(PLATFORM))

# Same as all, but clean first.
fresh: clean all

# This target is used to iterate over all platforms. It builds both the release
# and debug versions of all the platform-specific configurations.
platform-%:
	@echo
	make BUILD_MODE=debug PLATFORM=$* configurations
	@echo
	make BUILD_MODE=release PLATFORM=$* configurations

# This middleware target is needed because the parent make does not know the
# configurations yet (they are platform-specific).
configurations: $(CONFIGURATIONS)

# This target is used to iterate over all configurations for the current
# platform.
CONFIGURATION_%:
ifndef PLATFORM
	$(error "Missing platform. Use PLATFORM=<platform> BUILD_MODE=<build_mode> make CONFIGURATION_<configuration>")
endif
ifndef BUILD_MODE
	$(error "Missing build mode. Use PLATFORM=<platform> BUILD_MODE=<build_mode> make CONFIGURATION_<configuration>")
endif
	make BUILD_MODE=$(BUILD_MODE) PLATFORM=$(PLATFORM) CONFIGURATION=$@ release

# This middleware target is needed because the parent make does not know the
# name to give to the binary release yet (it is configuration-specific).
release: $(API_RELEASE)

# Generate the pre-linked uVisor binary for the given platform and configuration.
# The binary is then packaged with the uVisor library implementations into a
# unique .a file.
$(API_RELEASE): $(API_ASM_OUTPUT) $(API_DIR)/lib/$(PLATFORM)/$(BUILD_MODE) $(API_OBJS)
	$(AR) Dcr $(API_RELEASE) $(API_OBJS)
	echo "$(PROGRAM_VERSION)" > $(API_VERSION)

# Generate the output asm file from the asm input file and the INCBIN'ed binary.
$(API_ASM_OUTPUT): $(CONFIGURATION_PREFIX).bin
	cp -f $(API_ASM_HEADER) $(API_ASM_OUTPUT)
	$(CPP) -w -P $(API_CONFIG) $(API_ASM_INPUT) > $(API_ASM_OUTPUT)

# Generate the library folders if they do not exist.
$(API_DIR)/lib/$(PLATFORM)/$(BUILD_MODE):
	mkdir -p $(API_DIR)/lib/$(PLATFORM)/$(BUILD_MODE)

$(CONFIGURATION_PREFIX).bin: $(CONFIGURATION_PREFIX).elf
	$(OBJCOPY) $< -O binary $@

$(CONFIGURATION_PREFIX).elf: $(OBJS) $(CONFIGURATION_PREFIX).linker
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(SYSLIBS)
	$(OBJDUMP) -d $@ > $(CONFIGURATION_PREFIX).asm

$(CONFIGURATION_PREFIX).linker: $(CORE_LINKER_DIR)/default.h
	mkdir -p $(dir $(CONFIGURATION_PREFIX))
	$(CPP) -w -P $(LINKER_CONFIG) $< -o $@

%.c.$(CONFIGURATION_LOWER).$(BUILD_MODE).o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.S.$(CONFIGURATION_LOWER).$(BUILD_MODE).o: %.S.$(CONFIGURATION_LOWER).$(BUILD_MODE).s
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
	find $(ROOT_DIR) -iname '*.*.s' -delete
	rm -rf $(API_DIR)/lib
	rm -rf $(foreach MODE, debug release, $(wildcard $(PLATFORM_DIR)/*/$(MODE)))
	$(APP_CLEAN)
