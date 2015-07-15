/***************************************************************
 * This confidential and  proprietary  software may be used only
 * as authorised  by  a licensing  agreement  from  ARM  Limited
 *
 *             (C) COPYRIGHT 2013-2015 ARM Limited
 *                      ALL RIGHTS RESERVED
 *
 *  The entire notice above must be reproduced on all authorised
 *  copies and copies  may only be made to the  extent permitted
 *  by a licensing agreement from ARM Limited.
 *
 ***************************************************************/
#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

void uvisor_benchmark_configure(void);
void uvisor_benchmark_start(void);
uint32_t uvisor_benchmark_stop(void);

#endif/*__BENCHMARK_H__*/
