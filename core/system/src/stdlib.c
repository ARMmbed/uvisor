/*
 * Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <uvisor.h>

#ifdef  CHANNEL_DEBUG
void swo_putc(uint8_t data)
{
    static uint8_t itm_init = 0;

    if(!itm_init)
    {
        itm_init = TRUE;

        /* reset previous channel settings */
        ITM->LAR  = 0xC5ACCE55;

        /* wait for debugger to connect */
        while(!((ITM->TCR & ITM_TCR_ITMENA_Msk) &&
                (ITM->TER & (1<<CHANNEL_DEBUG))));
    }

    if(    (ITM->TCR & ITM_TCR_ITMENA_Msk) &&
        (ITM->TER & (1<<CHANNEL_DEBUG))
        )
    {
        /* FIXME power */
        /* wait for TX */
        while (ITM->PORT[CHANNEL_DEBUG].u32 == 0);
        /* TX debug character */
        ITM->PORT[CHANNEL_DEBUG].u8 = data;
    }
}

void UVISOR_WEAK default_putc(uint8_t data)
{
    swo_putc(data);
}
#endif/*CHANNEL_DEBUG*/
