/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
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
