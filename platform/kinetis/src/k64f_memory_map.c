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
#include <memory_map.h>

static const MemMap g_mem_map[] = {
    {"Flash",      0x00000000, 0x000FFFFF}, /* available Flash               */
    {">FlexBus",   0x08000000, 0x0FFFFFFF}, /* (alias) FlexBus               */
    {"Acc. RAM",   0x14000000, 0x14000FFF}, /* Programming acceleration RAM  */
    {">FlexBus",   0x18000000, 0x1BFFFFFF}, /* (alias) FlexBus               */
    {"SRAM",       0x1FFF0000, 0x2002FFFF}, /* available SRAM                */
    {">TCMU",      0x22000000, 0x23FFFFFF}, /* (alias) TCMU SRAM             */
    {">Flash D.",  0x30000000, 0x3007FFFF}, /* (alias) Flash Data            */
    {">Flash F.",  0x34000000, 0x3FFFFFFF}, /* (alias) Flash Flex            */
    {"AIPS0",      0x40000000, 0x40000210}, /* start of AIPS0                */
    {"AXBS",       0x40004000, 0x40007410},
    {"DMA",        0x40008000, 0x4000C800},
    {"FB",         0x4000C000, 0x4000C190},
    {"MPU",        0x4000D000, 0x4000F0C0},
    {"FMC",        0x4001F000, 0x4001FA00},
    {"FTFE",       0x40020000, 0x40020060},
    {"DMAMUX",     0x40021000, 0x40021040},
    {"CAN0",       0x40024000, 0x40026300},
    {"RNG",        0x40029000, 0x40029040},
    {"SPI0",       0x4002C000, 0x4002C230},
    {"SPI1",       0x4002D000, 0x4002D230},
    {"I2S0",       0x4002F000, 0x4002F420},
    {"CRC",        0x40032000, 0x40032030},
    {"USBDCD",     0x40035000, 0x40035070},
    {"PDB0",       0x40036000, 0x40036680},
    {"PIT",        0x40037000, 0x40037500},
    {"FTM0",       0x40038000, 0x40038270},
    {"FTM1",       0x40039000, 0x40039270},
    {"FTM2",       0x4003A000, 0x4003A270},
    {"ADC0",       0x4003B000, 0x4003B1C0},
    {"RTC",        0x4003D000, 0x4003F020},
    {"RFVBAT",     0x4003E000, 0x4003E080},
    {"LPTMR0",     0x40040000, 0x40040040},
    {"RFSYS",      0x40041000, 0x40041080},
    {"SIM",        0x40047000, 0x40048068},
    {"PORTA",      0x40049000, 0x40049330},
    {"PORTB",      0x4004A000, 0x4004A330},
    {"PORTC",      0x4004B000, 0x4004B330},
    {"PORTD",      0x4004C000, 0x4004C330},
    {"PORTE",      0x4004D000, 0x4004D330},
    {"WDOG",       0x40052000, 0x40052060},
    {"EWM",        0x40061000, 0x40061010},
    {"CMT",        0x40062000, 0x40062030},
    {"MCG",        0x40064000, 0x40064038},
    {"OSC",        0x40065000, 0x40065004},
    {"I2C0",       0x40066000, 0x40066030},
    {"I2C1",       0x40067000, 0x40067030},
    {"UART0",      0x4006A000, 0x4006A080},
    {"UART1",      0x4006B000, 0x4006B080},
    {"UART2",      0x4006C000, 0x4006C080},
    {"UART3",      0x4006D000, 0x4006D080},
    {"USB0",       0x40072000, 0x40072574},
    {"CMP0",       0x40073000, 0x40073018},
    {"CMP1",       0x40073008, 0x40073020},
    {"CMP2",       0x40073010, 0x40073028},
    {"VREF",       0x40074000, 0x40074008},
    {"LLWU",       0x4007C000, 0x4007C02C},
    {"PMC",        0x4007D000, 0x4007D00C},
    {"SMC",        0x4007E000, 0x4007E010},
    {"RCM",        0x4007F000, 0x4007F020}, /* end of AIPS0                   */
    {"AIPS1",      0x40080000, 0x40080210}, /* start of AIPS1                 */
    {"SPI2",       0x400AC000, 0x400AC230},
    {"SDHC",       0x400B1000, 0x400B1400},
    {"FTM3",       0x400B9000, 0x400B9270},
    {"ADC1",       0x400BB000, 0x400BB1C0},
    {"ENET",       0x400C0000, 0x400C18A0},
    {"DAC0",       0x400CC000, 0x400CC090},
    {"DAC1",       0x400CD000, 0x400CD090},
    {"I2C2",       0x400E6000, 0x400E6030},
    {"UART4",      0x400EA000, 0x400EA080},
    {"UART5",      0x400EB000, 0x400EB080}, /* end of AIPS1                  */
    {"PTA",        0x400FF000, 0x400FF060}, /* start of GPIO                 */
    {"PTB",        0x400FF040, 0x400FF0A0},
    {"PTC",        0x400FF080, 0x400FF0E0},
    {"PTD",        0x400FF0C0, 0x400FF120},
    {"PTE",        0x400FF100, 0x400FF160}, /* end of GPIO                   */
    {">AIPS-GPIO", 0x42000000, 0x43FFFFFF}, /* (alias) AIPS0, AIPS1, GPIO    */
    {"FlexBusWB",  0x60000000, 0x7FFFFFFF}, /* FlexBus (Write-Back)          */
    {"FlexBusWT",  0x80000000, 0x9FFFFFFF}, /* FlexBus (Write-Through)       */
    {"FlexBusXN",  0xA0000000, 0xDFFFFFFF}, /* FlexBus (peripherals, XN)     */
    {"ITM",        0xE0000000, 0xE0000FFF}, /* start of PPB                  */
    {"DWT",        0xE0001000, 0xE0001FFF},
    {"FPB",        0xE0002000, 0xE0002FFF},
    {"SCS",        0xE000E000, 0xE000EFFF},
    {"TPIU",       0xE0040000, 0xE0040FFF},
    {"ETM",        0xE0041000, 0xE0041FFF},
    {"ETB",        0xE0042000, 0xE0042FFF},
    {"ETF",        0xE0043000, 0xE0043FFF},
    {"MCM",        0xE0080000, 0xE0080FFF},
    {"MMCAU",      0xE0081000, 0xE0081FFF},
    {"ROMTable",   0xE00FF000, 0xE00FFFFF}, /* end of PPB                     */
};

const MemMap* memory_map_name(uint32_t addr)
{
    int i;
    const MemMap *map;

    /* find system memory region */
    map = g_mem_map;
    for(i = 0; i < UVISOR_ARRAY_COUNT(g_mem_map); i++)
        if((addr >= map->base) && (addr <= map->end))
            return map;
        else
            map++;

    return NULL;
}
