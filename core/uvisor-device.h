#ifndef __UVISOR_DEVICE_H__
#define __UVISOR_DEVICE_H__

#if   defined(EFM32GG)
#  include <em_device.h>
#elif defined(MK64F)
#  include <MK64F12.h>
#else
#  error "unknown ARCH in Makefile"
#endif

#endif/*__UVISOR_DEVICE_H__*/
