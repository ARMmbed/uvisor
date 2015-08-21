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
PROJECT:=stm32f4_uvisor
ARCH:=STM32F4
CPU:=$(ARCH)29xx
CONFIG:=
CPU_JLINK:=$(ARCH)29ZI

APP_CLEAN:=
APP_CFLAGS:=-DUVISOR $(CONFIG) -Iconfig -Isrc -Ilib -Idebug

APP_SRC:=\
	src/stm32f4_halt.c \
	src/stm32f4_hw.c \
	debug/stm32f4_memory_map.c

include ../../core/Makefile.rules
