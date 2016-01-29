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
# Path of the current Makefile
CURRENT_PATH:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# The project name is given by the top-level platform-specific folder
PROJECT:=$(shell basename $(CURRENT_PATH))
PROJECT_UPPER:=$(shell echo $(PROJECT) | tr '[:lower:]' '[:upper:]')

ARCH_MPU:=MK64F

# Default include paths for the project.
# Note: Out of all the chip-specific headers we only need those for the K64F, as
#       they are only used for MPU symbols
INCLUDE_PATHS:=-I. -Iinc -Iarch -Iarch/MK64F12 -Iarch/MK64F12/include -Iarch/MK64F12/startup

APP_CFLAGS:=-DUVISOR $(CONFIG) $(INCLUDE_PATHS)
APP_SRC:=\
	src/k64f_halt.c \
	src/k64f_memory_map.c \
	src/system.c

# We hardcode the CPU value even if we build for different CPUs. This is
# possible as we only rely on the NXP MPU symbols, which are the same across the
# whole Kinetis family.
CPU:=CPU_MK64FN1M0VLL12

# Export all variables to the sub-make.
export

# Currently we only support one configuration (for the FRDM-K64F board)
kinetis: CONFIGURATIONS:= \
	K64F
kinetis:
	$(foreach CONFIGURATION, $(CONFIGURATIONS), \
		CONFIGURATION=$(CONFIGURATION) \
		APP_CFLAGS='$(APP_CFLAGS) $(foreach PATH, $(shell find arch/* -type d), -I$(PATH))' \
		make -f ../../core/Makefile.rules clean release)

include ../../core/Makefile.rules
