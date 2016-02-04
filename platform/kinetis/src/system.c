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
void UVISOR_ALIAS(isr_default_handler)     DMA0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA3_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA4_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA5_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA6_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA7_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA8_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA9_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA10_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA11_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA12_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA13_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA14_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA15_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DMA_Error_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     MCM_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     FTFE_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     Read_Collision_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     LVD_LVW_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     LLW_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     Watchdog_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     RNG_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2C0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2C1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SPI0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SPI1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2S0_Tx_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2S0_Rx_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART0_LON_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART0_RX_TX_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART0_ERR_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART1_RX_TX_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART1_ERR_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART2_RX_TX_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART2_ERR_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART3_RX_TX_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART3_ERR_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     ADC0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CMP0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CMP1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     FTM0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     FTM1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     FTM2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CMT_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     RTC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     RTC_Seconds_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PIT0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PIT1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PIT2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PIT3_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PDB0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     USB0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     USBDCD_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     Reserved71_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DAC0_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     MCG_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     LPTimer_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PORTA_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PORTB_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PORTC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PORTD_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     PORTE_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SWI_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SPI2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART4_RX_TX_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART4_ERR_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART5_RX_TX_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     UART5_ERR_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CMP2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     FTM3_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     DAC1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     ADC1_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     I2C2_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN0_ORed_Message_buffer_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN0_Bus_Off_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN0_Error_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN0_Tx_Warning_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN0_Rx_Warning_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     CAN0_Wake_Up_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     SDHC_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     ENET_1588_Timer_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     ENET_Transmit_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     ENET_Receive_IRQn_Handler(void);
void UVISOR_ALIAS(isr_default_handler)     ENET_Error_IRQn_Handler(void);

/* vector table; it will be placed in Flash */
__attribute__((section(".isr")))
const TIsrVector g_isr_vector[ISR_VECTORS] =
{
	/* initial stack pointer */
	(TIsrVector)&__stack_top__,

	/* system interrupts */
	&main_entry,                           /* -15 */
	NonMaskableInt_IRQn_Handler,           /* -14 */
	HardFault_IRQn_Handler,                /* -13 */
	MemoryManagement_IRQn_Handler,         /* -12 */
	BusFault_IRQn_Handler,                 /* -11 */
	UsageFault_IRQn_Handler,               /* -10 */
	isr_default_sys_handler,               /* - 9 */
	isr_default_sys_handler,               /* - 8 */
	isr_default_sys_handler,               /* - 7 */
	isr_default_sys_handler,               /* - 6 */
	SVCall_IRQn_Handler,                   /* - 5 */
	DebugMonitor_IRQn_Handler,             /* - 4 */
	isr_default_sys_handler,               /* - 3 */
	PendSV_IRQn_Handler,                   /* - 2 */
	SysTick_IRQn_Handler,                  /* - 1 */

	/* peripheral interrupts */
	DMA0_IRQn_Handler,                     /*   0 */
	DMA1_IRQn_Handler,                     /*   1 */
	DMA2_IRQn_Handler,                     /*   2 */
	DMA3_IRQn_Handler,                     /*   3 */
	DMA4_IRQn_Handler,                     /*   4 */
	DMA5_IRQn_Handler,                     /*   5 */
	DMA6_IRQn_Handler,                     /*   6 */
	DMA7_IRQn_Handler,                     /*   7 */
	DMA8_IRQn_Handler,                     /*   8 */
	DMA9_IRQn_Handler,                     /*   9 */
	DMA10_IRQn_Handler,                    /*  10 */
	DMA11_IRQn_Handler,                    /*  11 */
	DMA12_IRQn_Handler,                    /*  12 */
	DMA13_IRQn_Handler,                    /*  13 */
	DMA14_IRQn_Handler,                    /*  14 */
	DMA15_IRQn_Handler,                    /*  15 */
	DMA_Error_IRQn_Handler,                /*  16 */
	MCM_IRQn_Handler,                      /*  17 */
	FTFE_IRQn_Handler,                     /*  18 */
	Read_Collision_IRQn_Handler,           /*  19 */
	LVD_LVW_IRQn_Handler,                  /*  20 */
	LLW_IRQn_Handler,                      /*  21 */
	Watchdog_IRQn_Handler,                 /*  22 */
	RNG_IRQn_Handler,                      /*  23 */
	I2C0_IRQn_Handler,                     /*  24 */
	I2C1_IRQn_Handler,                     /*  25 */
	SPI0_IRQn_Handler,                     /*  26 */
	SPI1_IRQn_Handler,                     /*  27 */
	I2S0_Tx_IRQn_Handler,                  /*  28 */
	I2S0_Rx_IRQn_Handler,                  /*  29 */
	UART0_LON_IRQn_Handler,                /*  30 */
	UART0_RX_TX_IRQn_Handler,              /*  31 */
	UART0_ERR_IRQn_Handler,                /*  32 */
	UART1_RX_TX_IRQn_Handler,              /*  33 */
	UART1_ERR_IRQn_Handler,                /*  34 */
	UART2_RX_TX_IRQn_Handler,              /*  35 */
	UART2_ERR_IRQn_Handler,                /*  36 */
	UART3_RX_TX_IRQn_Handler,              /*  37 */
	UART3_ERR_IRQn_Handler,                /*  38 */
	ADC0_IRQn_Handler,                     /*  39 */
	CMP0_IRQn_Handler,                     /*  40 */
	CMP1_IRQn_Handler,                     /*  41 */
	FTM0_IRQn_Handler,                     /*  42 */
	FTM1_IRQn_Handler,                     /*  43 */
	FTM2_IRQn_Handler,                     /*  44 */
	CMT_IRQn_Handler,                      /*  45 */
	RTC_IRQn_Handler,                      /*  46 */
	RTC_Seconds_IRQn_Handler,              /*  47 */
	PIT0_IRQn_Handler,                     /*  48 */
	PIT1_IRQn_Handler,                     /*  49 */
	PIT2_IRQn_Handler,                     /*  50 */
	PIT3_IRQn_Handler,                     /*  51 */
	PDB0_IRQn_Handler,                     /*  52 */
	USB0_IRQn_Handler,                     /*  53 */
	USBDCD_IRQn_Handler,                   /*  54 */
	Reserved71_IRQn_Handler,               /*  55 */
	DAC0_IRQn_Handler,                     /*  56 */
	MCG_IRQn_Handler,                      /*  57 */
	LPTimer_IRQn_Handler,                  /*  58 */
	PORTA_IRQn_Handler,                    /*  59 */
	PORTB_IRQn_Handler,                    /*  60 */
	PORTC_IRQn_Handler,                    /*  61 */
	PORTD_IRQn_Handler,                    /*  62 */
	PORTE_IRQn_Handler,                    /*  63 */
	SWI_IRQn_Handler,                      /*  64 */
	SPI2_IRQn_Handler,                     /*  65 */
	UART4_RX_TX_IRQn_Handler,              /*  66 */
	UART4_ERR_IRQn_Handler,                /*  67 */
	UART5_RX_TX_IRQn_Handler,              /*  68 */
	UART5_ERR_IRQn_Handler,                /*  69 */
	CMP2_IRQn_Handler,                     /*  70 */
	FTM3_IRQn_Handler,                     /*  71 */
	DAC1_IRQn_Handler,                     /*  72 */
	ADC1_IRQn_Handler,                     /*  73 */
	I2C2_IRQn_Handler,                     /*  74 */
	CAN0_ORed_Message_buffer_IRQn_Handler, /*  75 */
	CAN0_Bus_Off_IRQn_Handler,             /*  76 */
	CAN0_Error_IRQn_Handler,               /*  77 */
	CAN0_Tx_Warning_IRQn_Handler,          /*  78 */
	CAN0_Rx_Warning_IRQn_Handler,          /*  79 */
	CAN0_Wake_Up_IRQn_Handler,             /*  80 */
	SDHC_IRQn_Handler,                     /*  81 */
	ENET_1588_Timer_IRQn_Handler,          /*  82 */
	ENET_Transmit_IRQn_Handler,            /*  83 */
	ENET_Receive_IRQn_Handler,             /*  84 */
	ENET_Error_IRQn_Handler,               /*  85 */
};

void UVISOR_NAKED UVISOR_NORETURN isr_default_sys_handler(void)
{
	/* the multiplexer will execute the correct handler depending on the IRQn */
	vmpu_sys_mux();
}

void UVISOR_NAKED UVISOR_NORETURN isr_default_handler(void)
{
	/* the multiplexer will execute the correct handler depending on the IRQn */
	unvic_isr_mux();
}
