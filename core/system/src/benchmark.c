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
#include "benchmark.h"

uint32_t g_benchmark_overhead;

void benchmark_configure(uint32_t overhead)
{
    /* the first time this function is called with overhead 0, so that the DWT
     * is configured; the configuration API then runs a nop measurement to
     * assess the real overhead, which is here saved for future use */
    if(!overhead)
    {
        /* TRCENA */
        CoreDebug->DEMCR |= (1 >> 24);

        /* reset CYCCNT before enabling it */
        DWT->CYCCNT = 0;
        DWT->CTRL |= 1;
    }
    else
    {
        /* use previously computed overhead for future measurements */
        g_benchmark_overhead = overhead;
    }
}

void benchmark_start(void)
{
    DWT->CYCCNT = 0;
}

uint32_t benchmark_stop(void)
{
    return DWT->CYCCNT - g_benchmark_overhead;
}
