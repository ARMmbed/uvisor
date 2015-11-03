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
#include <vmpu.h>

static const MemMap g_mem_map[] = {
    {">Flash",   0x00000000,                FLASH_END              - FLASH_BASE                   },
    {"Flash",    FLASH_BASE,                FLASH_END                                             },
    {"CCM",      CCMDATARAM_BASE,           CCMDATARAM_END,                                       },
    {"SRAM1",    SRAM1_BASE,                SRAM2_BASE             - 1                            },
    {"SRAM2",    SRAM2_BASE,                SRAM3_BASE             - 1                            },
    {"SRAM3",    SRAM3_BASE,                SRAM3_BASE             + 0x10000 - 1                  },
    {"bbSRAM",   VMPU_SRAM_BITBAND_START,   VMPU_SRAM_BITBAND_END                                 },
    {"TIM2",     TIM2_BASE,                 TIM2_BASE              + sizeof(TIM_TypeDef)          },
    {"TIM3",     TIM3_BASE,                 TIM3_BASE              + sizeof(TIM_TypeDef)          },
    {"TIM4",     TIM4_BASE,                 TIM4_BASE              + sizeof(TIM_TypeDef)          },
    {"TIM5",     TIM5_BASE,                 TIM5_BASE              + sizeof(TIM_TypeDef)          },
    {"TIM6",     TIM6_BASE,                 TIM6_BASE              + sizeof(TIM_TypeDef)          },
    {"TIM7",     TIM7_BASE,                 TIM7_BASE              + sizeof(TIM_TypeDef)          },
    {"TIM12",    TIM12_BASE,                TIM12_BASE             + sizeof(TIM_TypeDef)          },
    {"TIM13",    TIM13_BASE,                TIM13_BASE             + sizeof(TIM_TypeDef)          },
    {"TIM14",    TIM14_BASE,                TIM14_BASE             + sizeof(TIM_TypeDef)          },
    {"RTC",      RTC_BASE,                  RTC_BASE               + sizeof(RTC_TypeDef)          },
    {"WWDG",     WWDG_BASE,                 WWDG_BASE              + sizeof(WWDG_TypeDef)         },
    {"IWDG",     IWDG_BASE,                 IWDG_BASE              + sizeof(IWDG_TypeDef)         },
    {"I2S2e",    I2S2ext_BASE,              I2S2ext_BASE           + sizeof(SPI_TypeDef)          },
    {"SPI2",     SPI2_BASE,                 SPI2_BASE              + sizeof(SPI_TypeDef)          },
    {"SPI3",     SPI3_BASE,                 SPI3_BASE              + sizeof(SPI_TypeDef)          },
    {"I2S3e",    I2S3ext_BASE,              I2S3ext_BASE           + sizeof(SPI_TypeDef)          },
    {"USART2",   USART2_BASE,               USART2_BASE            + sizeof(USART_TypeDef)        },
    {"USART3",   USART3_BASE,               USART3_BASE            + sizeof(USART_TypeDef)        },
    {"UART4",    UART4_BASE,                UART4_BASE             + sizeof(USART_TypeDef)        },
    {"UART5",    UART5_BASE,                UART5_BASE             + sizeof(USART_TypeDef)        },
    {"I2C1",     I2C1_BASE,                 I2C1_BASE              + sizeof(I2C_TypeDef)          },
    {"I2C2",     I2C2_BASE,                 I2C2_BASE              + sizeof(I2C_TypeDef)          },
    {"I2C3",     I2C3_BASE,                 I2C3_BASE              + sizeof(I2C_TypeDef)          },
    {"CAN1",     CAN1_BASE,                 CAN1_BASE              + sizeof(CAN_TypeDef)          },
    {"CAN2",     CAN2_BASE,                 CAN2_BASE              + sizeof(CAN_TypeDef)          },
    {"PWR",      PWR_BASE,                  PWR_BASE               + sizeof(PWR_TypeDef)          },
    {"DAC",      DAC_BASE,                  DAC_BASE               + sizeof(DAC_TypeDef)          },
    {"UART7",    UART7_BASE,                UART7_BASE             + sizeof(USART_TypeDef)        },
    {"UART8",    UART8_BASE,                UART8_BASE             + sizeof(USART_TypeDef)        },
    {"TIM1",     TIM1_BASE,                 TIM1_BASE              + sizeof(TIM_TypeDef)          },
    {"TIM8",     TIM8_BASE,                 TIM8_BASE              + sizeof(TIM_TypeDef)          },
    {"USART1",   USART1_BASE,               USART1_BASE            + sizeof(USART_TypeDef)        },
    {"USART6",   USART6_BASE,               USART6_BASE            + sizeof(USART_TypeDef)        },
    {"ADC1",     ADC1_BASE,                 ADC1_BASE              + sizeof(ADC_TypeDef)          },
    {"ADC2",     ADC2_BASE,                 ADC2_BASE              + sizeof(ADC_TypeDef)          },
    {"ADC3",     ADC3_BASE,                 ADC3_BASE              + sizeof(ADC_TypeDef)          },
    {"ADC",      ADC_BASE,                  ADC_BASE               + sizeof(ADC_Common_TypeDef)   },
    {"SDIO",     SDIO_BASE,                 SDIO_BASE              + sizeof(SDIO_TypeDef)         },
    {"SPI1",     SPI1_BASE,                 SPI1_BASE              + sizeof(SPI_TypeDef)          },
    {"SPI4",     SPI4_BASE,                 SPI4_BASE              + sizeof(SPI_TypeDef)          },
    {"SYSCFG",   SYSCFG_BASE,               SYSCFG_BASE            + sizeof(SYSCFG_TypeDef)       },
    {"EXTI",     EXTI_BASE,                 EXTI_BASE              + sizeof(EXTI_TypeDef)         },
    {"TIM9",     TIM9_BASE,                 TIM9_BASE              + sizeof(TIM_TypeDef)          },
    {"TIM10",    TIM10_BASE,                TIM10_BASE             + sizeof(TIM_TypeDef)          },
    {"TIM11",    TIM11_BASE,                TIM11_BASE             + sizeof(TIM_TypeDef)          },
    {"SPI5",     SPI5_BASE,                 SPI5_BASE              + sizeof(SPI_TypeDef)          },
    {"SPI6",     SPI6_BASE,                 SPI6_BASE              + sizeof(SPI_TypeDef)          },
    {"SAI1",     SAI1_BASE,                 SAI1_BASE              + sizeof(SAI_TypeDef)          },
    {"SAI1bA",   SAI1_Block_A_BASE,         SAI1_Block_A_BASE      + sizeof(SAI_Block_TypeDef)    },
    {"SAI1bB",   SAI1_Block_B_BASE,         SAI1_Block_B_BASE      + sizeof(SAI_Block_TypeDef)    },
    {"LTDC",     LTDC_BASE,                 LTDC_BASE              + sizeof(LTDC_TypeDef)         },
    {"LTDCl1",   LTDC_Layer1_BASE,          LTDC_Layer1_BASE       + sizeof(LTDC_Layer_TypeDef)   },
    {"LTDCl2",   LTDC_Layer2_BASE,          LTDC_Layer2_BASE       + sizeof(LTDC_Layer_TypeDef)   },
    {"GPIOA",    GPIOA_BASE,                GPIOA_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOB",    GPIOB_BASE,                GPIOB_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOC",    GPIOC_BASE,                GPIOC_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOD",    GPIOD_BASE,                GPIOD_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOE",    GPIOE_BASE,                GPIOE_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOF",    GPIOF_BASE,                GPIOF_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOG",    GPIOG_BASE,                GPIOG_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOH",    GPIOH_BASE,                GPIOH_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOI",    GPIOI_BASE,                GPIOI_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOJ",    GPIOJ_BASE,                GPIOJ_BASE             + sizeof(GPIO_TypeDef)         },
    {"GPIOK",    GPIOK_BASE,                GPIOK_BASE             + sizeof(GPIO_TypeDef)         },
    {"CRC",      CRC_BASE,                  CRC_BASE               + sizeof(CRC_TypeDef)          },
    {"RCC",      RCC_BASE,                  RCC_BASE               + sizeof(RCC_TypeDef)          },
    {"FLASHr",   FLASH_R_BASE,              FLASH_R_BASE           + sizeof(FLASH_TypeDef)        },
    {"BKPSRAM",  BKPSRAM_BASE,              BKPSRAM_BASE           + 0x1000 - 1                   },
    {"DMA1",     DMA1_BASE,                 DMA1_BASE              + sizeof(DMA_TypeDef)          },
    {"DMA1s0",   DMA1_Stream0_BASE,         DMA1_Stream0_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA1s1",   DMA1_Stream1_BASE,         DMA1_Stream1_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA1s2",   DMA1_Stream2_BASE,         DMA1_Stream2_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA1s3",   DMA1_Stream3_BASE,         DMA1_Stream3_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA1s4",   DMA1_Stream4_BASE,         DMA1_Stream4_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA1s5",   DMA1_Stream5_BASE,         DMA1_Stream5_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA1s6",   DMA1_Stream6_BASE,         DMA1_Stream6_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA1s7",   DMA1_Stream7_BASE,         DMA1_Stream7_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA2",     DMA2_BASE,                 DMA2_BASE              + sizeof(TIM_TypeDef)          },
    {"DMA2s0",   DMA2_Stream0_BASE,         DMA2_Stream0_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA2s1",   DMA2_Stream1_BASE,         DMA2_Stream1_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA2s2",   DMA2_Stream2_BASE,         DMA2_Stream2_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA2s3",   DMA2_Stream3_BASE,         DMA2_Stream3_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA2s4",   DMA2_Stream4_BASE,         DMA2_Stream4_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA2s5",   DMA2_Stream5_BASE,         DMA2_Stream5_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA2s6",   DMA2_Stream6_BASE,         DMA2_Stream6_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"DMA2s7",   DMA2_Stream7_BASE,         DMA2_Stream7_BASE      + sizeof(DMA_Stream_TypeDef)   },
    {"ETH",      ETH_BASE,                  ETH_BASE               + sizeof(ETH_TypeDef)          },
    {"DMA2D",    DMA2D_BASE,                DMA2D_BASE             + sizeof(DMA2D_TypeDef)        },
    {"UOTGhs",   USB_OTG_HS_PERIPH_BASE,    USB_OTG_HS_PERIPH_BASE + sizeof(USB_OTG_GlobalTypeDef)},
    {"UOTGfs",   USB_OTG_FS_PERIPH_BASE,    USB_OTG_FS_PERIPH_BASE + sizeof(USB_OTG_GlobalTypeDef)},
    {"DCMI",     DCMI_BASE,                 DCMI_BASE              + sizeof(DCMI_TypeDef)         },
    {"RNG",      RNG_BASE,                  RNG_BASE               + sizeof(RNG_TypeDef)          },
    {"FMCb1",    FMC_Bank1_R_BASE,          FMC_Bank1_R_BASE       + sizeof(FMC_Bank1_TypeDef)    },
    {"FMCb1e",   FMC_Bank1E_R_BASE,         FMC_Bank1E_R_BASE      + sizeof(FMC_Bank1E_TypeDef)   },
    {"FMCb23",   FMC_Bank2_3_R_BASE,        FMC_Bank2_3_R_BASE     + sizeof(FMC_Bank2_3_TypeDef)  },
    {"FMCb4",    FMC_Bank4_R_BASE,          FMC_Bank4_R_BASE       + sizeof(FMC_Bank4_TypeDef)    },
    {"FMCb56",   FMC_Bank5_6_R_BASE,        FMC_Bank5_6_R_BASE     + sizeof(FMC_Bank5_6_TypeDef)  },
    {"ITM",      ITM_BASE,                  ITM_BASE               + sizeof(ITM_Type)             },
    {"DWT",      DWT_BASE,                  DWT_BASE               + sizeof(DWT_Type)             },
    {"SCS",      SCS_BASE,                  SCS_BASE               + sizeof(SCnSCB_Type)          },
    {"SysT",     SysTick_BASE,              SysTick_BASE           + sizeof(SysTick_Type)         },
    {"NVIC",     NVIC_BASE,                 NVIC_BASE              + sizeof(NVIC_Type)            },
    {"SCB",      SCB_BASE,                  SCB_BASE               + sizeof(SCB_Type)             },
    {"MPU",      MPU_BASE,                  MPU_BASE               + sizeof(MPU_Type)             },
    {"CrDbg",    CoreDebug_BASE,            CoreDebug_BASE         + sizeof(CoreDebug_Type)       },
    {"FPU",      FPU_BASE,                  FPU_BASE               + sizeof(FPU_Type)             },
    {"TPI",      TPI_BASE,                  TPI_BASE               + sizeof(TPI_Type)             },
    {"bbPERIPH", VMPU_PERIPH_BITBAND_START, VMPU_PERIPH_BITBAND_END                               },
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
