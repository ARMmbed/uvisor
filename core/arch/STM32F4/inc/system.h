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

#define HW_IRQ_VECTORS 91

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
extern void WWDG_IRQn_Handler(void);
extern void PVD_IRQn_Handler(void);
extern void TAMP_STAMP_IRQn_Handler(void);
extern void RTC_WKUP_IRQn_Handler(void);
extern void FLASH_IRQn_Handler(void);
extern void RCC_IRQn_Handler(void);
extern void EXTI0_IRQn_Handler(void);
extern void EXTI1_IRQn_Handler(void);
extern void EXTI2_IRQn_Handler(void);
extern void EXTI3_IRQn_Handler(void);
extern void EXTI4_IRQn_Handler(void);
extern void DMA1_Stream0_IRQn_Handler(void);
extern void DMA1_Stream1_IRQn_Handler(void);
extern void DMA1_Stream2_IRQn_Handler(void);
extern void DMA1_Stream3_IRQn_Handler(void);
extern void DMA1_Stream4_IRQn_Handler(void);
extern void DMA1_Stream5_IRQn_Handler(void);
extern void DMA1_Stream6_IRQn_Handler(void);
extern void ADC_IRQn_Handler(void);
extern void CAN1_TX_IRQn_Handler(void);
extern void CAN1_RX0_IRQn_Handler(void);
extern void CAN1_RX1_IRQn_Handler(void);
extern void CAN1_SCE_IRQn_Handler(void);
extern void EXTI9_5_IRQn_Handler(void);
extern void TIM1_BRK_TIM9_IRQn_Handler(void);
extern void TIM1_UP_TIM10_IRQn_Handler(void);
extern void TIM1_TRG_COM_TIM11_IRQn_Handler(void);
extern void TIM1_CC_IRQn_Handler(void);
extern void TIM2_IRQn_Handler(void);
extern void TIM3_IRQn_Handler(void);
extern void TIM4_IRQn_Handler(void);
extern void I2C1_EV_IRQn_Handler(void);
extern void I2C1_ER_IRQn_Handler(void);
extern void I2C2_EV_IRQn_Handler(void);
extern void I2C2_ER_IRQn_Handler(void);
extern void SPI1_IRQn_Handler(void);
extern void SPI2_IRQn_Handler(void);
extern void USART1_IRQn_Handler(void);
extern void USART2_IRQn_Handler(void);
extern void USART3_IRQn_Handler(void);
extern void EXTI15_10_IRQn_Handler(void);
extern void RTC_Alarm_IRQn_Handler(void);
extern void OTG_FS_WKUP_IRQn_Handler(void);
extern void TIM8_BRK_TIM12_IRQn_Handler(void);
extern void TIM8_UP_TIM13_IRQn_Handler(void);
extern void TIM8_TRG_COM_TIM14_IRQn_Handler(void);
extern void TIM8_CC_IRQn_Handler(void);
extern void DMA1_Stream7_IRQn_Handler(void);
extern void FMC_IRQn_Handler(void);
extern void SDIO_IRQn_Handler(void);
extern void TIM5_IRQn_Handler(void);
extern void SPI3_IRQn_Handler(void);
extern void UART4_IRQn_Handler(void);
extern void UART5_IRQn_Handler(void);
extern void TIM6_DAC_IRQn_Handler(void);
extern void TIM7_IRQn_Handler(void);
extern void DMA2_Stream0_IRQn_Handler(void);
extern void DMA2_Stream1_IRQn_Handler(void);
extern void DMA2_Stream2_IRQn_Handler(void);
extern void DMA2_Stream3_IRQn_Handler(void);
extern void DMA2_Stream4_IRQn_Handler(void);
extern void ETH_IRQn_Handler(void);
extern void ETH_WKUP_IRQn_Handler(void);
extern void CAN2_TX_IRQn_Handler(void);
extern void CAN2_RX0_IRQn_Handler(void);
extern void CAN2_RX1_IRQn_Handler(void);
extern void CAN2_SCE_IRQn_Handler(void);
extern void OTG_FS_IRQn_Handler(void);
extern void DMA2_Stream5_IRQn_Handler(void);
extern void DMA2_Stream6_IRQn_Handler(void);
extern void DMA2_Stream7_IRQn_Handler(void);
extern void USART6_IRQn_Handler(void);
extern void I2C3_EV_IRQn_Handler(void);
extern void I2C3_ER_IRQn_Handler(void);
extern void OTG_HS_EP1_OUT_IRQn_Handler(void);
extern void OTG_HS_EP1_IN_IRQn_Handler(void);
extern void OTG_HS_WKUP_IRQn_Handler(void);
extern void OTG_HS_IRQn_Handler(void);
extern void DCMI_IRQn_Handler(void);
extern void HASH_RNG_IRQn_Handler(void);
extern void FPU_IRQn_Handler(void);
extern void UART7_IRQn_Handler(void);
extern void UART8_IRQn_Handler(void);
extern void SPI4_IRQn_Handler(void);
extern void SPI5_IRQn_Handler(void);
extern void SPI6_IRQn_Handler(void);
extern void SAI1_IRQn_Handler(void);
extern void LTDC_IRQn_Handler(void);
extern void LTDC_ER_IRQn_Handler(void);
extern void DMA2D_IRQn_Handler(void);

#endif/*__SYSTEM_H__*/
