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
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define HW_IRQ_VECTORS 86

/* all ISRs by default are weakly linked to the default handler */
extern void NonMaskableInt_IRQn_Handler(void);
extern void HardFault_IRQn_Handler(void);
extern void MemoryManagement_IRQn_Handler(void);
extern void BusFault_IRQn_Handler(void);
extern void UsageFault_IRQn_Handler(void);
extern void SVCall_IRQn_Handler(void);
extern void DebugMonitor_IRQn_Handler(void);
extern void PendSV_IRQn_Handler(void);
extern void SysTick_IRQn_Handler(void);
extern void DMA0_IRQn_Handler(void);
extern void DMA1_IRQn_Handler(void);
extern void DMA2_IRQn_Handler(void);
extern void DMA3_IRQn_Handler(void);
extern void DMA4_IRQn_Handler(void);
extern void DMA5_IRQn_Handler(void);
extern void DMA6_IRQn_Handler(void);
extern void DMA7_IRQn_Handler(void);
extern void DMA8_IRQn_Handler(void);
extern void DMA9_IRQn_Handler(void);
extern void DMA10_IRQn_Handler(void);
extern void DMA11_IRQn_Handler(void);
extern void DMA12_IRQn_Handler(void);
extern void DMA13_IRQn_Handler(void);
extern void DMA14_IRQn_Handler(void);
extern void DMA15_IRQn_Handler(void);
extern void DMA_Error_IRQn_Handler(void);
extern void MCM_IRQn_Handler(void);
extern void FTFE_IRQn_Handler(void);
extern void Read_Collision_IRQn_Handler(void);
extern void LVD_LVW_IRQn_Handler(void);
extern void LLW_IRQn_Handler(void);
extern void Watchdog_IRQn_Handler(void);
extern void RNG_IRQn_Handler(void);
extern void I2C0_IRQn_Handler(void);
extern void I2C1_IRQn_Handler(void);
extern void SPI0_IRQn_Handler(void);
extern void SPI1_IRQn_Handler(void);
extern void I2S0_Tx_IRQn_Handler(void);
extern void I2S0_Rx_IRQn_Handler(void);
extern void UART0_LON_IRQn_Handler(void);
extern void UART0_RX_TX_IRQn_Handler(void);
extern void UART0_ERR_IRQn_Handler(void);
extern void UART1_RX_TX_IRQn_Handler(void);
extern void UART1_ERR_IRQn_Handler(void);
extern void UART2_RX_TX_IRQn_Handler(void);
extern void UART2_ERR_IRQn_Handler(void);
extern void UART3_RX_TX_IRQn_Handler(void);
extern void UART3_ERR_IRQn_Handler(void);
extern void ADC0_IRQn_Handler(void);
extern void CMP0_IRQn_Handler(void);
extern void CMP1_IRQn_Handler(void);
extern void FTM0_IRQn_Handler(void);
extern void FTM1_IRQn_Handler(void);
extern void FTM2_IRQn_Handler(void);
extern void CMT_IRQn_Handler(void);
extern void RTC_IRQn_Handler(void);
extern void RTC_Seconds_IRQn_Handler(void);
extern void PIT0_IRQn_Handler(void);
extern void PIT1_IRQn_Handler(void);
extern void PIT2_IRQn_Handler(void);
extern void PIT3_IRQn_Handler(void);
extern void PDB0_IRQn_Handler(void);
extern void USB0_IRQn_Handler(void);
extern void USBDCD_IRQn_Handler(void);
extern void Reserved71_IRQn_Handler(void);
extern void DAC0_IRQn_Handler(void);
extern void MCG_IRQn_Handler(void);
extern void LPTimer_IRQn_Handler(void);
extern void PORTA_IRQn_Handler(void);
extern void PORTB_IRQn_Handler(void);
extern void PORTC_IRQn_Handler(void);
extern void PORTD_IRQn_Handler(void);
extern void PORTE_IRQn_Handler(void);
extern void SWI_IRQn_Handler(void);
extern void SPI2_IRQn_Handler(void);
extern void UART4_RX_TX_IRQn_Handler(void);
extern void UART4_ERR_IRQn_Handler(void);
extern void UART5_RX_TX_IRQn_Handler(void);
extern void UART5_ERR_IRQn_Handler(void);
extern void CMP2_IRQn_Handler(void);
extern void FTM3_IRQn_Handler(void);
extern void DAC1_IRQn_Handler(void);
extern void ADC1_IRQn_Handler(void);
extern void I2C2_IRQn_Handler(void);
extern void CAN0_ORed_Message_buffer_IRQn_Handler_Handler(void);
extern void CAN0_Bus_Off_IRQn_Handler(void);
extern void CAN0_Error_IRQn_Handler(void);
extern void CAN0_Tx_Warning_IRQn_Handler(void);
extern void CAN0_Rx_Warning_IRQn_Handler(void);
extern void CAN0_Wake_Up_IRQn_Handler(void);
extern void SDHC_IRQn_Handler(void);
extern void ENET_1588_Timer_IRQn_Handler(void);
extern void ENET_Transmit_IRQn_Handler(void);
extern void ENET_Receive_IRQn_Handler(void);
extern void ENET_Error_IRQn_Handler(void);

#endif/*__SYSTEM_H__*/
