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

# The project name is given by the top-level platform-specific folder.
PROJECT:=$(shell basename $(CURRENT_PATH))

# Project-specific flags
APP_CFLAGS:=-I. -Iinc

# Project-specific source files
APP_SRC:=\
	src/vmpu_hw.c

# Export all variables to the sub-make.
export

CONFIGURATIONS:= \
	STM32_M4_0x10000000_0x0

stm32_r:
	$(foreach CONFIGURATION, \
	          $(CONFIGURATIONS), \
	          CONFIGURATION=$(CONFIGURATION) make -f ../../core/Makefile.rules clean release ; )

stm32_d:
	$(foreach CONFIGURATION, \
	          $(CONFIGURATIONS), \
	          CONFIGURATION=$(CONFIGURATION) make OPT= -f ../../core/Makefile.rules clean release ; )

# We include the main Makefile so that global rules are still available.
include ../../core/Makefile.rules
