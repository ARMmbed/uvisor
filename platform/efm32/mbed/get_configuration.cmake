###########################################################################
#
#  Copyright 2016 Silicon Laboratories, Inc.
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

if(TARGET_LIKE_EFM32 AND YOTTA_CFG_UVISOR_PRESENT)
	if(TARGET_LIKE_CORTEX_M3)
		set(UVISOR_CONFIGURATION "configuration_efm32_m3_p1")
	elseif(TARGET_LIKE_CORTEX_M4)
		set(UVISOR_CONFIGURATION "configuration_efm32_m4_p1")
	endif()
endif()
