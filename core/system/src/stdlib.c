/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2014 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#include <uvisor.h>

void* memset(void *s, int c, size_t n)
{
    uint8_t *p = (uint8_t*) s;

    if(n>0)
        while(n--)
            *p++ = (uint8_t)c;

    return s;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    const uint8_t *s = (const uint8_t*) src;
    uint8_t *d = (uint8_t*) dst;

    if(n>0)
        while(n--)
            *d++ = *s++;

    return dst;
}

#ifdef  CHANNEL_DEBUG
void UVISOR_WEAK default_putc (uint8_t data)
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
#endif/*CHANNEL_DEBUG*/
