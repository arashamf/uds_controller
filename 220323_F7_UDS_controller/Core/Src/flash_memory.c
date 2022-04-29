	#include "flash_memory.h"

	extern char UART3_msg_TX [RS232_BUFFER_SIZE];

//********************************************************************************************************************************//
void write_flash (uint32_t adress, uint8_t * ip_buffer)
{
	uint32_t code_error;
	HAL_FLASH_Unlock(); // разблокировать флеш
	erase_flash_7sector ();  // Очистим 7 страницу памяти 
	for (size_t count = 0; count < 4; count++)
	{                                         
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, adress, *(ip_buffer + count)) != HAL_OK) //запись и обработчик ошибки
		{
	    code_error = HAL_FLASH_GetError();
	    sprintf (UART3_msg_TX, "error = %u\r\n", code_error);
	    UART3_SendString ((char*)UART3_msg_TX);	
	    return;
		}
		adress += 1; //смещение адреса на 1 байт
	}	
	HAL_FLASH_Lock(); // заблокировать флеш
}

//********************************************************************************************************************************//
uint8_t read_flash (uint32_t adress, uint8_t * buffer_ip)
{
	uint32_t buffer = 0;
	buffer = *(__IO uint32_t*) adress; //считывание 32 битового слова
	for (uint8_t count = 0; count < 4; count++)
	{
		
		*(buffer_ip + count) = (uint8_t)(buffer >> (8*count)); //запоминания 1 байта слова
		if (*(buffer_ip + count) == 0xFF)
			return 0;
	}
	return 1;
}

//********************************************************************************************************************************//
void erase_flash_7sector (void)
{
	while (FLASH->SR & FLASH_SR_BSY) {};
	FLASH->CR |= FLASH_CR_SER; //операция стирания
	FLASH->CR |= (FLASH_CR_SNB_0 | FLASH_CR_SNB_1 | FLASH_CR_SNB_2); //стирание 7 сектора
	FLASH->CR |= FLASH_CR_STRT; //начало операции стирания 
	while (!(FLASH->SR & FLASH_SR_BSY)) {};	
}
//********************************************************************************************************************************//
