#include <iot-os.h>
#include "crc16.h"

uint16_t
crc_ccitt (uint8_t * data, uint32_t length)
{
    uint16_t crc = 0xFFFF;

    while(length--)
    {
        crc = (crc >> 8) | (crc << 8);
        crc ^= *data++;
        crc ^= (crc & 0xFF) >> 4;
        crc ^= crc << 12;
        crc ^= (crc & 0xFF) << 5;
    }

    return crc ^ 0xFFFF;
}
