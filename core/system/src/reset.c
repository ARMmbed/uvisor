#include <iot-os.h>

void __attribute__ ((noreturn)) reset_handler(void)
{
    uint32_t t, *dst;
    const uint32_t* src;

    /* initialize data RAM from flash*/
    src = &__data_start_src__;
    dst = &__data_start__;
    while(dst<&__data_end__)
        *dst++ = *src++;

    /* set bss to zero */
    dst = &__bss_start__;
    while(dst<&__bss_end__)
        *dst++ = 0;

    main();
    while(1){};
}
