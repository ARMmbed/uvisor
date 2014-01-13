#include <iot-os.h>
#include "crc16.h"

uint16_t
crc_ccitt (uint8_t * data, unsigned int length)
{
    uint16_t crc = 0xFFFF;

    for (int j = 0; j < length; j++)
        {
            crc = (crc >> 8) | (crc << 8);
            crc ^= data[j];
            crc ^= (crc & 0xFF) >> 4;
            crc ^= crc << 12;
            crc ^= (crc & 0xFF) << 5;
        }
    return crc;
}
