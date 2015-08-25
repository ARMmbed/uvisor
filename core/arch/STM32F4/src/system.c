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
#include "halt.h"
#include "unvic.h"
#include "system.h"

/* all ISRs by default are weakly linked to the default handler */
void UVISOR_ALIAS(isr_default_sys_handler) NonMaskableInt_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) HardFault_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) MemoryManagement_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) BusFault_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) UsageFault_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) SVCall_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) DebugMonitor_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) PendSV_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_sys_handler) SysTick_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     WWDG_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PVD_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TAMP_STAMP_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     RTC_WKUP_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     FLASH_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     RCC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     EXTI0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     EXTI1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     EXTI2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     EXTI3_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     EXTI4_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA1_Stream0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA1_Stream1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA1_Stream2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA1_Stream3_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA1_Stream4_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA1_Stream5_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA1_Stream6_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     ADC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN1_TX_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN1_RX0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN1_RX1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN1_SCE_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     EXTI9_5_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM1_BRK_TIM9_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM1_UP_TIM10_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM1_TRG_COM_TIM11_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM1_CC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM3_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM4_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2C1_EV_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2C1_ER_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2C2_EV_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2C2_ER_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SPI1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SPI2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     USART1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     USART2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     USART3_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     EXTI15_10_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     RTC_Alarm_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     OTG_FS_WKUP_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM8_BRK_TIM12_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM8_UP_TIM13_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM8_TRG_COM_TIM14_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM8_CC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA1_Stream7_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     FMC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SDIO_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM5_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SPI3_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART4_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART5_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM6_DAC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     TIM7_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2_Stream0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2_Stream1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2_Stream2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2_Stream3_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2_Stream4_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     ETH_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     ETH_WKUP_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN2_TX_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN2_RX0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN2_RX1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN2_SCE_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     OTG_FS_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2_Stream5_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2_Stream6_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2_Stream7_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     USART6_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2C3_EV_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2C3_ER_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     OTG_HS_EP1_OUT_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     OTG_HS_EP1_IN_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     OTG_HS_WKUP_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     OTG_HS_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DCMI_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     HASH_RNG_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     FPU_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART7_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART8_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SPI4_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SPI5_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SPI6_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SAI1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     LTDC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     LTDC_ER_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2D_IRQn_Handler(void);

/* vector table; it will be placed in Flash */
__attribute__((section(".isr")))
const TIsrVector g_isr_vector[ISR_VECTORS] =
{
	/* initial stack pointer */
	(TIsrVector)&__stack_top__,

	/* system interrupts */
	&main_entry,                         /* -15 */
	NonMaskableInt_IRQn_Handler,         /* -14 */
	HardFault_IRQn_Handler,              /* -13 */
	MemoryManagement_IRQn_Handler,       /* -12 */
	BusFault_IRQn_Handler,               /* -11 */
	UsageFault_IRQn_Handler,             /* -10 */
	isr_default_sys_handler,             /* - 9 */
	isr_default_sys_handler,             /* - 8 */
	isr_default_sys_handler,             /* - 7 */
	isr_default_sys_handler,             /* - 6 */
	SVCall_IRQn_Handler,                 /* - 5 */
	DebugMonitor_IRQn_Handler,           /* - 4 */
	isr_default_sys_handler,             /* - 3 */
	PendSV_IRQn_Handler,                 /* - 2 */
	SysTick_IRQn_Handler,                /* - 1 */

	/* peripheral interrupts */
	WWDG_IRQn_Handler,                   /*   0 */
	PVD_IRQn_Handler,                    /*   1 */
	TAMP_STAMP_IRQn_Handler,             /*   2 */
	RTC_WKUP_IRQn_Handler,               /*   3 */
	FLASH_IRQn_Handler,                  /*   4 */
	RCC_IRQn_Handler,                    /*   5 */
	EXTI0_IRQn_Handler,                  /*   6 */
	EXTI1_IRQn_Handler,                  /*   7 */
	EXTI2_IRQn_Handler,                  /*   8 */
	EXTI3_IRQn_Handler,                  /*   9 */
	EXTI4_IRQn_Handler,                  /*  10 */
	DMA1_Stream0_IRQn_Handler,           /*  11 */
	DMA1_Stream1_IRQn_Handler,           /*  12 */
	DMA1_Stream2_IRQn_Handler,           /*  13 */
	DMA1_Stream3_IRQn_Handler,           /*  14 */
	DMA1_Stream4_IRQn_Handler,           /*  15 */
	DMA1_Stream5_IRQn_Handler,           /*  16 */
	DMA1_Stream6_IRQn_Handler,           /*  17 */
	ADC_IRQn_Handler,                    /*  18 */
	CAN1_TX_IRQn_Handler,                /*  19 */
	CAN1_RX0_IRQn_Handler,               /*  20 */
	CAN1_RX1_IRQn_Handler,               /*  21 */
	CAN1_SCE_IRQn_Handler,               /*  22 */
	EXTI9_5_IRQn_Handler,                /*  23 */
	TIM1_BRK_TIM9_IRQn_Handler,          /*  24 */
	TIM1_UP_TIM10_IRQn_Handler,          /*  25 */
	TIM1_TRG_COM_TIM11_IRQn_Handler,     /*  26 */
	TIM1_CC_IRQn_Handler,                /*  27 */
	TIM2_IRQn_Handler,                   /*  28 */
	TIM3_IRQn_Handler,                   /*  29 */
	TIM4_IRQn_Handler,                   /*  30 */
	I2C1_EV_IRQn_Handler,                /*  31 */
	I2C1_ER_IRQn_Handler,                /*  32 */
	I2C2_EV_IRQn_Handler,                /*  33 */
	I2C2_ER_IRQn_Handler,                /*  34 */
	SPI1_IRQn_Handler,                   /*  35 */
	SPI2_IRQn_Handler,                   /*  36 */
	USART1_IRQn_Handler,                 /*  37 */
	USART2_IRQn_Handler,                 /*  38 */
	USART3_IRQn_Handler,                 /*  39 */
	EXTI15_10_IRQn_Handler,              /*  40 */
	RTC_Alarm_IRQn_Handler,              /*  41 */
	OTG_FS_WKUP_IRQn_Handler,            /*  42 */
	TIM8_BRK_TIM12_IRQn_Handler,         /*  43 */
	TIM8_UP_TIM13_IRQn_Handler,          /*  44 */
	TIM8_TRG_COM_TIM14_IRQn_Handler,     /*  45 */
	TIM8_CC_IRQn_Handler,                /*  46 */
	DMA1_Stream7_IRQn_Handler,           /*  47 */
	FMC_IRQn_Handler,                    /*  48 */
	SDIO_IRQn_Handler,                   /*  49 */
	TIM5_IRQn_Handler,                   /*  50 */
	SPI3_IRQn_Handler,                   /*  51 */
	UART4_IRQn_Handler,                  /*  52 */
	UART5_IRQn_Handler,                  /*  53 */
	TIM6_DAC_IRQn_Handler,               /*  54 */
	TIM7_IRQn_Handler,                   /*  55 */
	DMA2_Stream0_IRQn_Handler,           /*  56 */
	DMA2_Stream1_IRQn_Handler,           /*  57 */
	DMA2_Stream2_IRQn_Handler,           /*  58 */
	DMA2_Stream3_IRQn_Handler,           /*  59 */
	DMA2_Stream4_IRQn_Handler,           /*  60 */
	ETH_IRQn_Handler,                    /*  61 */
	ETH_WKUP_IRQn_Handler,               /*  62 */
	CAN2_TX_IRQn_Handler,                /*  63 */
	CAN2_RX0_IRQn_Handler,               /*  64 */
	CAN2_RX1_IRQn_Handler,               /*  65 */
	CAN2_SCE_IRQn_Handler,               /*  66 */
	OTG_FS_IRQn_Handler,                 /*  67 */
	DMA2_Stream5_IRQn_Handler,           /*  68 */
	DMA2_Stream6_IRQn_Handler,           /*  69 */
	DMA2_Stream7_IRQn_Handler,           /*  70 */
	USART6_IRQn_Handler,                 /*  71 */
	I2C3_EV_IRQn_Handler,                /*  72 */
	I2C3_ER_IRQn_Handler,                /*  73 */
	OTG_HS_EP1_OUT_IRQn_Handler,         /*  74 */
	OTG_HS_EP1_IN_IRQn_Handler,          /*  75 */
	OTG_HS_WKUP_IRQn_Handler,            /*  76 */
	OTG_HS_IRQn_Handler,                 /*  77 */
	DCMI_IRQn_Handler,                   /*  78 */
	isr_default_handler,                 /*  79 */
	HASH_RNG_IRQn_Handler,               /*  80 */
	FPU_IRQn_Handler,                    /*  81 */
	UART7_IRQn_Handler,                  /*  82 */
	UART8_IRQn_Handler,                  /*  83 */
	SPI4_IRQn_Handler,                   /*  84 */
	SPI5_IRQn_Handler,                   /*  85 */
	SPI6_IRQn_Handler,                   /*  86 */
	SAI1_IRQn_Handler,                   /*  87 */
	LTDC_IRQn_Handler,                   /*  88 */
	LTDC_ER_IRQn_Handler,                /*  89 */
	DMA2D_IRQn_Handler,                  /*  90 */
};

void UVISOR_NAKED UVISOR_NORETURN isr_default_sys_handler(void)
{
	/* the multiplexer will execute the correct handler depending on the IRQn */
	vmpu_sys_mux();
}

void isr_default_handler(void)
{
	/* the multiplexer will execute the correct handler depending on the IRQn */
	unvic_isr_mux();
}
