/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
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

#include "fatfs.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */
extern char logSDPath;  // User logical drive path 
extern FIL wlogfile;     //файловый объект для записи
extern FIL rlogfile;     //файловый объект для чтения 
extern FATFS log_fs ;    // рабочая область (file system object) для логических диска

extern char UART3_msg_TX [RS232_BUFFER_SIZE];
/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  return 0;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */
/**********************************************************************************************************************/
uint8_t mount_card (FATFS* fs)
{
	uint8_t result = 0; //код возврата функций FatFs
	/*if((result = FATFS_LinkDriver(&SD_Driver, &logSDPath)) != 0)
		return result;*/
//	MX_SDMMC1_SD_Init();
	if((result = BSP_SD_Init()) != MSD_OK)
		return result;
	result = f_mount (fs, (TCHAR const*)"/", 1);  //монтирование рабочей области для каждого тома (если 0 - отложенное монтирование, 1 - немедленное монтирование)  
	return result;
}

/**********************************************************************************************************************/
void reset_SD_card (void)
{
	DISABLE_SD_CARD; //выключение питания SD
	osDelay (2);
	ENABLE_SD_CARD; //включение питания SD
	osDelay (2);
	taskENTER_CRITICAL(); //вход в критическую секцию
//	HAL_SD_MspDeInit(&hsd1);
//	FATFS_UnLinkDriver(SDPath);
	MX_SDMMC1_SD_Init();
	FATFS_LinkDriver(&SD_Driver, SDPath);
	BSP_SD_Init();
	taskEXIT_CRITICAL(); //выход из критической секции		
	UART3_SendString ("SD card reset\r\n");
//	osDelay (2);
}

/**********************************************************************************************************************/
uint32_t read_txt (FIL* fp, const char * FileName, char * buffer, uint32_t byteswritten)
{
	FRESULT result;
	uint32_t bytesread = 0; 
	//char buffer [byteswritten+2];
	if ((result = f_open (fp, FileName, FA_READ)) != FR_OK)
	{
		sprintf (UART3_msg_TX,"file %s is not open. errorcode=%u\r\n", FileName, result);
		UART3_SendString (UART3_msg_TX);
	}
	else
	{
		result = f_read (fp, buffer, byteswritten, &bytesread); //byteswritten-количество байт, которые необходимо прочитать
		if((bytesread	!=	byteswritten)||(result	!=	FR_OK)) //если прочитано меньше, чем планировалось
		{
			sprintf (UART3_msg_TX,"file_read_error, code=%u, byte read=%u\r\n", result, bytesread);
			UART3_SendString (UART3_msg_TX);
		}
		
		if ((result = f_close(fp)) != FR_OK)
		{
			sprintf (UART3_msg_TX,"incorrect_close_readfile. code=%u\r\n", result);
			UART3_SendString (UART3_msg_TX);
		}
	}
	return bytesread; //bytesread - количество прочитанных байт
}

/**********************************************************************************************************************/
FRESULT write_reg (FIL* fp, const char * FileName, const char * txtbuf)
{
	FRESULT result;
	//uint32_t byteswritten = 0; //счетчики записи файла
	if ((result = f_open (fp, FileName, FA_OPEN_APPEND|FA_WRITE)) != FR_OK) //Если файл существует, то он будет открыт, если же нет, то будет создан новый файл
	{	
		sprintf (UART3_msg_TX,"file_for_write_is_not_open. errorcode=%u\r\n", result);
		UART3_SendString (UART3_msg_TX);
		return result;
	}
	//byteswritten = write_txt (fp, result, txtbuf);	
	write_txt (fp, result, txtbuf);	
	if ((result = f_close(fp)) != FR_OK)
	{
		sprintf (UART3_msg_TX,"incorrect_sync_writefile. code=%u\r\n", result);
		UART3_SendString (UART3_msg_TX);
	}
	return result;
}

/**********************************************************************************************************************/
uint32_t write_txt (FIL* fp, FRESULT result, const char* buffer)
{
	uint32_t bytesize = 0; 
//	unsigned long filesize = 0; //размер файла в байтах
	//if ((filesize = f_size(fp)) > 0)
//		f_lseek (fp, filesize); //Перемещение в конец файла для добавления новых данных
	result = f_write (fp, buffer, strlen(buffer), &bytesize); //bytesize - Количество записанных байт
	if((bytesize	!=	strlen(buffer))||(result	!=	FR_OK))
	{
		sprintf (UART3_msg_TX,"file_write_error, %u bytes write, code=%u\r\n", bytesize, result);
		UART3_SendString (UART3_msg_TX);
		return 0;
	}
	else {
		return bytesize;} //bytesize - количество записанных байт
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
