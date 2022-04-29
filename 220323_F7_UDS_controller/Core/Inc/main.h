/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SDMMC_PSO_Pin GPIO_PIN_2
#define SDMMC_PSO_GPIO_Port GPIOE
#define SIDE_COVER_Pin GPIO_PIN_3
#define SIDE_COVER_GPIO_Port GPIOE
#define TOP_COVER_Pin GPIO_PIN_4
#define TOP_COVER_GPIO_Port GPIOE
#define MASTER_KEY_Pin GPIO_PIN_5
#define MASTER_KEY_GPIO_Port GPIOE
#define E_RES_Pin GPIO_PIN_3
#define E_RES_GPIO_Port GPIOA
#define EN12V_Pin GPIO_PIN_12
#define EN12V_GPIO_Port GPIOD
#define LED_R_Pin GPIO_PIN_13
#define LED_R_GPIO_Port GPIOD
#define LED_G_Pin GPIO_PIN_14
#define LED_G_GPIO_Port GPIOD
#define DE_485_Pin GPIO_PIN_6
#define DE_485_GPIO_Port GPIOC
#define RE_485_Pin GPIO_PIN_7
#define RE_485_GPIO_Port GPIOC
#define TXD_Pin GPIO_PIN_9
#define TXD_GPIO_Port GPIOA
#define RXD_Pin GPIO_PIN_10
#define RXD_GPIO_Port GPIOA
#define SDMMC_CD_Pin GPIO_PIN_0
#define SDMMC_CD_GPIO_Port GPIOD
#define LCD_A0_Pin GPIO_PIN_7
#define LCD_A0_GPIO_Port GPIOD
#define LCD_RESET_Pin GPIO_PIN_4
#define LCD_RESET_GPIO_Port GPIOB
#define LCD_CS_Pin GPIO_PIN_5
#define LCD_CS_GPIO_Port GPIOB
#define RTC_RST_Pin GPIO_PIN_0
#define RTC_RST_GPIO_Port GPIOE
#define SD_OC_Pin GPIO_PIN_1
#define SD_OC_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */
#define RS232_BUFFER_SIZE 100
#define RX_BUFFER_SIZE 7
#define MAX_SELL 8 //количество ячеек
#define RTC_ADDRESS 0xD0 //i2c-адрес мк RTC со сдвигом влево на один разряд 
#define FLASH_IP_ADDRESS 0x080FF030 //адресс для хранения ip-адреса
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
