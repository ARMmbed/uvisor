#include <uvisor.h>

void main_entry(void)
{
    /* switch to unprivileged mode - this is possible as uvisor code
     * is readable by unprivileged code and only the key-value database
     * is protected from the unprivileged mode */
    __set_CONTROL(__get_CONTROL()|3);
    __ISB();
    __DSB();

    /* should never happen */
    while(1)
        __WFI();
}
