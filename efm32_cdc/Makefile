PROJECT:=efm32_cdc
ARCH:=EFM32GG
CPU:=$(ARCH)990F1024

APP_CFLAGS:=-Iconfig -Isrc -Ilib/emlib/inc -Isrc/helper -Isrc/usb/inc -Ilib/bsp
APP_SRC:=\
	lib/bsp/bsp_bcc.c \
	lib/bsp/bsp_dk_leds.c \
	lib/bsp/bsp_dk_mcuboard.c \
	lib/bsp/bsp_stk.c \
	lib/bsp/bsp_stk_leds.c \
	lib/bsp/bsp_trace.c \
	lib/emlib/src/em_acmp.c \
	lib/emlib/src/em_adc.c \
	lib/emlib/src/em_aes.c \
	lib/emlib/src/em_assert.c \
	lib/emlib/src/em_burtc.c \
	lib/emlib/src/em_cmu.c \
	lib/emlib/src/em_dac.c \
	lib/emlib/src/em_dbg.c \
	lib/emlib/src/em_dma.c \
	lib/emlib/src/em_ebi.c \
	lib/emlib/src/em_emu.c \
	lib/emlib/src/em_gpio.c \
	lib/emlib/src/em_i2c.c \
	lib/emlib/src/em_idac.c \
	lib/emlib/src/em_int.c \
	lib/emlib/src/em_lcd.c \
	lib/emlib/src/em_lesense.c \
	lib/emlib/src/em_letimer.c \
	lib/emlib/src/em_leuart.c \
	lib/emlib/src/em_mpu.c \
	lib/emlib/src/em_msc.c \
	lib/emlib/src/em_opamp.c \
	lib/emlib/src/em_pcnt.c \
	lib/emlib/src/em_prs.c \
	lib/emlib/src/em_rmu.c \
	lib/emlib/src/em_rtc.c \
	lib/emlib/src/em_system.c \
	lib/emlib/src/em_timer.c \
	lib/emlib/src/em_usart.c \
	lib/emlib/src/em_vcmp.c \
	lib/emlib/src/em_wdog.c \
	src/helper/dmactrl.c \
	src/usb/src/em_usbd.c \
	src/usb/src/em_usbdch9.c \
	src/usb/src/em_usbdep.c \
	src/usb/src/em_usbdint.c \
	src/usb/src/em_usbhal.c \
	src/usb/src/em_usbh.c \
	src/usb/src/em_usbhep.c \
	src/usb/src/em_usbhint.c \
	src/usb/src/em_usbtimer.c \
	src/main.c

include ../core/Makefile.rules

