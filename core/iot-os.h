#ifndef __IOT_OS_H__
#define __IOT_OS_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <tfp_printf.h>
#define dprintf tfp_printf

#define PACKED __attribute__((packed))
#define WEAK __attribute__ ((weak))
#define ALIAS(f) __attribute__ ((weak, alias (#f)))


/* per-project-configuration */
#include <config.h>

/* EFM32 definitions */
#include <em_device.h>

/* IOT-OS specific includes */
#include <isr.h>
#include <reset.h>
#include <linker.h>

#endif/*__IOT_OS_H__*/
