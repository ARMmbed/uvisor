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
void WEAK default_putc (uint8_t data)
{
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
