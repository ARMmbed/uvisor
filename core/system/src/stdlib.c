#include <string.h>
#include <stdint.h>

void* memset(void *s, int c, size_t n)
{
    uint8_t *p = (uint8_t*) s;

    if(n>0)
        while(n--)
            *p++ = (uint8_t)c;

    return s;
}