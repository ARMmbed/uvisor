###########################################################################
#
#  Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
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

ARCH:=STM32F4
CPU:=$(ARCH)29xx
CONFIG:=
CPU_JLINK:=$(ARCH)29ZI

INCLUDE_PATHS:=-I. -Iinc -Iarch/STM32F4 -Iarch/STM32F4/inc

APP_CLEAN:=
APP_CFLAGS:=-DUVISOR $(CONFIG) $(INCLUDE_PATHS)

APP_SRC:=\
	src/stm32f4_halt.c \
	src/stm32f4_hw.c \
	src/stm32f4_memory_map.c \
	arch/STM32F4/src/system.c

include ../../core/Makefile.rules
