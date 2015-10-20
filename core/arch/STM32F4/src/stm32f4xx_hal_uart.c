/**
  ******************************************************************************
  * @file    stm32f4xx_hal_uart.c
  * @author  MCD Application Team
  * @version V1.3.0
  * @date    09-March-2015
  * @brief   UART HAL module driver.
  *          This file provides firmware functions to manage the following 
  *          functionalities of the Universal Asynchronous Receiver Transmitter (UART) peripheral:
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *           + Peripheral Control functions  
  *           + Peripheral State and Errors functions  
  *           
  @verbatim       
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
  [..]
    The UART HAL driver can be used as follows:
    
    (#) Declare a UART_HandleTypeDef handle structure.
  
    (#) Initialize the UART low level resources by implementing the HAL_UART_MspInit() API:
        (##) Enable the USARTx interface clock.
        (##) UART pins configuration:
            (+++) Enable the clock for the UART GPIOs.
            (+++) Configure these UART pins as alternate function pull-up.
        (##) NVIC configuration if you need to use interrupt process (HAL_UART_Transmit_IT()
             and HAL_UART_Receive_IT() APIs):
            (+++) Configure the USARTx interrupt priority.
            (+++) Enable the NVIC USART IRQ handle.
        (##) DMA Configuration if you need to use DMA process (HAL_UART_Transmit_DMA()
             and HAL_UART_Receive_DMA() APIs):
            (+++) Declare a DMA handle structure for the Tx/Rx stream.
            (+++) Enable the DMAx interface clock.
            (+++) Configure the declared DMA handle structure with the required 
                  Tx/Rx parameters.                
            (+++) Configure the DMA Tx/Rx Stream.
            (+++) Associate the initialized DMA handle to the UART DMA Tx/Rx handle.
            (+++) Configure the priority and enable the NVIC for the transfer complete 
                  interrupt on the DMA Tx/Rx Stream.

    (#) Program the Baud Rate, Word Length, Stop Bit, Parity, Hardware 
        flow control and Mode(Receiver/Transmitter) in the Init structure.

    (#) For the UART asynchronous mode, initialize the UART registers by calling
        the HAL_UART_Init() API.
    
    (#) For the UART Half duplex mode, initialize the UART registers by calling 
        the HAL_HalfDuplex_Init() API.
    
    (#) For the LIN mode, initialize the UART registers by calling the HAL_LIN_Init() API.
    
    (#) For the Multi-Processor mode, initialize the UART registers by calling 
        the HAL_MultiProcessor_Init() API.
        
     [..] 
       (@) The specific UART interrupts (Transmission complete interrupt, 
            RXNE interrupt and Error Interrupts) will be managed using the macros
            __HAL_UART_ENABLE_IT() and __HAL_UART_DISABLE_IT() inside the transmit 
            and receive process.
          
     [..] 
       (@) These APIs (HAL_UART_Init() and HAL_HalfDuplex_Init()) configure also the 
            low level Hardware GPIO, CLOCK, CORTEX...etc) by calling the customized 
            HAL_UART_MspInit() API.
          
     [..] 
        Three operation modes are available within this driver :     
  
     *** Polling mode IO operation ***
     =================================
     [..]    
       (+) Send an amount of data in blocking mode using HAL_UART_Transmit() 
       (+) Receive an amount of data in blocking mode using HAL_UART_Receive()
       
     *** Interrupt mode IO operation ***    
     ===================================
     [..]    
       (+) Send an amount of data in non blocking mode using HAL_UART_Transmit_IT() 
       (+) At transmission end of transfer HAL_UART_TxCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_TxCpltCallback
       (+) Receive an amount of data in non blocking mode using HAL_UART_Receive_IT() 
       (+) At reception end of transfer HAL_UART_RxCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_RxCpltCallback
       (+) In case of transfer Error, HAL_UART_ErrorCallback() function is executed and user can 
            add his own code by customization of function pointer HAL_UART_ErrorCallback

     *** DMA mode IO operation ***    
     ==============================
     [..] 
       (+) Send an amount of data in non blocking mode (DMA) using HAL_UART_Transmit_DMA() 
       (+) At transmission end of half transfer HAL_UART_TxHalfCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_TxHalfCpltCallback 
       (+) At transmission end of transfer HAL_UART_TxCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_TxCpltCallback
       (+) Receive an amount of data in non blocking mode (DMA) using HAL_UART_Receive_DMA() 
       (+) At reception end of half transfer HAL_UART_RxHalfCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_RxHalfCpltCallback 
       (+) At reception end of transfer HAL_UART_RxCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_RxCpltCallback
       (+) In case of transfer Error, HAL_UART_ErrorCallback() function is executed and user can 
            add his own code by customization of function pointer HAL_UART_ErrorCallback
       (+) Pause the DMA Transfer using HAL_UART_DMAPause()      
       (+) Resume the DMA Transfer using HAL_UART_DMAResume()  
       (+) Stop the DMA Transfer using HAL_UART_DMAStop()      
    
     *** UART HAL driver macros list ***
     ============================================= 
     [..]
       Below the list of most used macros in UART HAL driver.
       
      (+) __HAL_UART_ENABLE: Enable the UART peripheral 
      (+) __HAL_UART_DISABLE: Disable the UART peripheral     
      (+) __HAL_UART_GET_FLAG : Check whether the specified UART flag is set or not
      (+) __HAL_UART_CLEAR_FLAG : Clear the specified UART pending flag
      (+) __HAL_UART_ENABLE_IT: Enable the specified UART interrupt
      (+) __HAL_UART_DISABLE_IT: Disable the specified UART interrupt
      (+) __HAL_UART_GET_IT_SOURCE: Check whether the specified UART interrupt has occurred or not
      
     [..] 
       (@) You can refer to the UART HAL driver header file for more useful macros 
      
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal_conf.h"

/** @addtogroup STM32F4xx_HAL_Driver
  * @{
  */

/** @defgroup UART UART
  * @brief HAL UART module driver
  * @{
  */
#ifdef HAL_UART_MODULE_ENABLED
    
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @addtogroup UART_Private_Constants
  * @{
  */
#define UART_TIMEOUT_VALUE  22000
/**
  * @}
  */
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup UART_Private_Functions   UART Private Functions
  * @{
  */
static void UART_SetConfig (UART_HandleTypeDef *huart);
static HAL_StatusTypeDef UART_WaitOnFlag(UART_HandleTypeDef *huart, uint32_t Flag, FlagStatus Status);
/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/
/** @defgroup UART_Exported_Functions UART Exported Functions
  * @{
  */

/** @defgroup UART_Exported_Functions_Group1 Initialization and de-initialization functions 
  *  @brief    Initialization and Configuration functions 
  *
@verbatim    
===============================================================================
            ##### Initialization and Configuration functions #####
 ===============================================================================  
    [..]
    This subsection provides a set of functions allowing to initialize the USARTx or the UARTy 
    in asynchronous mode.
      (+) For the asynchronous mode only these parameters can be configured: 
        (++) Baud Rate
        (++) Word Length 
        (++) Stop Bit
        (++) Parity: If the parity is enabled, then the MSB bit of the data written
             in the data register is transmitted but is changed by the parity bit.
             Depending on the frame length defined by the M bit (8-bits or 9-bits),
             please refer to Reference manual for possible UART frame formats.           
        (++) Hardware flow control
        (++) Receiver/transmitter modes
        (++) Over Sampling Method
    [..]
    The HAL_UART_Init(), HAL_HalfDuplex_Init(), HAL_LIN_Init() and HAL_MultiProcessor_Init() APIs 
    follow respectively the UART asynchronous, UART Half duplex, LIN and Multi-Processor
    configuration procedures (details for the procedures are available in reference manual (RM0329)).

@endverbatim
  * @{
  */

/**
  * @brief  Initializes the UART mode according to the specified parameters in
  *         the UART_InitTypeDef and create the associated handle.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart)
{
  /* Check the UART handle allocation */
  if(huart == HAL_NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  if(huart->Init.HwFlowCtl != UART_HWCONTROL_NONE)
  { 
    /* The hardware flow control is available only for USART1, USART2, USART3 and USART6 */
    assert_param(IS_UART_HWFLOW_INSTANCE(huart->Instance));
    assert_param(IS_UART_HARDWARE_FLOW_CONTROL(huart->Init.HwFlowCtl));
  }
  else
  {
    assert_param(IS_UART_INSTANCE(huart->Instance));
  }
  assert_param(IS_UART_WORD_LENGTH(huart->Init.WordLength));
  assert_param(IS_UART_OVERSAMPLING(huart->Init.OverSampling));
  
  if(huart->State == HAL_UART_STATE_RESET)
  {  
    /* Allocate lock resource and initialize it */
    huart->Lock = HAL_UNLOCKED;
    /* Init the low level hardware */
    HAL_UART_MspInit(huart);
  }

  huart->State = HAL_UART_STATE_BUSY;

  /* Disable the peripheral */
  __HAL_UART_DISABLE(huart);
  
  /* Set the UART Communication parameters */
  UART_SetConfig(huart);
  
  /* In asynchronous mode, the following bits must be kept cleared: 
     - LINEN and CLKEN bits in the USART_CR2 register,
     - SCEN, HDSEL and IREN  bits in the USART_CR3 register.*/
  huart->Instance->CR2 &= ~(USART_CR2_LINEN | USART_CR2_CLKEN);
  huart->Instance->CR3 &= ~(USART_CR3_SCEN | USART_CR3_HDSEL | USART_CR3_IREN);
  
  /* Enable the peripheral */
  __HAL_UART_ENABLE(huart);
  
  /* Initialize the UART state */
  huart->ErrorCode = HAL_UART_ERROR_NONE;
  huart->State= HAL_UART_STATE_READY;
  
  return HAL_OK;
}

/**
  * @brief  Initializes the half-duplex mode according to the specified
  *         parameters in the UART_InitTypeDef and create the associated handle.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HalfDuplex_Init(UART_HandleTypeDef *huart)
{
  /* Check the UART handle allocation */
  if(huart == HAL_NULL)
  {
    return HAL_ERROR;
  }
 
  /* Check the parameters */ 
  assert_param(IS_UART_INSTANCE(huart->Instance));
  assert_param(IS_UART_WORD_LENGTH(huart->Init.WordLength));
  assert_param(IS_UART_OVERSAMPLING(huart->Init.OverSampling));

  if(huart->State == HAL_UART_STATE_RESET)
  { 
    /* Allocate lock resource and initialize it */
    huart->Lock = HAL_UNLOCKED;
    /* Init the low level hardware */
    HAL_UART_MspInit(huart);
  }

  huart->State = HAL_UART_STATE_BUSY;

  /* Disable the peripheral */
  __HAL_UART_DISABLE(huart);
  
  /* Set the UART Communication parameters */
  UART_SetConfig(huart);
  
  /* In half-duplex mode, the following bits must be kept cleared: 
     - LINEN and CLKEN bits in the USART_CR2 register,
     - SCEN and IREN bits in the USART_CR3 register.*/
  huart->Instance->CR2 &= ~(USART_CR2_LINEN | USART_CR2_CLKEN);
  huart->Instance->CR3 &= ~(USART_CR3_IREN | USART_CR3_SCEN);
  
  /* Enable the Half-Duplex mode by setting the HDSEL bit in the CR3 register */
  huart->Instance->CR3 |= USART_CR3_HDSEL;
 
  /* Enable the peripheral */
  __HAL_UART_ENABLE(huart);
  
  /* Initialize the UART state*/
  huart->ErrorCode = HAL_UART_ERROR_NONE;
  huart->State= HAL_UART_STATE_READY;
  
  return HAL_OK;
}

/**
  * @brief  Initializes the LIN mode according to the specified
  *         parameters in the UART_InitTypeDef and create the associated handle.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @param  BreakDetectLength: Specifies the LIN break detection length.
  *         This parameter can be one of the following values:
  *            @arg UART_LINBREAKDETECTLENGTH_10B: 10-bit break detection
  *            @arg UART_LINBREAKDETECTLENGTH_11B: 11-bit break detection
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_LIN_Init(UART_HandleTypeDef *huart, uint32_t BreakDetectLength)
{
  /* Check the UART handle allocation */
  if(huart == HAL_NULL)
  {
    return HAL_ERROR;
  }
   
  /* Check the parameters */
  assert_param(IS_UART_INSTANCE(huart->Instance));
  assert_param(IS_UART_LIN_BREAK_DETECT_LENGTH(BreakDetectLength));
  assert_param(IS_UART_LIN_WORD_LENGTH(huart->Init.WordLength));
  assert_param(IS_UART_LIN_OVERSAMPLING(huart->Init.OverSampling));
  
  if(huart->State == HAL_UART_STATE_RESET)
  {
    /* Allocate lock resource and initialize it */
    huart->Lock = HAL_UNLOCKED;
    /* Init the low level hardware */
    HAL_UART_MspInit(huart);
  }

  huart->State = HAL_UART_STATE_BUSY;

  /* Disable the peripheral */
  __HAL_UART_DISABLE(huart);
  
  /* Set the UART Communication parameters */
  UART_SetConfig(huart);
  
  /* In LIN mode, the following bits must be kept cleared: 
     - LINEN and CLKEN bits in the USART_CR2 register,
     - SCEN and IREN bits in the USART_CR3 register.*/
  huart->Instance->CR2 &= ~(USART_CR2_CLKEN);
  huart->Instance->CR3 &= ~(USART_CR3_HDSEL | USART_CR3_IREN | USART_CR3_SCEN);
  
  /* Enable the LIN mode by setting the LINEN bit in the CR2 register */
  huart->Instance->CR2 |= USART_CR2_LINEN;
  
  /* Set the USART LIN Break detection length. */
  huart->Instance->CR2 &= ~(USART_CR2_LBDL);
  huart->Instance->CR2 |= BreakDetectLength; 
  
  /* Enable the peripheral */
  __HAL_UART_ENABLE(huart);
  
  /* Initialize the UART state*/
  huart->ErrorCode = HAL_UART_ERROR_NONE;
  huart->State= HAL_UART_STATE_READY;
  
  return HAL_OK;
}

/**
  * @brief  Initializes the Multi-Processor mode according to the specified
  *         parameters in the UART_InitTypeDef and create the associated handle.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @param  Address: USART address
  * @param  WakeUpMethod: specifies the USART wake-up method.
  *          This parameter can be one of the following values:
  *            @arg UART_WAKEUPMETHOD_IDLELINE: Wake-up by an idle line detection
  *            @arg UART_WAKEUPMETHOD_ADDRESSMARK: Wake-up by an address mark
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_MultiProcessor_Init(UART_HandleTypeDef *huart, uint8_t Address, uint32_t WakeUpMethod)
{
  /* Check the UART handle allocation */
  if(huart == HAL_NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_UART_INSTANCE(huart->Instance));
  assert_param(IS_UART_WAKEUPMETHOD(WakeUpMethod));
  assert_param(IS_UART_ADDRESS(Address));
  assert_param(IS_UART_WORD_LENGTH(huart->Init.WordLength));
  assert_param(IS_UART_OVERSAMPLING(huart->Init.OverSampling));

  if(huart->State == HAL_UART_STATE_RESET)
  {
    /* Allocate lock resource and initialize it */
    huart->Lock = HAL_UNLOCKED;
    /* Init the low level hardware */
    HAL_UART_MspInit(huart);
  }

  huart->State = HAL_UART_STATE_BUSY;

  /* Disable the peripheral */
  __HAL_UART_DISABLE(huart);
  
  /* Set the UART Communication parameters */
  UART_SetConfig(huart);
  
  /* In Multi-Processor mode, the following bits must be kept cleared: 
     - LINEN and CLKEN bits in the USART_CR2 register,
     - SCEN, HDSEL and IREN  bits in the USART_CR3 register */
  huart->Instance->CR2 &= ~(USART_CR2_LINEN | USART_CR2_CLKEN);
  huart->Instance->CR3 &= ~(USART_CR3_SCEN | USART_CR3_HDSEL | USART_CR3_IREN);
  
  /* Clear the USART address */
  huart->Instance->CR2 &= ~(USART_CR2_ADD);
  /* Set the USART address node */
  huart->Instance->CR2 |= Address;
  
  /* Set the wake up method by setting the WAKE bit in the CR1 register */
  huart->Instance->CR1 &= ~(USART_CR1_WAKE);
  huart->Instance->CR1 |= WakeUpMethod;
  
  /* Enable the peripheral */
  __HAL_UART_ENABLE(huart);
  
  /* Initialize the UART state */
  huart->ErrorCode = HAL_UART_ERROR_NONE;
  huart->State= HAL_UART_STATE_READY;
  
  return HAL_OK;
}

/**
  * @brief  DeInitializes the UART peripheral. 
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart)
{
  /* Check the UART handle allocation */
  if(huart == HAL_NULL)
  {
    return HAL_ERROR;
  }
  
  /* Check the parameters */
  assert_param(IS_UART_INSTANCE(huart->Instance));

  huart->State = HAL_UART_STATE_BUSY;
  
  /* DeInit the low level hardware */
  HAL_UART_MspDeInit(huart);
  
  huart->ErrorCode = HAL_UART_ERROR_NONE;
  huart->State = HAL_UART_STATE_RESET;

  /* Process Lock */
  __HAL_UNLOCK(huart);

  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup UART_Exported_Functions_Group2 IO operation functions 
  *  @brief UART Transmit and Receive functions 
  *
@verbatim   
  ==============================================================================
                      ##### IO operation functions #####
  ==============================================================================  
  [..]
    This subsection provides a set of functions allowing to manage the UART asynchronous
    and Half duplex data transfers.

    (#) There are two modes of transfer:
       (++) Blocking mode: The communication is performed in polling mode. 
            The HAL status of all data processing is returned by the same function 
            after finishing transfer.  
       (++) Non blocking mode: The communication is performed using Interrupts 
            or DMA, these APIs return the HAL status.
            The end of the data processing will be indicated through the 
            dedicated UART IRQ when using Interrupt mode or the DMA IRQ when 
            using DMA mode.
            The HAL_UART_TxCpltCallback(), HAL_UART_RxCpltCallback() user callbacks 
            will be executed respectively at the end of the transmit or receive process.
            The HAL_UART_ErrorCallback() user callback will be executed when 
            a communication error is detected.

    (#) Blocking mode APIs are:
        (++) HAL_UART_Transmit()
        (++) HAL_UART_Receive() 
        
    (#) Non Blocking mode APIs with Interrupt are:
        (++) HAL_UART_Transmit_IT()
        (++) HAL_UART_Receive_IT()
        (++) HAL_UART_IRQHandler()

    (#) Non Blocking mode functions with DMA are:
        (++) HAL_UART_Transmit_DMA()
        (++) HAL_UART_Receive_DMA()

    (#) A set of Transfer Complete Callbacks are provided in non blocking mode:
        (++) HAL_UART_TxCpltCallback()
        (++) HAL_UART_RxCpltCallback()
        (++) HAL_UART_ErrorCallback()

    [..] 
      (@) In the Half duplex communication, it is forbidden to run the transmit 
          and receive process in parallel, the UART state HAL_UART_STATE_BUSY_TX_RX 
          can't be useful.
      
@endverbatim
  * @{
  */

/**
  * @brief  Sends an amount of data in blocking mode. 
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @param  pData: Pointer to data buffer
  * @param  Size: Amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
  uint16_t* tmp;
  uint32_t tmp1 = 0;
  
  tmp1 = huart->State;
  if((tmp1 == HAL_UART_STATE_READY) || (tmp1 == HAL_UART_STATE_BUSY_RX))
  {
    if((pData == HAL_NULL ) || (Size == 0)) 
    {
      return  HAL_ERROR;
    }
    
    /* Process Locked */
    __HAL_LOCK(huart);
    
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    /* Check if a non-blocking receive process is ongoing or not */
    if(huart->State == HAL_UART_STATE_BUSY_RX) 
    {
      huart->State = HAL_UART_STATE_BUSY_TX_RX;
    }
    else
    {
      huart->State = HAL_UART_STATE_BUSY_TX;
    }

    huart->TxXferSize = Size;
    huart->TxXferCount = Size;
    while(huart->TxXferCount > 0)
    {
      huart->TxXferCount--;
      if(huart->Init.WordLength == UART_WORDLENGTH_9B)
      {
        if(UART_WaitOnFlag(huart, UART_FLAG_TXE, RESET) != HAL_OK)
        { 
          return HAL_TIMEOUT;
        }
        tmp = (uint16_t*) pData;
        huart->Instance->DR = (*tmp & (uint16_t)0x01FF);
        if(huart->Init.Parity == UART_PARITY_NONE)
        {
          pData +=2;
        }
        else
        { 
          pData +=1;
        }
      } 
      else
      {
        if(UART_WaitOnFlag(huart, UART_FLAG_TXE, RESET) != HAL_OK)
        {
          return HAL_TIMEOUT;
        }
        huart->Instance->DR = (*pData++ & (uint8_t)0xFF);
      } 
    }
    
    if(UART_WaitOnFlag(huart, UART_FLAG_TC, RESET) != HAL_OK)
    { 
      return HAL_TIMEOUT;
    }
    
    /* Check if a non-blocking receive process is ongoing or not */
    if(huart->State == HAL_UART_STATE_BUSY_TX_RX) 
    {
      huart->State = HAL_UART_STATE_BUSY_RX;
    }
    else
    {
      huart->State = HAL_UART_STATE_READY;
    }
    
    /* Process Unlocked */
    __HAL_UNLOCK(huart);
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;   
  }
}

/**
  * @brief  Receives an amount of data in blocking mode. 
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @param  pData: Pointer to data buffer
  * @param  Size: Amount of data to be received
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{ 
  uint16_t* tmp;
  uint32_t tmp1 = 0;
  
  tmp1 = huart->State;
  if((tmp1 == HAL_UART_STATE_READY) || (tmp1 == HAL_UART_STATE_BUSY_TX))
  { 
    if((pData == HAL_NULL ) || (Size == 0)) 
    {
      return  HAL_ERROR;
    }
    
    /* Process Locked */
    __HAL_LOCK(huart);
    
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    /* Check if a non-blocking transmit process is ongoing or not */
    if(huart->State == HAL_UART_STATE_BUSY_TX) 
    {
      huart->State = HAL_UART_STATE_BUSY_TX_RX;
    }
    else
    {
      huart->State = HAL_UART_STATE_BUSY_RX;
    }
    
    huart->RxXferSize = Size; 
    huart->RxXferCount = Size;
    
    /* Check the remain data to be received */
    while(huart->RxXferCount > 0)
    {
      huart->RxXferCount--;
      if(huart->Init.WordLength == UART_WORDLENGTH_9B)
      {
        if(UART_WaitOnFlag(huart, UART_FLAG_RXNE, RESET) != HAL_OK)
        { 
          return HAL_TIMEOUT;
        }
        tmp = (uint16_t*) pData ;
        if(huart->Init.Parity == UART_PARITY_NONE)
        {
          *tmp = (uint16_t)(huart->Instance->DR & (uint16_t)0x01FF);
          pData +=2;
        }
        else
        {
          *tmp = (uint16_t)(huart->Instance->DR & (uint16_t)0x00FF);
          pData +=1;
        }

      } 
      else
      {
        if(UART_WaitOnFlag(huart, UART_FLAG_RXNE, RESET) != HAL_OK)
        { 
          return HAL_TIMEOUT;
        }
        if(huart->Init.Parity == UART_PARITY_NONE)
        {
          *pData++ = (uint8_t)(huart->Instance->DR & (uint8_t)0x00FF);
        }
        else
        {
          *pData++ = (uint8_t)(huart->Instance->DR & (uint8_t)0x007F);
        }
        
      }
    }
    
    /* Check if a non-blocking transmit process is ongoing or not */
    if(huart->State == HAL_UART_STATE_BUSY_TX_RX) 
    {
      huart->State = HAL_UART_STATE_BUSY_TX;
    }
    else
    {
      huart->State = HAL_UART_STATE_READY;
    } 
    /* Process Unlocked */
    __HAL_UNLOCK(huart);
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;   
  }
}


/** @defgroup UART_Exported_Functions_Group4 Peripheral State and Errors functions 
  *  @brief   UART State and Errors functions 
  *
@verbatim   
  ==============================================================================
                 ##### Peripheral State and Errors functions #####
  ==============================================================================  
 [..]
   This subsection provides a set of functions allowing to return the State of 
   UART communication process, return Peripheral Errors occurred during communication 
   process
   (+) HAL_UART_GetState() API can be helpful to check in run-time the state of the UART peripheral.
   (+) HAL_UART_GetError() check in run-time errors that could be occurred during communication. 

@endverbatim
  * @{
  */
  
/**
  * @brief  Returns the UART state.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL state
  */
HAL_UART_StateTypeDef HAL_UART_GetState(UART_HandleTypeDef *huart)
{
  return huart->State;
}

/**
* @brief  Return the UART error code
* @param  huart : pointer to a UART_HandleTypeDef structure that contains
  *              the configuration information for the specified UART.
* @retval UART Error Code
*/
uint32_t HAL_UART_GetError(UART_HandleTypeDef *huart)
{
  return huart->ErrorCode;
}

/**
  * @brief  Configures the UART peripheral. 
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
static void UART_SetConfig(UART_HandleTypeDef *huart)
{
  uint32_t tmpreg = 0x00;
  
  /* Check the parameters */
  assert_param(IS_UART_BAUDRATE(huart->Init.BaudRate));  
  assert_param(IS_UART_STOPBITS(huart->Init.StopBits));
  assert_param(IS_UART_PARITY(huart->Init.Parity));
  assert_param(IS_UART_MODE(huart->Init.Mode));

  /*-------------------------- USART CR2 Configuration -----------------------*/
  tmpreg = huart->Instance->CR2;

  /* Clear STOP[13:12] bits */
  tmpreg &= (uint32_t)~((uint32_t)USART_CR2_STOP);

  /* Configure the UART Stop Bits: Set STOP[13:12] bits according to huart->Init.StopBits value */
  tmpreg |= (uint32_t)huart->Init.StopBits;
  
  /* Write to USART CR2 */
  huart->Instance->CR2 = (uint32_t)tmpreg;

  /*-------------------------- USART CR1 Configuration -----------------------*/
  tmpreg = huart->Instance->CR1;

  /* Clear M, PCE, PS, TE and RE bits */
  tmpreg &= (uint32_t)~((uint32_t)(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE | \
                                   USART_CR1_RE | USART_CR1_OVER8));

  /* Configure the UART Word Length, Parity and mode: 
     Set the M bits according to huart->Init.WordLength value 
     Set PCE and PS bits according to huart->Init.Parity value
     Set TE and RE bits according to huart->Init.Mode value
     Set OVER8 bit according to huart->Init.OverSampling value */
  tmpreg |= (uint32_t)huart->Init.WordLength | huart->Init.Parity | huart->Init.Mode | huart->Init.OverSampling;
  
  /* Write to USART CR1 */
  huart->Instance->CR1 = (uint32_t)tmpreg;
  
  /*-------------------------- USART CR3 Configuration -----------------------*/  
  tmpreg = huart->Instance->CR3;
  
  /* Clear CTSE and RTSE bits */
  tmpreg &= (uint32_t)~((uint32_t)(USART_CR3_RTSE | USART_CR3_CTSE));
  
  /* Configure the UART HFC: Set CTSE and RTSE bits according to huart->Init.HwFlowCtl value */
  tmpreg |= huart->Init.HwFlowCtl;
  
  /* Write to USART CR3 */
  huart->Instance->CR3 = (uint32_t)tmpreg;
  
  /* Check the Over Sampling */
  if(huart->Init.OverSampling == UART_OVERSAMPLING_8)
  {
    /*-------------------------- USART BRR Configuration ---------------------*/
    if((huart->Instance == USART1) || (huart->Instance == USART6))
    {
      huart->Instance->BRR = UART_BRR_SAMPLING8(HAL_RCC_GetPCLK2Freq(), huart->Init.BaudRate);
    }
    else
    {
      huart->Instance->BRR = UART_BRR_SAMPLING8(HAL_RCC_GetPCLK1Freq(), huart->Init.BaudRate);
    }
  }
  else
  {
    /*-------------------------- USART BRR Configuration ---------------------*/
    if((huart->Instance == USART1) || (huart->Instance == USART6))
    {
      huart->Instance->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK2Freq(), huart->Init.BaudRate);
    }
    else
    {
      huart->Instance->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK1Freq(), huart->Init.BaudRate);
    }
  }
}

/**
  * @brief  This function handles UART Communication.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @param  Flag: specifies the UART flag to check.
  * @param  Status: The new Flag status (SET or RESET).
  * @retval HAL status
  */

static HAL_StatusTypeDef UART_WaitOnFlag(UART_HandleTypeDef *huart, uint32_t Flag, FlagStatus Status)
{
  /* Wait until flag is set */
  if(Status == RESET)
  {
    while(__HAL_UART_GET_FLAG(huart, Flag) == RESET);
  }
  else
  {
    while(__HAL_UART_GET_FLAG(huart, Flag) != RESET);
  }
  return HAL_OK;
}

/**
  * @}
  */

/**
 *   * @brief  UART MSP Init.
 *     * @param  huart: pointer to a UART_HandleTypeDef structure that contains
 *       *                the configuration information for the specified UART
 *       module.
 *         * @retval None
 *           */
 __weak void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	  /* NOTE: This function Should not be modified, when the callback is
	   * needed,
	   *            the HAL_UART_MspInit could be implemented in the user
	   *            file
	   *               */ 
}


#endif /* HAL_UART_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
