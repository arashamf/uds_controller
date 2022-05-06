#include "rtc.h"


extern char UART3_msg_TX [RS232_BUFFER_SIZE]; //буффер для передачи сообщений по UART3
extern char RS485_RXbuffer [RX_BUFFER_SIZE];; //буффер для приёма сообщений по UART3

//******************************************************************************************************************************************************//
void GetTime (uint8_t RTC_adress, uint8_t registr_adress, uint8_t sizebuf, uint8_t * RTC_buffer)
{

	while(HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)RTC_adress, &registr_adress, 1, (uint32_t)0xFFFF)!= HAL_OK)
  {
		if (HAL_I2C_GetError(&hi2c1) != HAL_I2C_ERROR_AF)
		{
			sprintf (UART3_msg_TX , "RTC_write_error\r\n");
			UART3_SendString (UART3_msg_TX);
			return;
		}
	}
	
	while(HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY){} //ожидание готовности приёма		
		
	while (HAL_I2C_Master_Receive (&hi2c1, (uint16_t) RTC_adress, (uint8_t*)RTC_buffer, (uint16_t) sizebuf, (uint32_t)0xFFFF)!=HAL_OK) //запись показателей времени	
	{
		if(HAL_I2C_GetError(&hi2c1)!=HAL_I2C_ERROR_AF)
		{
			sprintf (UART3_msg_TX , "read_error\r\n");
			UART3_SendString (UART3_msg_TX);
			return;
		}
	}
	
	for (uint8_t count = 0; count < sizebuf; count++)
	{
		*RTC_buffer = RTC_ConvertToDec(*RTC_buffer); //перевод числа из двоично-десятичного представления  в обычное
		RTC_buffer++;
	}
}

//******************************************************************************************************************************************************//
void GetTemp (uint8_t  RTC_adress, uint8_t registr_adress,  uint8_t * temp_buffer)
{
	
	while(HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)RTC_adress, &registr_adress, 1, (uint32_t)0xFFFF)!= HAL_OK)
  {
		if (HAL_I2C_GetError(&hi2c1) != HAL_I2C_ERROR_AF)
		{
			sprintf (UART3_msg_TX , "RTC_write_error\r\n");
			UART3_SendString (UART3_msg_TX);
		}
	}
	
	while(HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY){} //ожидание готовности приёма
		
	while (HAL_I2C_Master_Receive (&hi2c1, (uint16_t) RTC_adress, temp_buffer, 1, (uint32_t)0xFFFF)!=HAL_OK)
	{
		if(HAL_I2C_GetError(&hi2c1)!=HAL_I2C_ERROR_AF)
		{
			sprintf (UART3_msg_TX , "read_error\r\n");
			UART3_SendString (UART3_msg_TX);
		}
	}
}

//******************************************************************************************************************************************************//
void SetTime (uint8_t RTC_adress, uint8_t registr_adress, char *time)
{
	//формирование сообщения для RTC	
	uint8_t I2C_RTC_buffer [4] = {0}; //обнулим массив
	uint8_t * ptr_RTC_buffer = I2C_RTC_buffer;
	signed char ptr = 1;
	for (signed char i = 0; i < 6; i++) 
	{
		if (!(i%2)) //при i=0, 2, 4
		{
			I2C_RTC_buffer [ptr] += 10 * (*(time+i) - 48); //десятый разряд дня, месяца, года
		}
		else
		{
			I2C_RTC_buffer [ptr] += (*(time+i)) - 48; //единичный разряд дня, месяца, года
			I2C_RTC_buffer [ptr] = RTC_ConvertToBinDec(I2C_RTC_buffer [ptr]); //перевод в двоично-десятичное представление
			ptr++;
		}
	}
	I2C_RTC_buffer [0] = registr_adress; //в первый элемент массива - адрес регистра RTC	
	
	while(HAL_I2C_Master_Transmit(&hi2c1, (uint16_t) RTC_adress, (uint8_t*)ptr_RTC_buffer, 4, (uint32_t)0xFFFF)!= HAL_OK) //отправка данных
  {
		if (HAL_I2C_GetError(&hi2c1) != HAL_I2C_ERROR_AF)
		{
			sprintf (UART3_msg_TX , "write_error\r\n");
			UART3_SendString (UART3_msg_TX);
		}
	}	
}

//********************************ф-я перевода числа из двоично-десятичного представления  в обычное********************************//
uint8_t RTC_ConvertToDec(uint8_t digit)
{
	uint8_t ch = ((digit>>4)*10 + (0x0F & digit));
	return ch;
}

//********************************ф-я перевода числа из обычного в двоично-десятичного представления********************************//
uint8_t RTC_ConvertToBinDec(uint8_t digit)
{
	uint8_t ch = ((digit/10) << 4) + (digit%10);
	return ch;
}


//******************************************************************************************************************************************************//
void read_reg_RTC (I2C_HandleTypeDef hi, uint8_t adress)
{
	uint8_t reg_adr = 0x2; //адрес регистра RTC 
	uint8_t sizebuf = 0x3; //количество байт для чтения
	uint8_t I2C_RTC_buffer [sizebuf];
	//передача адреса ds3231
	while(HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)adress, &reg_adr, 1, (uint32_t)0xFFFF)!= HAL_OK) //запись адреса RTC
  {
		if (HAL_I2C_GetError(&hi) != HAL_I2C_ERROR_AF)
		{
			sprintf (UART3_msg_TX , "write_error\r\n");
			UART3_SendString (UART3_msg_TX);
		}
	}
	while(HAL_I2C_GetState(&hi2c1)!=HAL_I2C_STATE_READY){}
		
	while (HAL_I2C_Master_Receive (&hi2c1, (uint16_t) adress, (uint8_t*)I2C_RTC_buffer, (uint16_t) sizebuf, (uint32_t)0xFFFF)!=HAL_OK)
	{
		if(HAL_I2C_GetError(&hi)!=HAL_I2C_ERROR_AF)
		{
			sprintf (UART3_msg_TX , "read_error\r\n");
			UART3_SendString (UART3_msg_TX);
		}
	}
	//перевод числа из двоично-десятичного представления  в обычное
	for (uint8_t count = 0; count < sizebuf; count++)
	{
		I2C_RTC_buffer[count] = RTC_ConvertToDec(I2C_RTC_buffer[count]);
		sprintf (UART3_msg_TX , "%x ", I2C_RTC_buffer[count]);
		UART3_SendString (UART3_msg_TX);
	}
	HAL_UART_Transmit (&huart3, (uint8_t*)"\r\n", strlen("\r\n"), 0xFFFF);
}

//******************************************************************************************************************************************************//
void edit_RTC_data (I2C_HandleTypeDef hi, uint8_t adress,  char * time)
{	
	char *ptr;	
	uint8_t errflag = 1; //флаг ошибки данных
	ptr = strtok(RS485_RXbuffer , ": "); //Ф-ия возвращает указатель на первую найденную лексему в строке. Если не найдено, то возвращается пустой указатель
	ptr = strtok(NULL, " \r\n"); // для последующего вызова можно передать NULL, тогда функция продолжит поиск в оригинальной строке
	strncpy (time, ptr, 6);  //копирование 6 символов цифр в массив для отправки
	for (uint8_t count = 0; count < 6; count++) //проверка чисел на корректность значений
	{
		if (count == 0)
		{
			if (!((time [count] > 47) && (time [count] < 51))) //если символ меньше 0 и больше 2
			{
				errflag = 0;
				break;
			}
		}
		else
		{
			if (count == 1)
			{
				if (time [0] == 50) //если в старшем разряде часов 2
				{
					if (!((time [count] > 47) && (time [count] < 52))) //если символ меньше 0 и больше 3
					{
						errflag = 0;
						break;
					}
				}
				else
				{
					if (!((time [count] > 47) && (time [count] < 58))) //если символ меньше 0 и больше 3
					{
						errflag = 0;
						break;
					}
				}
			}
			else
			{
				if ((count == 2) || (count == 4))
				{
					if (!((time [count] > 47) && (time [count] < 54))) //если символ меньше 0 и больше 5
					{
					errflag = 0;
					break;
					}	
				}	
				else
				{
					if (!((time [count] > 47) && (time [count] < 58)))  //если символ меньше 0 и больше 9
					{
						errflag = 0;
						break;
					}
				}
			}
		}
	}			
	if (errflag == 1)
	{
		/*sprintf (UART3_msg_TX , "data_ok\r\n");	
		UART3_SendString (UART3_msg_TX);
		SetTime (hi, adress, time); //отправка данных времени на мк RTC
		sprintf (wtext, "%u:%u:%u set time\r\n", RTC_ConvertToDec (I2C_RTC_buffer [3]),RTC_ConvertToDec (I2C_RTC_buffer [2]), RTC_ConvertToDec (I2C_RTC_buffer [1]));
		write_reg (&logfile , wtext);*/
	}
	else
	{
		sprintf (UART3_msg_TX , "%s- data_error\r\n", time);	
		UART3_SendString (UART3_msg_TX);
	}
}

//******************************************************************************************************************************************************//
void convert_time (unsigned char time_size, unsigned char * mod_time_data, unsigned char * time_data)
{
		
	for (size_t count = 0; count < time_size; count++)
	{
		*mod_time_data++ = (*(time_data + count) / 10) + 0x30;
		*mod_time_data++ = (*(time_data + count) % 10) + 0x30;
	}
}
//******************************************************************************************************************************************************//
