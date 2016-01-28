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

ARCH:=MK64F
ARCH_MPU:=$(ARCH)
CPU:=$(ARCH)N1M0XXX12
CONFIG:=

INCLUDE_PATHS:=-I. -Iinc -Iarch/MK64F -Iarch/MK64F/inc

APP_CLEAN:=
APP_CFLAGS:=-DUVISOR $(CONFIG) $(INCLUDE_PATHS)
APP_SRC:=\
	src/k64f_halt.c \
	src/k64f_memory_map.c \
	arch/MK64F/src/system.c

include ../../core/Makefile.rules
