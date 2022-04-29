/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define ENABLE_SD_CARD  (HAL_GPIO_WritePin (SDMMC_PSO_GPIO_Port, SDMMC_PSO_Pin, GPIO_PIN_RESET))
#define ENABLE_ETHERNET HAL_GPIO_WritePin(E_RES_GPIO_Port, E_RES_Pin, GPIO_PIN_SET)
	#define ENABLE_12V  (HAL_GPIO_WritePin (EN12V_GPIO_Port, EN12V_Pin, GPIO_PIN_SET))	//включение 12V нагрузки
	
#define LED_RED(x) ((x)? (HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET)) : (HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET))); 
#define LED_GREEN(x) ((x)? (HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET)) : (HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET))); 

#define DE(x) ((x)? (HAL_GPIO_WritePin (DE_485_GPIO_Port, DE_485_Pin, GPIO_PIN_SET)) : (HAL_GPIO_WritePin (DE_485_GPIO_Port, DE_485_Pin, GPIO_PIN_RESET))); 
#define RE(x) ((x)? (HAL_GPIO_WritePin (RE_485_GPIO_Port, RE_485_Pin, GPIO_PIN_SET)) : (HAL_GPIO_WritePin (RE_485_GPIO_Port, RE_485_Pin, GPIO_PIN_RESET))); 
#define RS485_TX DE(1);RE(1);
#define RS485_RX DE(0);RE(0);

//LCD_RESET
#define LCD_RST1  HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET); 
#define LCD_RST0  HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
	
//   LCD_DC
#define LCD_DC1  GPIOD->BSRR=GPIO_BSRR_BS_7 //GPIO_PIN_SET 
#define LCD_DC0  GPIOD->BSRR=GPIO_BSRR_BR_7 //GPIO_PIN_RESET 

//  LCD_CS
#define LCD_CS1   GPIOB->BSRR=GPIO_BSRR_BS_5  //GPIO_PIN_SET
#define LCD_CS0   GPIOB->BSRR=GPIO_BSRR_BR_5 //GPIO_PIN_RESET

#define BUZZER_ON  HAL_TIM_PWM_Start (&htim4, TIM_CHANNEL_4); //писк бипером
#define BUZZER_OFF  HAL_TIM_PWM_Stop (&htim4, TIM_CHANNEL_4);

/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
