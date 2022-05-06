#include "myWeb.h"
#include "main.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "usart.h"
#include "gpio.h" 

extern char UART3_msg_TX [RS232_BUFFER_SIZE];

//******************************************************************************************************************************************************//
signed short Parse(char *inBuf, PARSE_DATA *output)
{
	char *tmp;	
	unsigned short string_size = 0;
	errortype count = 0;
	
	if (strncmp(inBuf, "GET ", 4) != 0) 
		{return NoGet;}								// We accept GET method only
	
	tmp=strchr(inBuf,' ');				// find the first ' ' from "GET "
	inBuf=tmp+1;									// cut off the "GET "
	
	tmp=strchr(inBuf,' ');				// find the end of the URI (the second ' ' in fact)
	if(tmp) 
		*tmp=0;											// set 0 for string termination
	
	tmp=strchr(inBuf,'/');				// find first '/' symbol
	if(tmp) 
		inBuf=tmp+1;								// Now we have "?command=cmd&param1=val1&param2=val2..."
		
	tmp=strchr(inBuf,'?');				// find '?'
	if(tmp) 
		inBuf=tmp+1;									// Now we have "command=cmd&param1=val1&param2=val2..."
	
	if(strncmp(inBuf,"command=",8)!=0) 
		{return UnknownCommand;}					// if there's no "command="
	
	tmp=strchr(inBuf,'=');				// find the first '='
	if(tmp) 
		inBuf=tmp+1;								// cut off "command=". Now we have "cell_cmd&param1=val1&param2=val2..."
	
	if(strncmp(inBuf,"port",4)!=0) 
		{return ErrorNumberCell;}				// параметр ячейки некорректен
	
		tmp=strchr(inBuf,'_');				// поск первого '_' (после port)
	if(tmp) 
		{*tmp=0;}											// вырезка номера ячейки в строке (port00..14) 
		else														//если не найден номер ячейки
		{return UnknownParam;}
	if (strlen (inBuf) > (LENGTH_NAME_CELL-1)) //проверка длины названия ячейки
		{return TooLongTextString;}
	
	strcpy(output -> name_cell,inBuf); 			//копирование номера ячейки
	inBuf = tmp + 1; 							//сдвиг указателя на следующий символ после символа '_'
	
	tmp=strchr(inBuf,'&');				// поиск первого символа '&'
	if(tmp==0) 
	{
		if (strlen (inBuf) > (LENGTH_COMMAND-1)) 
			{return TooLongTextString;}
		strcpy(output -> inCommand, inBuf); 	//если '&' не найден, то вся строка является командой без каких-либо параметров
	}
	else 
	{
		*tmp=0;												// завершение строки вместо символа "&"
		if (strlen (inBuf) > (LENGTH_COMMAND-1)) 
			{return TooLongTextString;}
		strcpy(output -> inCommand, inBuf);			// grab the command
		inBuf=tmp+1;									// указатель на первый символ первого параметра: "param1=val1&param2=val2..."
	}

	while((tmp=strchr(inBuf,'='))!=0) // поиск первого символа '='
	{	
		*tmp=0;											// обнуление символа '='
		
		if((string_size = strlen(inBuf)) > (LENGTH_PARAMETR - 1)) 
			{return TooLongTextString;}	
			
		strcpy(output->param[count], inBuf);				// копирование первого найденного параметра
		inBuf = tmp+1;							//указатель на первый символ после '='
		
		if((tmp=strchr(inBuf,'&'))!=0) //если символ '&' найден
		{
			*tmp=0;										// обнуление символа '&'
		
			if((string_size = strlen(inBuf)) > (LENGTH_VAL - 1)) 
				{return TooLongTextString;}	
		
			strcpy(output->val[count],inBuf);				// copy "val"
			inBuf = tmp+1;						//сдвиг указателя
		}
		else 
		{
			if((string_size = strlen(inBuf)) > (LENGTH_VAL - 1)) 
				{return TooLongTextString;}	
			
			strcpy(output->val[count],inBuf);	// no '&' symbol which may mean the end of the string		
		}
		count ++;
		if (count > 2) //не более трёх параметров
			break;
	}
	return count;
}	

//******************************************************************************************************************************************************//
errortype  make_command (errortype count, PARSE_DATA *output, RELEASE_DATA * getsetting)
{

	unsigned char cell_number = 0;

	//проверка номера ячейки
	if ((output -> name_cell [4] >=	0x30) &&	(output -> name_cell [4] <=	0x31) &&	(output -> name_cell [5] >=	0x30) &&	(output -> name_cell [5] <= 0x39))	
	{
		cell_number = ((output -> name_cell [4] - 0x30)*10 + (output -> name_cell [5] - 0x30));
		if (cell_number > 14)
			{return ErrorNumberCell;}
	}
	else 
		{return ErrorNumberCell;}	

	//проверка команды open
	if (strcmp (output -> inCommand, "open") == 0)  //открытие ячейки
	{	
		if (cell_number != 0)
		{
			output -> type_command = 100 + cell_number;
			{return Ok;}
		}
		else
			{return ErrorNumberCell;}
	}
	
	//проверка команды close
	if(strcmp(output -> inCommand,"close")==0) 		//закрытие ячейки
	{		
		output -> type_command = 300 + cell_number;
		return Ok;
	}
	
	//проверка команды state
	if(strcmp(output -> inCommand,"state")==0) 
	{		
		output -> type_command = 200 + cell_number;
		return Ok;	
	}
	
	if(strcmp(output -> inCommand,"reset")==0) {												// restore all default setup, including network and web interface
		return CommandNotSupported;	}	
	
	if(strcmp(output -> inCommand,"setup")==0) {											// activate web interface
		return CommandNotSupported;	}
	
	//проверка команды term
	if(strcmp(output -> inCommand,"term")==0) 
	{			
		output -> type_command = WhatIsTemperature;	
		return Ok;
	}
	
	if(strcmp(output -> inCommand,"read")==0) {											
		return CommandNotSupported;	}
	
	if(strcmp(output -> inCommand,"0read")==0) {											
		return CommandNotSupported;	}

	//проверка команды установки даты
	if(strcmp(output -> inCommand,"rtds")==0) 
	{						
		if (count < 3)
			return UnknownParam;
		else
		{
			if ((strncmp(output -> param [0], "d", 1)) || (strncmp(output -> param [1], "m", 1)) || (strncmp(output -> param [2], "y", 1)))
			{
				return ErrorParam;				
			}	
			else
			{
				if (!((output -> val[0][0] > 47) && (output -> val[0][0] < 52))) //старший разряд дня даты должен быть не менее 0 и не боллее 3 (0-3)
				{
					return ErrorParam;			
				}
				else
				{
					if (output -> val[0][0] == 51) //если старший разряд даты равен 3
					{
						if (!((47 < output -> val[0][1]) && (output -> val[0][1] < 50))) //младший разряд дня даты должен находиться в диапазоне от 0 до 1 (30-31)
						{
							return ErrorParam;
						}
					}
					else
					{
						if (!((47 < output -> val[0][1]) && (output -> val[0][1] < 58))) //младший разряд дня даты должен находиться в диапазоне от 0 до 9 (00-29)
						{
							return ErrorParam;
						}
					}
				}
				if (!((47 < output -> val[1][0]) && (output -> val[1][0] < 50))) //старший разряд месяца должен быть в диапазоне от 0 до 1 (0..1)
				{
					return ErrorParam;
				}
				else
				{
					if (output -> val[1][0] == 49) //если старший разряд месяца равен 1
					{	
						if (!((47 < output -> val[1][1]) && (output -> val[1][1] < 51))) //младший разряд месяца должен находиться в диапазоне от 0 до 2 (10..12)
						{
							return ErrorParam;
						}
					}
					else
					{
						if (!((47 < output -> val[1][1]) && (output -> val[1][1] < 58))) //младший разряд месяца должен находиться в диапазоне от 0 до 9 (00..09)
						{
							return ErrorParam;
						}
					}
				}
				if (!((47 < output -> val[2][0]) && (output -> val[2][0] < 58))) //старший разряд года должен находиться в диапазоне [0..9]
				{	
					return ErrorParam;
				}
				if (!((47 < output -> val[2][1]) && (output -> val[2][1] < 58))) //младший разряд года должен находиться в диапазоне [00..99]
				{	
					return ErrorParam;
				}
				else
				{
					uint8_t ptr = 0;
					output -> type_command = SetNewDate; //идентификатор даты
					for (uint8_t i = 0; i < count; i++)
					{
						for (uint8_t j = 0; j < 2; j++)
						{
							getsetting->RTC_setting [ptr] = *(output->val[i]+j); 
							ptr++;
						}	
					}
					return Ok;
				}
			}
		}
	}
	
	//проверка команды установки времени
	if(strcmp(output -> inCommand,"rtss")==0) 
	{						
		if (count < 3)
			return UnknownParam;
		else
		{
			if ((strncmp(output -> param [0], "h", 1)) || (strncmp(output -> param [1], "m", 1)) || (strncmp(output -> param [2], "s", 1)))
			{
				return ErrorParam;				
			}	
			else
			{
				if (!((output -> val[0][0] > 47) && (output -> val[0][0] < 51))) //старший разряд часов должен быть не менее 0 и не более 2 (0..2)
				{
					return ErrorParam;			
				}
				else
				{
					if (output -> val[0][0] == 50) //если старший разряд часов равен 2
					{
						if (!((output -> val[0][1] > 47) && (output -> val[0][1] < 52))) //младший разряд часов должен находиться в диапазоне от 0 до 3 (20..23)
						{
							return ErrorParam;
						}
					}	
					else
					{
						if (!((output -> val[0][1] > 47) && (output -> val[0][1] < 58))) //младший разряд часов должен находиться в диапазоне от 0 до 9 (00..19)
						{
							return ErrorParam;
						}
					}
				}
				if (!((47 < output -> val[1][0]) && (output -> val[1][0] < 54))) //старший  разряд минут должен быть в диапазоне от 0 до 5
				{
					return ErrorParam;
				}
				if (!((47 < output -> val[1][1]) && (output -> val[1][1] < 58))) //младший  разряд минут должен быть в диапазоне от 0 до 9
				{	
						return ErrorParam;
				}
				if (!((47 < output -> val[2][0]) && (output -> val[2][0] < 54))) //старший  разряд секунд должен быть в диапазоне от 0 до 9
				{	
						return ErrorParam;
				}
				if (!((47 < output -> val[2][1]) && (output -> val[2][1] < 58))) //младший  разряд секунд должен быть в диапазоне от 0 до 9
				{	
						return ErrorParam;
				}
				else
				{
					output -> type_command = SetNewTime; //идентификатор установки времени
					uint8_t ptr = 0;
					for (uint8_t i = 0; i < count; i++)
					{
						for (uint8_t j = 0; j < 2; j++)
						{
							getsetting->RTC_setting [ptr] = *(output->val[count-1-i]+j); //секунды и часы меняются местами в очерёдности следования
							ptr++;
						}	
					}
					return Ok;
				}
			}
		}
	}
	
	//проверка команды получения времени и даты
	if(strcmp(output -> inCommand,"rtcr")==0)  	
	{    
		output -> type_command =  WhatTimeIsIt;
		return Ok;
	}
	
	if(strcmp(output -> inCommand,"clear")==0) {											
		return CommandNotSupported;	}
	
		//проверка команды установки ip адреса
	if(strcmp(output -> inCommand,"iset")==0) 
	{		
		if (strncmp(output -> param [0], "ip", 2))
			{return ErrorParam;}	
		else
		{
			if (strlen (output->val[0]) < 15)
				{return ErrorParam;}
			else
			{
				for (uint8_t count = 0; count < 15; count++)
				{
					if ((count == 3) || (count == 7) || (count == 11)) //поиск трёх точек
					{
						if ((*(output->val[0] + count)) != '.')
							{return ErrorParam;}
						else	
							{continue;}
					}
					if ((0x39 < (*(output->val[0] + count))) || (0x30 > (*(output->val[0] + count))))
					{
						{return ErrorParam;}
					}
				}	
				uint16_t buffer_ip = 0; uint8_t ptr_ip = 0;
				for (uint8_t count = 0; count < 15; count+=4)
				{
					buffer_ip = ((100* (output->val[0][count] - 0x30)) + (10* (output->val[0][count+1] - 0x30)) + (output->val[0][count+2] - 0x30));					
					if (buffer_ip < 255)
						{getsetting->new_ipadress [ptr_ip++] = (uint8_t)buffer_ip;}
					else
						{return ErrorParam;}
				}
				output -> type_command = NewIpSet;
				return Ok;
			}
		}
	}
		
	if(strcmp(output -> inCommand,"st22")==0) 
		{return CommandNotSupported;}
	
		return NoCommand;							// если ничего не подошло
}

//******************************************************************************************************************************************************//
void Read_TCP_Message (char *msg_input, 	RELEASE_DATA * getsetting) 
{	
	errortype parseRes = -1;
	
	PARSE_DATA parse_buffer;
	PARSE_DATA *ptr_buffer = &parse_buffer;
	
	if ((parseRes = Parse(msg_input, ptr_buffer)) >= 0)	// если первичный парсинг прошёл успешно (т.е.общая структура команды соответсвует шаблону)
	{
		parseRes = make_command (parseRes, ptr_buffer, getsetting); //в этой ф-ии проверяется корректность полученной команды и её параметров, и заполняется специальная структура данных
	}
	
	strcpy (getsetting->answerbuf, "HTTP/1.1 200 OK\nContent-type: text/plain\n\nAnswer=");      
	if(parseRes < 0)	//если в предыдущих 2 функциях парсинга была получена ощибка, выводим тип этой ошибки
	{
		if(parseRes == NoGet) {	
			strcat(getsetting->answerbuf,"ERROR&no_get");	
		}
		if(parseRes == NoCommand) {	
			strcat(getsetting->answerbuf,"ERROR&illegal_command");	
		}
		if(parseRes == UnknownParam) 
		{	
			strcat(getsetting->answerbuf,"ERROR&unknown_param");	
		}
		if(parseRes == UnknownCommand) 
		{	
			strcat(getsetting->answerbuf,"ERROR&unknown_command"); //не использую
		}
		if(parseRes == CommandNotSupported) 
		{	
			strcat(getsetting->answerbuf,"ERROR&command_not_supported");	//не использую
		}
		if(parseRes == TooManyTexts) 
		{	
			strcat (getsetting->answerbuf,"ERROR&too_many_texts");	//не использую
		}
		if(parseRes == TooLongTextString) 
		{	
			strcat (getsetting->answerbuf,"ERROR&too_long_text_string");	
		}
		if(parseRes == ErrorParam) 
		{	
			strcat(getsetting->answerbuf,"ERROR&illegal_param");	
		}
		if(parseRes == ErrorNumberCell) 
		{	
			strcat(getsetting->answerbuf,"ERROR&illegal_number_cell");	
		}
		getsetting->type_data = parseRes;
	}
	else
	{
		if(parseRes == Ok )
		{	
			strcat(getsetting->answerbuf,"OK"); //объединение строк. K output добавляется Ок
			getsetting -> type_data = ptr_buffer -> type_command; //шифр команды
		}		
	}
//	sprintf(UART3_msg_TX, "%s\r\n", getsetting->answerbuf);
//	UART3_SendString ((char*)UART3_msg_TX);	
}
//***********************************************ф-я конвертации ip-адресса из числовой формы в символьную***********************************************//

void convert_ip (char * mod_ip_adress, unsigned char * ip4_adress)
{
		
	char * buffer_ptr = mod_ip_adress;
	for (uint8_t count = 0; count < 4; count++)
	{
		*buffer_ptr++ = (*(ip4_adress + count) / 100) + 0x30;
		*buffer_ptr++ = (*(ip4_adress + count) % 100 / 10) + 0x30;
		*buffer_ptr++ = (*(ip4_adress + count) % 10 / 1) + 0x30;
		*buffer_ptr++ = '.';		
	}
	
	*(mod_ip_adress + 15) = '\0';
}

//******************************************************************************************************************************************************//
