/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "lwip.h"

#include "iwdg.h"
#include "tim.h"
#include "stdio.h"
#include "string.h"
#include "usart.h"
#include "gpio.h"
#include "flash_memory.h"
#include "myWeb.h"
#include "st7735.h"
#include "rtc.h"
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
/* USER CODE BEGIN Variables */
extern char UART3_msg_TX [RS232_BUFFER_SIZE];
extern char RS485_TXbuffer [4];
extern uint8_t cell_state [MAX_SELL+1][5];

extern uint8_t IP_ADDRESS[4]; //установленный ip-адрес в виде четырёх uint8_t (lwip.c)  
extern char mod_ip_adress [16]; //ip-адрес в символьной форме (например 192.168.001.060) для регистрации и отображения

const char httpHeader[] = "HTTP/1.1 200 OK\nContent-type: text/plain\n\n" ;  // HTTP-заголовок

uint8_t flag_masterkey = 0; //флаг срабатывания мастер ключа
const size_t time_size = 6; //размер буфера для сохранения показателей времени/даты
	
typedef struct 
{
  uint8_t temperature;
  uint8_t RTC_data [time_size];
	} get_RTC_data ; //структура с данными, полученными от микросхемы RTC

char http_send_buffer [400]; //буффер, в который записывается сформированный http-ответ

osTimerId osProgTimerIWDG;  //программный таймер перезагружающий сторожевик
osTimerId osProgTimerBeeper;  //программный таймер отключающий бипер
osTimerId osProgTimerMasterKey;  //программный таймер выключающий соленоиды

osMutexId mutex_RS485_Handle; //мьютекс блокировки передачи команд ячейкам

osMessageQId HTTP_msg_Queue; //очередь в которую передаётся указатель на полученный HTTP-запрос
osMessageQId HTTP_answer_Queue; //очередь в которую передаётся указатель на сформированный HTTP-ответ
osMessageQId Cell_msg_Queue; //очередь в которую передаётся код команды ячейкам
osMessageQId Master_Key_Queue; //очередь в которую передаётся команды управления ячейками по результатам обработки состояния мастер-ключа
osMessageQId Ip_adress_Queue;
osMessageQId RTC_typedata_Queue;
osMessageQId RS485_msg_Queue;
	
osThreadId Task_Switch_Led_Handle;
osThreadId Task_Parse_HTTP_msg_Handle;
osThreadId Task_Parsing_Cell_command_Handle;
osThreadId Task_Ping_All_Sell_Handle;
osThreadId Task_Read_Keys_MasterCell_Handle;
osThreadId Task_MasterCell_Switcher_Handle;
osThreadId Task_RTC_get_time_Handle;
osThreadId Task_Set_New_IP_Handle;
osThreadId Task_Show_LCD_Handle;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void ProgTimerIWDGCallback(void const *argument);
void ProgTimerBeeperCallback(void const *argument);
void ProgTimerMasterKeyCallback (void const *argument);

void Switch_Led (void const * argument);
void Parse_HTTP_msg (void const * argument);
void Parsing_Cell_command (void const * argument);
void Ping_All_Sell (void const * argument);
void Read_Keys_MasterCell (void const * argument);
void MasterCell_Switcher (void const * argument);
void RTC_get_time (void const * argument);
void Set_New_IP (void const * argument);
void Show_LCD (void const * argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
	osMutexDef (mutex_RS485); 
	mutex_RS485_Handle = osMutexCreate(osMutex (mutex_RS485));
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */

  osTimerDef (TimerIWDG, ProgTimerIWDGCallback);
	osProgTimerIWDG = osTimerCreate(osTimer (TimerIWDG), osTimerPeriodic, NULL);
	
	osTimerDef (TimerBeeper, ProgTimerBeeperCallback);
	osProgTimerBeeper = osTimerCreate(osTimer (TimerBeeper), osTimerOnce, NULL);
	
	osTimerDef (TimerMasterKey, ProgTimerMasterKeyCallback);
	osProgTimerMasterKey = osTimerCreate(osTimer (TimerMasterKey), osTimerOnce, NULL);
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  osMessageQDef (HTTP_msg_Queuename, 3, uint8_t *);
	HTTP_msg_Queue = osMessageCreate (osMessageQ (HTTP_msg_Queuename), NULL); ///очередь для передачи указателя на полученное http-сообщение
	
	osMessageQDef (HTTP_answer_Queuename, 4, uint8_t *); 
	HTTP_answer_Queue = osMessageCreate (osMessageQ (HTTP_answer_Queuename), NULL); ///очередь для передачи указателя на сформированное http-сообщение
 
	osMessageQDef (Cell_msg_Queuename, 2, uint16_t); 
	Cell_msg_Queue = osMessageCreate (osMessageQ (Cell_msg_Queuename), NULL); //очередь для кода команды управления ячейками
	
	osMessageQDef (Master_Key_Queuename, 1, uint8_t); 
	Master_Key_Queue = osMessageCreate (osMessageQ (Master_Key_Queuename), NULL); //очередь для команд управления ячейками от мастерключа

	osMessageQDef (Ip_adress_Queuename, 2, uint8_t *);
	Ip_adress_Queue = osMessageCreate (osMessageQ (Ip_adress_Queuename), NULL); //очередь для передачи полученного ip-адреса

	osMessageQDef(RTC_typedata_Queuename, 2, uint8_t);
	RTC_typedata_Queue = osMessageCreate (osMessageQ (RTC_typedata_Queuename), NULL); //очередь для передачи типа запрашиваемых RTC данных
	
	osMessageQDef (RS485_msg_Queuename, 2, uint8_t *);
	RS485_msg_Queue = osMessageCreate (osMessageQ (RS485_msg_Queuename), NULL); //очередь для передачи полученного по RS-485 сообщения
	
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	osThreadDef (Task_Parse_HTTP_msg, Parse_HTTP_msg, osPriorityNormal, 0, 1024); 
	Task_Parse_HTTP_msg_Handle = osThreadCreate(osThread(Task_Parse_HTTP_msg), NULL); 
	
	osThreadDef (Task_Switch_Led, Switch_Led, osPriorityLow, 0, 128); 
	Task_Switch_Led_Handle = osThreadCreate(osThread(Task_Switch_Led), NULL); 
	
  osThreadDef (Task_Parsing_Cell_command, Parsing_Cell_command, osPriorityNormal, 0, 512); 
	Task_Parsing_Cell_command_Handle = osThreadCreate(osThread(Task_Parsing_Cell_command), NULL); 
	
	osThreadDef (Task_Ping_All_Sell, Ping_All_Sell, osPriorityNormal, 0, 384); 
	Task_Ping_All_Sell_Handle = osThreadCreate (osThread (Task_Ping_All_Sell), NULL);
	
	osThreadDef (Task_Read_Keys_MasterCell, Read_Keys_MasterCell, osPriorityBelowNormal, 0, 128); 
	Task_Read_Keys_MasterCell_Handle = osThreadCreate (osThread (Task_Read_Keys_MasterCell), NULL);
	
	osThreadDef (Task_MasterCell_Switcher, MasterCell_Switcher, osPriorityNormal, 0, 128); 
	Task_MasterCell_Switcher_Handle = osThreadCreate (osThread (Task_MasterCell_Switcher), NULL);	
	
	osThreadDef (Task_RTC_get_time, RTC_get_time, osPriorityNormal, 0, 128); 
	Task_RTC_get_time_Handle = osThreadCreate (osThread (Task_RTC_get_time), NULL);
	
	osThreadDef (Task_Set_New_IP, Set_New_IP, osPriorityNormal, 0, 128); 
	Task_Set_New_IP_Handle = osThreadCreate (osThread (Task_Set_New_IP), NULL);
	
	osThreadDef (Task_Show_LCD, Show_LCD, osPriorityLow, 0, 128); 
	Task_Show_LCD_Handle = osThreadCreate (osThread (Task_Show_LCD), NULL);
	
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
//************************************************открытие и прослушивание tcp-соединения************************************************//
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
	convert_ip (mod_ip_adress, IP_ADDRESS);
	
	TIM4->CCR4 = 50; //ширина импульса шим = 1/2Т
	osTimerStart(osProgTimerIWDG, 2000); //запуск циклического таймера
	
 	struct netconn *conn; //указатель на переменную структуры соединения
	struct netconn *newconn;  //структура соединения для входящего подключения
	struct netbuf *inbuffer; //структура для приёмного буффера
	
	char http_put_buffer [450]; //буффер, в который формируется http-ответ
	char * http_answer; //указатель на буффер сформированного http-сообщения
	char* ptr_http_msg; //указатель на полученное http-сообщение
	
	volatile err_t err, rcv_err; //переменная ошибки
	uint16_t len; //количество символов в сообщении
	
	osEvent event;
		
	if ((conn = netconn_new(NETCONN_TCP)) != NULL) //новое TCP соединение, если нет ошибок установки соединения
	{ 
		if ((err = netconn_bind(conn, NULL, 25006)) == ERR_OK) //привязка *conn к определенному IP-адресу и порту 
    {
      netconn_listen(conn); //прослушка порта в бесконечном цикле		
			while (1)
			{
				if ((err = netconn_accept(conn, &newconn)) == ERR_OK) //разрешение на приём нового tcp-соединения в сети прослушивания
				{	
					if ((rcv_err = netconn_recv(newconn, &inbuffer)) == ERR_OK) //если tcp-сообщение принято без ошибок
					{
//					osTimerStart(osProgTimerBeeper, 250); 
//					HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); //писк бипером
						netbuf_data(inbuffer, (void**)&ptr_http_msg, &len); //копирование полученного http-сообщения
						osMessagePut (HTTP_msg_Queue, (uint32_t)ptr_http_msg, 10); //передача в очередь указателя на полученное http-сообщение
						event = osMessageGet(HTTP_answer_Queue, 100); //ожидание появления данных в очереди
						if (event.status == osEventMessage) //если данные для ответного http-сообщения появились в очереди
						{
							http_answer = (char *)event.value.v; //копирование данных для ответного http-сообщения
							sprintf (http_put_buffer, "%s", httpHeader); //добавление заголовка в ответное http-сообщение
							strcat (http_put_buffer,  http_answer); //добавление тела http-сообщения 
							netconn_write(newconn, http_put_buffer, strlen(http_put_buffer), NETCONN_NOFLAG); //отправка ответного http-сообщения клиенту
						} 					
					}
					netbuf_delete(inbuffer); //очистка входного буффера		
					netconn_close(newconn); //закрытие соединения
					netconn_delete(newconn); //очистка соединения											
				}
				osDelay(10);  
			}                                     
		}
		else
		{
			UART3_SendString((char*)"can_not_bind_TCP_netconn\r\n"); //если произошла ошибка задания соединению к ip-адресу и порте
			UART3_SendString ((char*)UART3_msg_TX);
			netconn_delete(newconn); //то удаление структуры
		}
	}
	else
	{
		UART3_SendString((char*)"can not create TCP netconn\r\n");
		UART3_SendString ((char*)UART3_msg_TX);
	}	
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
//************************************************мигание светодиодом************************************************//
void Switch_Led (void const * argument)
{
	for(;;)
  {
		LED_RED (1);
		osDelay (1000);
		LED_RED (0);
		osDelay (1000);
	}
}

//************************************************парсинг tcp-сообщения и передача полученных данных другим задачам************************************************//
void Parse_HTTP_msg (void const * argument)
{
	char * http_get_data; //полученный код команды из очереди
	osEvent event;
	RELEASE_DATA parse_output;
	RELEASE_DATA *ptr_data = &parse_output;
	signed short buf = 0; //буфер для хранения кода команды
	char * tmp;
	
  for(;;)
  {
		event = osMessageGet(HTTP_msg_Queue, 50); //ожидание появления данных в очереди
		if (event.status == osEventMessage) //если данные появились в очереди
		{
			http_get_data = (char *)event.value.v;
			parse_output.RTC_setting[0] = 0; //первый элемент массива адрес регистра RTC
			Read_TCP_Message (http_get_data, ptr_data); //получение структуры от корректированных данных	
			buf = ptr_data->type_data; //считывание идентификатора сообщения
			if (buf > 0)
			{
				if ((buf > 100) && (buf < 400)) //больше 100 и меньше 400 это команды в ячейку
				{
					osMessagePut(Cell_msg_Queue, buf, 10); //передача команды управления ячейкой таску Parsing_Cell_command
				}
				else
				{
					if ((buf > 0) && (buf < 100))
					{
						switch (buf)
						{
							case 1: //если запрос времени и даты								
								osMessagePut (RTC_typedata_Queue, (uint32_t)buf, 10); //передача команды запроса времени/температуры задаче RTC_get_time
								break;
							
							case 2: //если запрос температуры
								osMessagePut (RTC_typedata_Queue, (uint32_t)buf, 10); //передача команды запроса времени/температуры задаче RTC_get_time
								break;
							
							case 3: //если установка времени
								SetTime (RTC_ADDRESS,  0x0, ptr_data->RTC_setting);		
								buf = 1;
								osMessagePut (RTC_typedata_Queue, (uint32_t)buf, 10); //передача команды запроса времени/температуры задаче RTC_get_time
								break;		
							
							case 4: //если установка даты
								SetTime (RTC_ADDRESS,  0x4, ptr_data->RTC_setting);	
								buf = 1;
								osMessagePut (RTC_typedata_Queue, (uint32_t)buf, 10); //передача команды запроса времени/температуры задаче RTC_get_time
								break;	
							
							case 5: //если установка ip-адреса 
								osMessagePut(Ip_adress_Queue, (uint32_t)ptr_data->new_ipadress, 10); //передача полученного для установке ip-адреса задаче Set_New_IP
								break;
								
							default:
								break;
						}
					}
				}
			}	
			else //подготовка ответа при некорректном запросе
			{
				tmp=strchr(ptr_data->answerbuf,'&');				// поиск первого символа '&'
				if(tmp != 0)
				{
					tmp = tmp+1; //сдвиг указателя на один символ влево от '&'
					sprintf (http_send_buffer, "stoika=%s_result&%s\r\n", mod_ip_adress, tmp); //формирования сообщения
					osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //указатель на ответное http-сообщение в очередь для задачи StartDefaultTask
				}
				else
				{
					osMessagePut (HTTP_answer_Queue, (uint32_t)ptr_data->answerbuf, 10); //на всякий случай
				}	
			}
		}		
		osThreadYield (); //Передача управление следующему потоку с наивысшем приоритетом, который находится в состоянии ГОТОВ
	}
}

//************************************************формирование команды для ячеек по результатам парсинга tcp-сообщения************************************************//
void Parsing_Cell_command (void const * argument)
{
	osEvent event1, event2;
	uint16_t number_cell = 0; //номер ячейки в числовой форме
	uint8_t adress_cell [] = {'0', '0', '0'}; //номер ячейки  в символьной форме
	
	uint16_t command = 0; //шифр полученной команды (тип команды + номер ячейки)
	uint8_t typecommand = 0; //тип отправляемой команды
	
	char tmp_buffer [15] = {0}; //времменный буфер-массив
	uint8_t * ptr_RS485_buffer; //указатель на принятое сообщение по RS-485 (UART1_msg_RX [6])
	
	RS485_TXbuffer [0] = 0x2; //1 байт при передаче от контроллера к ячейки всегда равен числу 2
	
  for(;;)
  {
		event1 = osMessageGet(Cell_msg_Queue, 50); //ожидание появления данных в очереди
		if (event1.status == osEventMessage) //если данные появились в очереди
		{
			command = event1.value.v; //получение сообщения
			http_send_buffer [0] = '\0';
			if (command == 200) //если тип команды - широковещательный state
			{	
				sprintf (http_send_buffer,"stoika=%s&state=ok\r\n", mod_ip_adress);	
				for (size_t count = 0; count <= MAX_SELL; count++)
				{	
					ptr_RS485_buffer = &cell_state [count][0];	//указатель на массив с данными состояния ячеек		
					if (count != MAX_SELL) //если это ячейки с 1 по MAX_SELL
					{
						adress_cell[1] = ((count+1)/10 + 0x30); //установка номера ячейки
						adress_cell[2] = ((count+1)%10 + 0x30);	
						if (cell_state [count][0] != 0) //если от ячейки получены данные состояния
						{
							sprintf  (tmp_buffer, "cell_%c%c=%c,%c,%c,%c,%c\r\n",  adress_cell[1], adress_cell[2], *ptr_RS485_buffer, *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+2), *(ptr_RS485_buffer+3), *(ptr_RS485_buffer+4));	
							strcat (http_send_buffer, tmp_buffer);
						}
						else	//если данные не получены от ячейки
						{ 
							sprintf (tmp_buffer, "cell_%c%c=no\r\n", adress_cell[1], adress_cell[2]);
							strcat (http_send_buffer, tmp_buffer);
						}
					}
					else //если это мастер-ячейка
					{
						adress_cell[0] = '1';
						adress_cell[1] = '4'; //присвоим мастер-ячейке 14 номер			
						sprintf (tmp_buffer, "cell_%c%c=%c,%c,%c,%c,%c\r\n", adress_cell[0], adress_cell[1], *(ptr_RS485_buffer), *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+2), *(ptr_RS485_buffer+3),  *(ptr_RS485_buffer+4));
						strcat (http_send_buffer, tmp_buffer);
					}
				}
				osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
			}			
			else
			{
				RS485_TXbuffer [3] = command/100 + 0x30; //шифр комманды
				if (RS485_TXbuffer [3] == '3') //если зашифрована команда close
					RS485_TXbuffer [3] = '0'; //передадим символ 0
				typecommand = RS485_TXbuffer [3]-0x30; //запоминание типа полученной команды
				number_cell =  command % 100; //номер ячейки
				
				adress_cell [1] = RS485_TXbuffer [1] = (number_cell/10 + 0x30); //старший символ номера ячейки
				adress_cell [2] = RS485_TXbuffer [2] = (number_cell%10 + 0x30); //младший символ номера ячейки
				
				PutCommandToCell (RS485_TXbuffer); //отправка команды ячейке по RS-485	
			
				switch (typecommand)
				{
					case 0: //если запрос типа close ячейки
						sprintf (http_send_buffer,"stoika=%s&close_port=%s&result=accepted", mod_ip_adress, adress_cell);
						event2 = osMessageGet(RS485_msg_Queue, 4); //ожидание появления данных в очереди
					if (event2.status == osEventMessage) //если данные появились в очереди (ответ на команду close, сейчас я их не использую)
						{	
						 (void)event2.value.v;	
						}
						osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на сформированный ответ
						break;
				
					case 1: //если запрос типа open ячейки
						sprintf (http_send_buffer,"stoika=%s&open_port=%s&result=accepted", mod_ip_adress, adress_cell);
						event2 = osMessageGet(RS485_msg_Queue, 4); //ожидание появления данных в очереди
						if (event2.status == osEventMessage) //если данные появились в очереди (ответ на команду open, сейчас я их не использую)
						{	
							(void)event2.value.v;	
						}
						osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на сформированный ответ
						break;
				
					case 2: //если запрос типа state ячейки
						sprintf (http_send_buffer,"stoika=%s&state=ok", mod_ip_adress);
						ptr_RS485_buffer = &cell_state [number_cell-1][0];	//указатель на массив с данными состояния ячеек		
						if (*ptr_RS485_buffer != 0) //если есть данные от ячейки
						{	
							sprintf  (tmp_buffer, "&cell_%s=%c,%c,%c,%c,%c", adress_cell, *ptr_RS485_buffer, *(ptr_RS485_buffer + 1), *(ptr_RS485_buffer + 2), *(ptr_RS485_buffer + 3), *(ptr_RS485_buffer + 4));	
							strcat (http_send_buffer, tmp_buffer);
							osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
						}
						else	 //если не получен ответ от ячейки
						{ 
							sprintf (tmp_buffer, "&cell_%s=no", adress_cell);
							strcat (http_send_buffer, tmp_buffer);
							osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
						}
						break;
					
					default:
						break;	
				}	
			}
		}
		osThreadYield (); 
	}
}

//************************************************периодическая отправка всем ячейкам запроса типа state************************************************//
void Ping_All_Sell (void const * argument)
{
	osEvent event; 
	uint8_t * ptr_RS485_msg; //указатель на принятое сообщение по RS-485 (UART1_msg_RX)
	
	for (;;)
	{
		RS485_TXbuffer [3] = '2'; //передача запроса типа 'state'			
		for (size_t count = 0; count < MAX_SELL; count++) //опрос ячеек
		{
			RS485_TXbuffer [1] = ((count+1)/10 + 0x30); //старший символ номера ячейки	
			RS485_TXbuffer [2] = ((count+1)%10 + 0x30); //младший символ номера ячейки
			
			if (osMutexWait (mutex_RS485_Handle, 20) == osOK) //ожидание и захват мьютекса в течение xx мс
			{	
				PutCommandToCell (RS485_TXbuffer);
			
				event = osMessageGet(RS485_msg_Queue, 2); //ожидание появления данных в очереди
				if (event.status == osEventMessage) //если данные появились в очереди
				{	
					ptr_RS485_msg = (uint8_t *)event.value.v;
					memcpy ((cell_state + count), (ptr_RS485_msg+1), 5); //копирование 5 символов сообщения от ячейки, начиная с 2 элемента
				}
				else //если не получен ответ от ячейки
				{
					cell_state [count][0] = 0;
				}
				osMutexRelease (mutex_RS485_Handle);
			}
		}
		osDelay (20);
	}
}

/************************************************чтение микриков 14 ячейки************************************************/
void Read_Keys_MasterCell (void const * argument)
{
	cell_state [MAX_SELL][4] = cell_state [MAX_SELL][0] = '0'; //обнуление 0 и 4 бита мастер-ячейки 
	uint16_t status_inputs = 0;
	uint16_t old_status_inputs = 0;
	uint8_t MasterKeyStatus = (((READ_BIT(GPIOE->IDR, MASTER_KEY_Pin)) >> 4) + 0x30);
	for (;;) 
	{
		if ((status_inputs = READ_BIT(GPIOE->IDR,  SIDE_COVER_Pin | MASTER_KEY_Pin | TOP_COVER_Pin)) != old_status_inputs) //если состояние мастер ключей было изменено
		{			
			old_status_inputs = status_inputs; //сохраняем состояние
			osDelay (50); //ожидание окончание дребезга
			if ((status_inputs = READ_BIT(GPIOE->IDR,  SIDE_COVER_Pin | MASTER_KEY_Pin | TOP_COVER_Pin)) == old_status_inputs) //если состояние мастер ключей не измененялось за 50 мс
			{
				for (size_t count = 3; count <= 5; count++)
				{
					if (((status_inputs >> count) & 0x1) == 0x0) //если считанный бит равен 0, (контакт замкнут)
						cell_state [MAX_SELL][count - 2] = '1'; //сохраним 1 в буффер состояния ключей
					else
						cell_state [MAX_SELL][count - 2] = '0';
					if (count == 5)  //если это бит состояния мастер ключа
					{
						if (MasterKeyStatus != cell_state [MAX_SELL][count - 2]) //если состояние мк изменилось
						{
							if (cell_state [MAX_SELL][count - 2] ==  '1') //если мастер-ключ был переведён в положение включено
							{
								if (flag_masterkey == 0) //если мастер-ключ не был до этого момента нажат
								{	
									flag_masterkey = 1; //поднятие флага о включение мк
									LED_GREEN (1);
									osMessagePut (Master_Key_Queue, 1, 10); //передача в очередь команды на включение соленоидов (1)
								}
							}
							else 
							{
								if (cell_state [MAX_SELL][count - 2] == '0') //если мастер-ключ был переведён в положение выключено 
								{
									if (flag_masterkey == 1) //если мастер-ключ был до этого момента нажат (flag_masterkey == 1)
									{
										osMessagePut (Master_Key_Queue, 0, 10); //передача в очередь команды на выключение соленоидов (0)
									}
								}
							}
							MasterKeyStatus = cell_state [MAX_SELL][count - 2]; //сохраним новое состояние мк
						}
					}
				}
			}
			else
			{
				osDelay (50);
				continue;
			}
		}
		else
		{			
			osDelay (250);
		}
	}
}

/************************************************отправка ячейкам команды типа open************************************************/
void MasterCell_Switcher (void const * argument)
{
	osEvent event; 
	uint8_t typecommand;
	for (;;)
	{
		event = osMessageGet(Master_Key_Queue, 70); //ожидание появления данных в очереди
		if (event.status == osEventMessage) //если данные появились в очереди
		{	
			typecommand = (uint8_t)event.value.v;
			command_AllCell (typecommand, MAX_SELL); //передача команды всем соленоидам
			if (typecommand == 0) //если была отправлена команда на выключение соленоидов
			{
				osTimerStop (osProgTimerMasterKey); //остановка таймера на выключение соленоидов
				flag_masterkey = 0;
				LED_GREEN (0);
			}
			else //если была отправлена команда на включение соленоидов
			{
				osTimerStart(osProgTimerMasterKey, 30000); //запуск таймера на выключение соленоидов
			}
		}
		osThreadYield (); 
	}
}



/************************************************запрос времени/температуры************************************************/
void RTC_get_time (void const * argument)
{ 
	osEvent event;
	uint8_t reg_adr; //адрес начального регистра с данными времени
	uint8_t type_RTC_data = 0; 
	get_RTC_data Time_Data; //инициализация структуры с данными времени и температуры
	get_RTC_data * ptr_RTC_data = &Time_Data; //инициализация указателя на массив с данными времени в числовом виде
	uint8_t time_array [time_size*2] = {0}; //массив с данными времени в символьном виде
	
	for (;;) 
	{
		event = osMessageGet(RTC_typedata_Queue, 50); //ожидание появления данных в очереди
		if (event.status == osEventMessage) //если данные появились в очереди
		{
			type_RTC_data = (uint8_t)event.value.v;
			reg_adr = 0x0; //адрес начального регистра с данными времени
			GetTime (RTC_ADDRESS, reg_adr, 3, ptr_RTC_data -> RTC_data); //чтение регистров 0х0-0х2
			reg_adr = 0x4;
			GetTime (RTC_ADDRESS, reg_adr, 3, (ptr_RTC_data -> RTC_data)+3); //чтение регистров 0х4-0х6 (дата: дд/мм/гг)
			reg_adr = 0x11;
			GetTime (RTC_ADDRESS, reg_adr, 1, &ptr_RTC_data -> temperature); //чтение регистра 0х11 (температура)
				
			if (type_RTC_data == 1) //если запрос времени и даты
			{
				convert_time (time_size, time_array, ptr_RTC_data -> RTC_data);
				sprintf (http_send_buffer, "stoika=%s&rtcr=_%c%c/%c%c/%c%c_%c%c:%c%c:%c%c\r\n", mod_ip_adress, time_array [6], time_array [7], time_array [8], time_array [9], 
				time_array [10], time_array [11], time_array [4], time_array [5], time_array [2], time_array [3], time_array [0], time_array [1]);
				UART3_SendString ((char*)http_send_buffer);	
				osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
			}
			else
			{
				if (type_RTC_data == 2) //если запрос температуры
				{
					sprintf (http_send_buffer, "stoika=%s&term=%u\r\n", mod_ip_adress, ptr_RTC_data -> temperature);
					osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
				}
			}		
		}		
		osThreadYield (); 
	}
}

/************************************************установка ip-адреса************************************************/
void Set_New_IP (void const * argument)
{
	uint8_t * getip;
	osEvent event;

	for (;;) 
	{
		event = osMessageGet(Ip_adress_Queue, 50); //ожидание появления данных в очереди
		if (event.status == osEventMessage) //если данные появились в очереди
		{
			getip = (uint8_t *)event.value.v; 
			
			taskENTER_CRITICAL(); //вход в критическую секцию
			write_flash (FLASH_IP_ADDRESS, getip);
			taskEXIT_CRITICAL(); //выход из критической секции	
			
			sprintf (http_send_buffer, "stoika=%s&result=accepted\r\n", mod_ip_adress);
			osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
		}
		osThreadYield (); 
	}
}

/************************************************вывод данных на LCD-дисплей************************************************/
void Show_LCD (void const * argument)
{ 

	uint32_t tickcount = osKernelSysTick();
	uint8_t time [time_size]; //массив с данными времени в числовом виде 
	uint8_t time_array [time_size*2] = {0}; //массив с данными времени в символьном виде
	char led_buffer [15]; //буффер с конвертированными данными времени в строковом отображении
	
	uint8_t dimension = 2; //междустрочное растояние
	
	char tmp_buffer [15]; //временный буффер
	
	uint8_t adress_cell[2] = {0}; //массив с номером ячейки
	uint8_t * ptr_RS485_buffer; //указатель на массив с принятыми по RS-485 данными
	
	for (;;) 
	{
		GetTime (RTC_ADDRESS,  FIRST_RTC_REGISTR_TIME, time_size/2, time); //чтение регистров 0х0-0х2 со значениями времени (с:м:ч)
		GetTime (RTC_ADDRESS,  FIRST_RTC_REGISTR_DATE, time_size/2, &time[time_size/2]); //чтение регистров 0х4-0х6 со значениями даты (д:м:г)
		convert_time (time_size, time_array, time); //конвертация данных времени в символьный вид
		ClearLcdMemory();
		
		LCD_SetFont(Arial_15x17, black);
		LCD_ShowString(2, dimension , mod_ip_adress);
		sprintf (led_buffer, "%c%c.%c%c.%c%c %c%c:%c%c:%c%c",  time_array[6],  time_array[7], time_array[8],  time_array[9], time_array[10],  
		time_array[11],  time_array[4], time_array[5],  time_array[2], time_array[3],  time_array[0], time_array[1]);
		LCD_ShowString(15, (dimension += 14), led_buffer);	
		
		for (size_t count = 0; count <= MAX_SELL; count++) //опрос ячеек
		{		
			
			ptr_RS485_buffer = &cell_state [count][0];	//указатель на массив с данными состояния ячеек	
			if (count != MAX_SELL) //если это ячейки с 1 по MAX_SELL
			{
				adress_cell[0] = ((count+1)/10 + 0x30); //установка номера ячейки
				adress_cell[1] = ((count+1)%10 + 0x30);
				if ((count+1)%2) //если номер ячейки нечётный (1, 3, 5 ... 13)
				{	
					if (*(ptr_RS485_buffer) != 0)
						sprintf  (led_buffer, "%s=%c,%c,%c,%c,%c", adress_cell, *(ptr_RS485_buffer), *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+2), *(ptr_RS485_buffer+3),  *(ptr_RS485_buffer+4));
					else
						sprintf (led_buffer, "%c%c=no_data", adress_cell[0], adress_cell[1]);
				}
				else //если номер ячейки чётный (2, 4 ... 12)
				{
					if (*(ptr_RS485_buffer) != 0)				
						sprintf (tmp_buffer, " %s=%c,%c,%c,%c,%c", adress_cell, *(ptr_RS485_buffer), *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+2), *(ptr_RS485_buffer+3),  *(ptr_RS485_buffer+4));
					else
						sprintf (tmp_buffer, " %c%c=no_data", adress_cell[0], adress_cell[1]);
					strcat (led_buffer, tmp_buffer); //объединение буфферов двух ячеек в один
					LCD_ShowString(2, (dimension+=14), led_buffer); //вывод на дисплей
					led_buffer [0] = '\0'; //обнуление буффера
				}
			}
			else //если это последняя итерация (мастер-ячейка)
			{
				adress_cell[0] = '1';
				adress_cell[1] = '4'; //присвоим мастер-ячейке 14 номер				
				if (led_buffer [0] != '\0') //если в буфере что-то есть
				{
					sprintf (tmp_buffer, " %c%c=%c,%c,%c,%c,%c", adress_cell[0], adress_cell[1], *(ptr_RS485_buffer), *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+3), *(ptr_RS485_buffer+2),  *(ptr_RS485_buffer+4));
					strcat (led_buffer, tmp_buffer); //объединение буфферов двух ячеек в один
				}
				else {
					sprintf (led_buffer, "%c%c=%c,%c,%c,%c,%c", adress_cell[0], adress_cell[1], *(ptr_RS485_buffer), *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+2), *(ptr_RS485_buffer+3),  *(ptr_RS485_buffer+4));}
				LCD_ShowString(2, (dimension+=14), led_buffer);
			}
		}
		LCD_Refresh();
		dimension = 2; //сброс междустрочного расстояния		
		osDelayUntil (&tickcount, 1000);
	}
} 

/************************************************************************************************/
void ProgTimerIWDGCallback(void const *argument)
{
	HAL_IWDG_Refresh(&hiwdg); //перезагрузка iwdg
}

/************************************************************************************************/
void ProgTimerBeeperCallback(void const *argument)
{
	HAL_TIM_PWM_Stop (&htim4, TIM_CHANNEL_4); //выключение бипера
}

/************************************************************************************************/
void ProgTimerMasterKeyCallback (void const *argument)
{
	osMessagePut (Master_Key_Queue, 0, 10); //передача в очередь команды ячейкам на выключение всех соленоидов(0)
}
/************************************************************************************************/
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
