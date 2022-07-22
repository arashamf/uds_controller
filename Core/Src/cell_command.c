#include "cell_command.h"

extern osMessageQId RS485_msg_Queue;
extern char RS485_RXbuffer [RX_BUFFER_SIZE];
extern char RS485_TXbuffer [4];
extern char UART3_msg_TX [RS232_BUFFER_SIZE]; //буффер сообщений RS-232
extern osMutexId mutex_RS485_Handle;

//****************************************************************************************************************************************************//
void PutCommandToCell (char * buffer_command)
{
	taskENTER_CRITICAL(); //вход в критическую секцию
	RS485_TX; //режим на передачу		
	UART1_SendString (buffer_command); //передача сообщения
	HAL_UART_Receive_IT(&huart1, (uint8_t*)RS485_RXbuffer, 6); //ожидание получение сообщения (6 байт) от ячейки
	RS485_RX; //режим на приём
	taskEXIT_CRITICAL(); //выход из критической секции			
}

//****************************************************************************************************************************************************//
void command_AllCell (uint8_t typecommand, uint8_t NumberOfCell, uint8_t **ptr_cell, uint16_t delay)
//void command_AllCell (uint8_t typecommand, uint8_t NumberOfCell, uint16_t delay)
{
	osEvent event; 
//	uint8_t * ptr_RS485_msg;
	RS485_TXbuffer [0] = 0x2; //1 байт при передаче от контроллера к ячейки всегда равен числу 2
	
	for (size_t count = 1; count <= NumberOfCell; count++)
	{	
		if (osMutexWait (mutex_RS485_Handle, 50) == osOK) //ожидание и захват мьютекса
		{		
		RS485_TXbuffer [1] = (count/10 + 0x30); //старший символ номера ячейки	
		RS485_TXbuffer [2] = (count%10 + 0x30); //младший символ номера ячейки	
		RS485_TXbuffer [3] = typecommand + 0x30; //передача типа запроса, 1 - 'open','0 - 'close'
//		sprintf (UART3_msg_TX,"%c,%c-%c\r\n", RS485_TXbuffer [1], RS485_TXbuffer [2], RS485_TXbuffer [3]);
//		UART3_SendString (UART3_msg_TX);
																
			PutCommandToCell (RS485_TXbuffer);
		
			event = osMessageGet(RS485_msg_Queue, 2); //ожидание появления данных в очереди
			if (event.status == osEventMessage) //если данные с ответом от ячейки появились в очереди
			{	
				(void)event.value.v;
	//			ptr_RS485_msg = (uint8_t *)event.value.v;
	//			memcpy (*(ptr_cell+count), (ptr_RS485_msg+1), 2); //копирование 5 символов сообщения от ячейки, в массив с данными от ячеек  
			}	
			osMutexRelease (mutex_RS485_Handle);
		}
		osDelay (delay); //задержка между итерациями отправки команды 
	}		
}
//****************************************************************************************************************************************************//

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
