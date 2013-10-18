#include <iot-os.h>

void reset_handler(void)
{
    __builtin_memcpy(&__data_start__, &__data_start_src__, __data_size__);
}
