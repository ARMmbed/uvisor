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
#       $(PLATFORM_DIR) from getting picked.
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
API_ASM_OUTPUT:=$(CONFIGURATION_PREFIX)/$(API_DIR)/uvisor-output.s
API_RELEASE:=$(API_DIR)/lib/$(PLATFORM)/$(BUILD_MODE)/$(CONFIGURATION_LOWER).a
API_VERSION:=$(API_DIR)/lib/$(PLATFORM)/$(BUILD_MODE)/$(CONFIGURATION_LOWER).txt

# Release library source files
# Note: We explicitly remove unsupported.c from the list of source files. It
#       will be compiled by the host OS in case uVisor is not supported.
API_SOURCES:=$(wildcard $(API_DIR)/src/*.c)
API_SOURCES:=$(filter-out $(API_DIR)/src/unsupported.c, $(API_SOURCES))

# Release library object files
API_OBJS:=$(foreach API_SOURCE, $(API_SOURCES), $(CONFIGURATION_PREFIX)/$(API_DIR)/$(notdir $(API_SOURCE:.c=.o))) \
          $(API_ASM_OUTPUT:.s=.o)

# Make the ARMv7-M MPU driver the default.
ifeq ("$(ARCH_MPU)","")
ARCH_MPU:=ARMv7M
endif

# ARMv7-M MPU driver
ifeq ("$(ARCH_MPU)","ARMv7M")
MPU_SRC:=\
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_armv7m.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_armv7m_debug.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_armv7m_mpu.c
endif

# Freescale K64 MPU driver
ifeq ("$(ARCH_MPU)","KINETIS")
MPU_SRC:=\
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_freescale_k64.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_debug.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_aips.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_mem.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu_freescale_k64_mpu.c
endif

# Core source files
SOURCES:=\
         $(CORE_SYSTEM_DIR)/src/benchmark.c \
         $(CORE_SYSTEM_DIR)/src/box_init.c \
         $(CORE_SYSTEM_DIR)/src/context.c \
         $(CORE_SYSTEM_DIR)/src/export_table.c \
         $(CORE_SYSTEM_DIR)/src/halt.c \
         $(CORE_SYSTEM_DIR)/src/main.c \
         $(CORE_SYSTEM_DIR)/src/page_allocator.c \
         $(CORE_SYSTEM_DIR)/src/page_allocator_faults.c \
         $(CORE_SYSTEM_DIR)/src/register_gateway.c \
         $(CORE_SYSTEM_DIR)/src/semaphore.c \
         $(CORE_SYSTEM_DIR)/src/stdlib.c \
         $(CORE_SYSTEM_DIR)/src/svc.c \
         $(CORE_SYSTEM_DIR)/src/system.c \
         $(CORE_SYSTEM_DIR)/src/unvic.c \
         $(CORE_SYSTEM_DIR)/src/mpu/vmpu.c \
         $(CORE_DEBUG_DIR)/src/debug.c \
         $(CORE_DEBUG_DIR)/src/memory_map.c \
         $(CORE_LIB_DIR)/printf/tfp_printf.c \
         $(MPU_SRC) \
         $(PLATFORM_SRC)

# Core object files
OBJS:=$(foreach SOURCE, $(SOURCES), $(CONFIGURATION_PREFIX)/$(CORE_DIR)/$(notdir $(SOURCE:.c=.o)))

# Source files search path
# The vpath command allows us to look for source files by only using their
# basename. It is required because we want object files to be placed in a
# different path from their corresponding sources.
# The search path needs to be set depending on whether the current target is to
# build the core or the release library (so the two can have the same names
# without collisions).
ifneq ($(MAKECMDGOALS),build_core)
vpath %.c $(API_DIR)/src
vpath %.s $(API_DIR)/src
else
vpath %.c $(CORE_SYSTEM_DIR)/src:\
          $(CORE_SYSTEM_DIR)/src/mpu:\
          $(CORE_DEBUG_DIR)/src:\
          $(CORE_LIB_DIR)/printf:\
          $(PLATFORM_DIR)/$(PLATFORM)/src
endif

SYSLIBS:=-lgcc -lc -lnosys
DEBUG:=-g3
WARNING:=-Wall -Werror

# Select optimizations depending on the build mode.
ifeq ("$(BUILD_MODE)","debug")
OPT:=
else
OPT:=-Os -DNDEBUG
endif

# Determine the repository version.
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
CPPFLAGS:=
CXXFLAGS:=-fno-exceptions

LINKER_CONFIG:=\
    -D$(CONFIGURATION) \
    -I$(PLATFORM_DIR)/$(PLATFORM)/inc \
    -include $(CORE_DIR)/uvisor-config.h

API_CONFIG:=\
    -DUVISOR_BIN=\"$(CONFIGURATION_PREFIX).bin\" \
    -D$(CONFIGURATION) \
    -I$(PLATFORM_DIR)/$(PLATFORM)/inc \
    -include $(CORE_DIR)/uvisor-config.h

.PHONY: all fresh configurations build_core build_api clean ctags

# Build both the release and debug versions for all platforms for all
# configurations.
all: $(foreach PLATFORM, $(PLATFORMS), platform-$(PLATFORM))

# Same as "all", but clean first.
fresh: clean all

# This target builds the release and debug version of a platform.
# The "all" target uses this target to iterate over all platforms.
platform-%:
	@echo
	@# 2nd-level make
	make BUILD_MODE=debug PLATFORM=$* configurations
	@echo
	@# 2nd-level make
	make BUILD_MODE=release PLATFORM=$* configurations

# This middleware target is needed because the 1st-level make does not know the
# configurations yet.
# $(CONFIGURATIONS) is defined by the platform-specific Makefile extension, so
# this target can only be called by the 2nd-level make.
configurations: $(CONFIGURATIONS)

# This target builds a single configuration, provided that the platform and the
# build mode are already configured.
# The "configurations" target uses this target to iterate over all
# configurations for a platform, given a build mode.
CONFIGURATION_%:
ifndef PLATFORM
	$(error "Missing platform. Use PLATFORM=<platform> BUILD_MODE=<build_mode> make CONFIGURATION_<configuration>")
endif
ifndef BUILD_MODE
	$(error "Missing build mode. Use PLATFORM=<platform> BUILD_MODE=<build_mode> make CONFIGURATION_<configuration>")
endif
	@# 3rd-level make
	make BUILD_MODE=$(BUILD_MODE) PLATFORM=$(PLATFORM) CONFIGURATION=$@ build_core
	@# 3rd-level make
	make BUILD_MODE=$(BUILD_MODE) PLATFORM=$(PLATFORM) CONFIGURATION=$@ build_api

# This middleware target is needed because the parent make does not know the
# name to give to the core binary yet.
# $(CONFIGURATION_PREFIX).bin is defined using platform-, configuration-, and
# build-mode-specific definitions, so this target can only be called by the
# 3rd-level make.
build_core: $(CONFIGURATION_PREFIX).bin

# This middleware target is needed because the parent make does not know the
# name to give to the release library yet.
# $(API_RELEASE) is defined using platform-, configuration-, and
# build-mode-specific definitions, so this target can only be called by the
# 3rd-level make.
build_api: $(API_RELEASE)

# Generate the needed folders if they do not exist.
$(CONFIGURATION_PREFIX)/$(CORE_DIR):
	mkdir -p $(CONFIGURATION_PREFIX)/$(CORE_DIR)
$(CONFIGURATION_PREFIX)/$(API_DIR):
	mkdir -p $(CONFIGURATION_PREFIX)/$(API_DIR)
$(dir $(API_RELEASE)):
	mkdir -p $(dir $(API_RELEASE))

# Generate the pre-linked uVisor binary for the given platform, configuration
# and build mode.
# The binary is then packaged with the uVisor library implementations into a
# unique .a file.
$(API_RELEASE): $(dir $(API_RELEASE)) $(CONFIGURATION_PREFIX)/$(API_DIR) $(API_OBJS)
	$(AR) Dcr $(API_RELEASE) $(API_OBJS)
	echo "$(PROGRAM_VERSION)" > $(API_VERSION)

# Copy the core ELF file into a binary.
$(CONFIGURATION_PREFIX).bin: $(CONFIGURATION_PREFIX).elf
	$(OBJCOPY) $< -O binary $@

# Link all the core object files into the core ELF.
$(CONFIGURATION_PREFIX).elf: $(CONFIGURATION_PREFIX)/$(CORE_DIR) $(OBJS) $(CONFIGURATION_PREFIX).linker
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(SYSLIBS)
	$(OBJDUMP) -d $@ > $(CONFIGURATION_PREFIX).asm

# Pre-process the core linker script.
$(CONFIGURATION_PREFIX).linker: $(CORE_LINKER_DIR)/default.h
	$(CPP) -w -P $(LINKER_CONFIG) $< -o $@

# Pre-process and compile a core C file into an object file.
$(CONFIGURATION_PREFIX)/$(CORE_DIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Pre-process and compile a release library C file into an object file.
$(CONFIGURATION_PREFIX)/$(API_DIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Generate the output asm file from the asm input file and the INCBIN'ed binary.
$(API_ASM_OUTPUT:.s=.o): $(CONFIGURATION_PREFIX).bin $(API_ASM_INPUT)
	cp $(API_ASM_HEADER) $(API_ASM_OUTPUT)
	$(CPP) -w -P $(API_CONFIG) $(API_ASM_INPUT) >> $(API_ASM_OUTPUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(API_ASM_OUTPUT) -o $@

ctags: source.c.tags

source.c.tags: $(SOURCES)
	CFLAGS="$(CFLAGS_PRE)" geany -g $@ $^

# Clean everything.
# Note: You will lose any previously compiled library.
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
