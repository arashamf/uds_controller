/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "iwdg.h"
#include "lwip.h"
#include "sdmmc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7735.h"
#include "DefineFont.h"
#include "delay.h"
#include "stdio.h"
#include "string.h"
#include "flash_memory.h"
#include "flash_W25M02.h"
#include "myWeb.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
char UART3_msg_TX [RS232_BUFFER_SIZE];
char RS485_RXbuffer [RX_BUFFER_SIZE] = {0};
char RS485_TXbuffer [TX_BUFFER_SIZE];

uint8_t prev_cell_state [MAX_CELL+1][7] = {0}; //массив с данными от ячеек

uint8_t save_ip_adress [4] = {0}; //здесь  хранится ip-адрес, считанный из flash

char mod_ip_adress [16]; //ip-адрес в символьной форме (например 192.168.001.060) для регистрации и отображения
uint8_t ptr_ipset = 0; //указатель на элемент массива с ip-адресом
uint8_t dimension = 5; //смещение по горизонтали при отображении на дисплее ip-адреса посимвольно
uint8_t button_status = (BUTTON_2_Pin | BUTTON_3_Pin); //сохранённое состояние кнопок
uint8_t status = 0; //полученное состояние кнопок 
uint8_t count = 0; //переменная для счётчика
char buffer_char; //буффер символа
uint16_t sum_number = 0; //число для проверки корректности ip адреса
uint8_t flag_ip_error = 1; //флаг проверки корректности введённого ip адреса
uint8_t buffer_ip [4]; //буффер с ip адресом в формате четырёх uint8_t


char logSDPath; // User logical drive path 
FIL wlogfile;     //файловый объект для записи
FIL rlogfile;     //файловый объект для чтения 
FATFS log_fs ;    // рабочая область (file system object) для логических диска 

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_SDMMC1_SD_Init();
  MX_FATFS_Init();
  MX_SPI3_Init();
  MX_TIM4_Init();
  MX_SPI4_Init();
  /* USER CODE BEGIN 2 */
	SPI4_CS1_OFF;
	SPI4_CS2_OFF;
	
	lcdInit();
	ClearLcdMemory();
	LCD_SetFont(Arial_15x17,black);
	LCD_ShowString(5,10,"controller_staring...");
	LCD_Refresh();
	
	ENABLE_ETHERNET; //включения ETHERNET модуля
	ENABLE_12V; //включения контроллеров ячеек
	RS485_RX; //режим на приём
	ENABLE_SD_CARD;	//включения питания SD карты
	
	if ((READ_BIT(BUTTON_1_GPIO_Port->IDR,  BUTTON_1_Pin)) == 0) //если кнопка 1 нажата
	{
		delay_ms(100); //дребезг
		if ((READ_BIT(BUTTON_1_GPIO_Port->IDR,  BUTTON_1_Pin)) == 0) //если кнопка 1 нажата
		{
			while ((READ_BIT(BUTTON_1_GPIO_Port->IDR,  BUTTON_1_Pin)) == 0) {}; //ожидание отпускание кнопки 1			
			ClearLcdMemory(); //очистим дисплея
			LCD_SetFont(Arial_15x17,black); //установка шрифта
			LCD_ShowString(30,10,"setting ip-adress:");		
				if ((read_flash (FLASH_IP_ADDRESS, save_ip_adress)) == 0) //считывание из флэша пользовательский ip, если он был туда записан
			{
				save_ip_adress[0] = 192; //если нет пользовательского ip, то используется ip по умолчанию
				save_ip_adress[1] = 168;
				save_ip_adress[2] = 1;
				save_ip_adress[3] = 60;
			}
			convert_ip (mod_ip_adress, save_ip_adress); //перевод полученного ip из числового в символный вид
			for (count = 0; count < 15; count++)
			{							
				if (count == ptr_ipset)
					{LCD_SetFont(Arial_22x23,red);}
				else
					{LCD_SetFont(Arial_22x23,black);}
				LCD_FastShowChar(dimension, 35, mod_ip_adress [count]);	
				if ((count == 3)||(count == 7)||(count == 11)) //если символ после точки
					{dimension += 5;}
				else
					{dimension += 11;}
			}	
			LCD_Refresh(); //обновляем изображение на дисплее
			while ((READ_BIT(BUTTON_1_GPIO_Port->IDR,  BUTTON_1_Pin)) == BUTTON_1_Pin) //пока кнопка 1 не нажата
			{
				if ((READ_BIT(BUTTON_3_GPIO_Port->IDR,  BUTTON_2_Pin | BUTTON_3_Pin)) != button_status) //проверка нажатия 2 и 3 кнопки
				{
					delay_ms(50); //дребезг
					if ((status = READ_BIT (BUTTON_3_GPIO_Port->IDR,  BUTTON_2_Pin | BUTTON_3_Pin)) != button_status)
					{
						button_status = status;
						if ((button_status & BUTTON_3_Pin ) == 0) //если была нажата кнопка 3, то изменяется номер цифры
						{
							ptr_ipset++;
							if ((ptr_ipset == 3) || (ptr_ipset == 7) || (ptr_ipset == 11)) //пропуск символа точки
								{ptr_ipset++;}
							if (ptr_ipset == 15) //если это был последний символ
								{ptr_ipset = 0;}
						}	
						if ((button_status & BUTTON_2_Pin) == 0) //если была нажата кнопка 2
						{
							mod_ip_adress [ptr_ipset]++; //то увеличиваем активное число на единицу 
							if (mod_ip_adress [ptr_ipset] > 0x39) //если символ был равен 9
								{mod_ip_adress [ptr_ipset] = 0x30;} 
						}
						
						dimension = 5; //установка начального смещения по горизонтали
						ClearLcdMemory();  //очистим дисплея
						LCD_SetFont(Arial_15x17,black); //установка шрифта
						LCD_ShowString(30,10,"setting ip-adress:");	
						for (count = 0; count < 15; count++)
						{
							//buffer_char = mod_ip_adress [count];
							if (count == ptr_ipset)
								{LCD_SetFont(Arial_22x23,red);}
							else
								{LCD_SetFont(Arial_22x23,black);}
							LCD_FastShowChar(dimension, 35, mod_ip_adress [count]);	
							if ((count == 3)||(count == 7)||(count == 11)) //если символ после точки
								{dimension += 5;}
							else
								{dimension += 11;}
						}
						LCD_Refresh(); //обновляем изображение на дисплее						
					}
				}
			};
			ptr_ipset=0; flag_ip_error = 1;
			for (count = 0; count < 4; count++)
			{
				sum_number = (mod_ip_adress[ptr_ipset++]-0x30)*100;
				sum_number += (mod_ip_adress[ptr_ipset++]-0x30)*10;
				sum_number += (mod_ip_adress[ptr_ipset++]-0x30);								
				if (sum_number > 255)
				{					
					flag_ip_error = 0; 
					break;
				}
				buffer_ip [count] = sum_number;
				ptr_ipset++;
			}	
			LCD_SetFont(Arial_15x17,black); //установка шрифта
			ClearLcdMemory();  //очистим дисплея
			if (flag_ip_error == 1) //если адрес корректный
			{
				LCD_ShowString(20,50,"saving  ip-address!");
				write_flash (FLASH_IP_ADDRESS, 	buffer_ip); //сохраним адрес во флеше
			}
			else
				{LCD_ShowString(15,50,"incorrect ip-address!");}
			LCD_Refresh(); //обновляем изображение на дисплее
			delay_ms (2000);
		}
	}
	MX_IWDG_Init();
	prev_cell_state[0][0] = '1'; 
	prev_cell_state[0][1] = '4'; //мастер ячейка	
	for (uint8_t count_cell = 1; count_cell <= MAX_CELL; count_cell++)
	{
		prev_cell_state[count_cell][0] = (count_cell/10 + 0x30); //установка номера ячейки
		prev_cell_state[count_cell][1] = (count_cell%10 + 0x30);
		prev_cell_state[count_cell][2] = 0; //начальная инициализация
	}
	
	if (READ_BIT (RCC->CSR, RCC_CSR_IWDGRSTF)) //если установлен бит IWDGRST (флаг срабатывания IWDG)
	{
		SET_BIT (RCC->CSR, RCC_CSR_RMVF); //сброс флага срабатывания IWDG
		sprintf (UART3_msg_TX,"UDS_controller_start_after_iwdg_reset\r\n");
		UART3_SendString ((char*)UART3_msg_TX);	
	}
	else
	{
		sprintf (UART3_msg_TX,"UDS_controller_start\r\n");
		UART3_SendString ((char*)UART3_msg_TX);	
	}
	
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();
  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART3
                              |RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_SDMMC1
                              |RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  PeriphClkInitStruct.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0xC0400000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}
/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM5 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM5) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
