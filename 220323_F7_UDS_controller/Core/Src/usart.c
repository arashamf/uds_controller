/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */

extern osMessageQId RS485_msg_Queue;
extern osMutexId mutex_RS485_Handle;

char counter_byte_UART1 = 0;

extern char UART3_msg_TX [RS232_BUFFER_SIZE];
extern char RS485_RXbuffer [RX_BUFFER_SIZE];
extern char RS485_TXbuffer [4];
extern uint8_t cell_state [MAX_SELL+1][5];
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

}
/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 57600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = TXD_Pin|RXD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, TXD_Pin|RXD_Pin);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8|GPIO_PIN_9);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
//********************************************************************************************************************************//
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
//	char *ptr_UART1_buf = UART_msg_RX;
	if(huart==&huart1)
  {
		osMessagePut (RS485_msg_Queue, (uint32_t)RS485_RXbuffer, 10);
		osThreadYield (); //переключение контектса
	}
}

//********************************************************************************************************************************//
void UART1_SendByte(char b)
{
	int timeout = 300000;
	while ((USART1->ISR & UART_FLAG_TXE) == (uint16_t)RESET)
	{
		if(timeout--==0)
			return;
	}
	if ((USART1->ISR & USART_ISR_TC) == USART_ISR_TC)
	{
		/* Transmit Data */
		USART1->TDR = (b & (uint16_t)0x01FF);
	}
	//wait for trasmitt
	while ((USART1->ISR & UART_FLAG_TC) == (uint16_t)RESET){}		
}

//********************************************************************************************************************************//
void UART1_SendString (const char * text)
{
	while(*text)
	{
		UART1_SendByte(*text);
		text++;
	}
}

//********************************************************************************************************************************//
void PutCommandToCell (char * buffer_command)
{
	taskENTER_CRITICAL(); //вход в критическую секцию
	RS485_TX; //режим на передачу		
	UART1_SendString (buffer_command); //передача сообщения
	HAL_UART_Receive_IT(&huart1, (uint8_t*)RS485_RXbuffer, 6); //ожидание получение сообщения (6 байт) от ячейки
	RS485_RX; //режим на приём
	taskEXIT_CRITICAL(); //выход из критической секции			
}

//******************************************широковещательная отправка команды ****************************************************//
void command_AllCell (uint8_t typecommand, uint8_t NumberOfCell)
{
	osEvent event; 
	uint8_t * ptr_RS485_msg;
	
	RS485_TXbuffer [3] = typecommand + 0x30; //передача типа запроса, 1 - 'open','0 - 'close'
	if (osMutexWait (mutex_RS485_Handle, 50) == osOK) //ожидание и захват мьютекса в течение xx мс
	{
		for (size_t count = 0; count < NumberOfCell; count++)
		{	
			RS485_TXbuffer [1] = ((count+1)/10 + 0x30); //старший символ номера ячейки	
			RS485_TXbuffer [2] = ((count+1)%10 + 0x30); //младший символ номера ячейки
		
			PutCommandToCell (RS485_TXbuffer);
		
			event = osMessageGet(RS485_msg_Queue, 2); //ожидание появления данных в очереди
			if (event.status == osEventMessage) //если данные с ответом от ячейки появились в очереди
			{	
				(void)event.value.v;
				ptr_RS485_msg = (uint8_t *)event.value.v;
				memcpy ((cell_state + count), (ptr_RS485_msg+1), 5); //копирование 5 символов сообщения от ячейки, начиная с 2 элемента
			}	
			osDelay (400);
		}		
	}
	osMutexRelease (mutex_RS485_Handle);
}
//********************************************************************************************************************************//

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
