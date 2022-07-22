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
#include "fatfs.h"
#include "cell_command.h"
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
extern char UART3_msg_TX [RS232_BUFFER_SIZE]; //буффер сообщений RS-232
extern char RS485_TXbuffer [TX_BUFFER_SIZE]; //буффер принятых сообщений RS-485
extern uint8_t prev_cell_state [MAX_CELL+1][7]; //буффер состояний ячеек

extern char logSDPath;  // User logical drive path 
extern FIL wlogfile;     //файловый объект для записи
extern FIL rlogfile;     //файловый объект для чтения 
extern FATFS log_fs ;    // рабочая область (file system object) для логических диска


const char httpHeader[] = "HTTP/1.1 200 OK\nContent-type: text/plain\n\n" ;  // HTTP-заголовок
const char power_on[] = " power_on" ;  // 

uint8_t flag_masterkey = 0; //флаг срабатывания мастер ключа
//uint8_t timer_masterkey = 0; //флаг срабатывания мастер ключа

extern uint8_t IP_ADDRESS[4]; //установленный ip-адрес в виде четырёх uint8_t (lwip.c)  
extern char mod_ip_adress [16]; //ip-адрес в символьной форме (например 192.168.001.060) для регистрации и отображения
const size_t time_size = 6; //размер буфера для сохранения показателей времени/даты
uint8_t time_array [time_size*2] = {0}; //массив с данными времени в символьном виде

typedef struct log_out_t 
{
	uint8_t type; //тип команды, 1 - чтение, 2 - запись
	uint16_t id_logfile; //номер логфайла
  char tmpbuffer_registration [50]; //массив с данными для регистрации на SD
} log_out;	

typedef struct 
{
  uint8_t temperature;
  uint8_t RTC_data [time_size];
	} get_RTC_data ; //структура с данными, полученными от микросхемы RTC
	
char http_send_buffer [740]; //буффер, в который записывается сформированный http-ответ

osTimerId osProgTimerIWDG;  //программный таймер перезагружающий сторожевик
osTimerId osProgTimerBeeper;  //программный таймер отключающий бипер
osTimerId osProgTimerMasterKey;  //программный таймер выключающий соленоиды

osMutexId mutex_RS485_Handle; //мьютекс блокировки передачи команд ячейкам
//osMutexId mutex_logfile_Handle; //мьютекс блокировки передачи команд ячейкам

osMessageQId HTTP_msg_Queue; //очередь передачи указателя на полученный HTTP-запрос
osMessageQId HTTP_answer_Queue; //очередьпередачи указателя на сформированный HTTP-ответ
osMessageQId Cell_msg_Queue; //очередь передачи кода команды ячейкам
osMessageQId MasterKey_Command_Queue; //очередь передачи команды управления соленоидами ячеек
osMessageQId Ip_adress_Queue; //очередь для передачи полученного ip-адреса
osMessageQId RTC_typedata_Queue; //очередь для передачи типа запрашиваемых RTC данных	
osMessageQId RS485_msg_Queue; //очередь для передачи полученного по RS-485 сообщения	

osMailQId Registration_Queue; //очередь для передачи структуры регистрации
	
osThreadId Task_Parse_HTTP_msg_Handle;
osThreadId Task_Parsing_Cell_command_Handle;
osThreadId Task_ReadWrite_Log_Handle;
osThreadId Task_Ping_All_Sell_Handle;
osThreadId Task_Control_Solenoid_Handle;
osThreadId Task_RTC_get_time_Handle;
osThreadId Task_SetNewIP_Handle;
osThreadId Task_Show_LCD_Handle;
osThreadId Task_Switch_Led_Handle;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void ProgTimerIWDGCallback(void const *argument);
void ProgTimerBeeperCallback(void const *argument);
void ProgTimerMasterKeyCallback (void const *argument);

void Parse_HTTP_msg (void const * argument);
void Parsing_Cell_command (void const * argument);
void ReadWrite_Log (void const * argument);
void Ping_All_Sell (void const * argument);
void Сontrol_Solenoid (void const * argument);
void RTC_get_time (void const * argument);
void Set_New_IP (void const * argument);
void Show_LCD (void const * argument);
void Switch_Led (void const * argument);
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
	
//	osMutexDef (mutex_logfile); 
//	mutex_logfile_Handle = osMutexCreate(osMutex (mutex_logfile));
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
	
	osMessageQDef (HTTP_answer_Queuename, 3, uint8_t *); 
	HTTP_answer_Queue = osMessageCreate (osMessageQ (HTTP_answer_Queuename), NULL); ///очередь для передачи указателя на сформированное http-сообщение
 
	osMessageQDef (Cell_msg_Queuename, 2, uint16_t); 
	Cell_msg_Queue = osMessageCreate (osMessageQ (Cell_msg_Queuename), NULL); //очередь для кода команды управления ячейками
	
	osMessageQDef (MasterKey_Command_Queuename, 2, signed char); 
	MasterKey_Command_Queue = osMessageCreate (osMessageQ (MasterKey_Command_Queuename), NULL); //очередь передачи команды управления соленоидами ячеек
 
	osMessageQDef (Ip_adress_Queuename, 2, uint8_t *);
	Ip_adress_Queue = osMessageCreate (osMessageQ (Ip_adress_Queuename), NULL); //очередь для передачи полученного ip-адреса

	osMessageQDef(RTC_typedata_Queuename, 2, uint8_t);
	RTC_typedata_Queue = osMessageCreate (osMessageQ (RTC_typedata_Queuename), NULL); //очередь для передачи типа запрашиваемых RTC данных
	
	osMessageQDef (RS485_msg_Queuename, 5, uint8_t *);
	RS485_msg_Queue = osMessageCreate (osMessageQ (RS485_msg_Queuename), NULL); //очередь для передачи полученного по RS-485 сообщения	
		
	osMailQDef(Registration_Queuename, 5, log_out);
  Registration_Queue = osMailCreate (osMailQ(Registration_Queuename), NULL); //очередь для передачи указателя на данные для регистрации
	
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1024);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	osThreadDef (Task_Parse_HTTP_msg, Parse_HTTP_msg, osPriorityNormal, 0, 1024); 
	Task_Parse_HTTP_msg_Handle = osThreadCreate(osThread(Task_Parse_HTTP_msg), NULL); 	 
	
	osThreadDef (Task_Parsing_Cell_command, Parsing_Cell_command, osPriorityNormal, 0, 256); 
	Task_Parsing_Cell_command_Handle = osThreadCreate(osThread(Task_Parsing_Cell_command), NULL); 
	
	osThreadDef (Task_ReadWrite_Log, ReadWrite_Log, osPriorityNormal, 0, 8120); 
	Task_ReadWrite_Log_Handle = osThreadCreate (osThread (Task_ReadWrite_Log), NULL);
	
	osThreadDef (Task_Ping_All_Sell, Ping_All_Sell, osPriorityNormal, 0, 512); 
	Task_Ping_All_Sell_Handle = osThreadCreate (osThread (Task_Ping_All_Sell), NULL);
	
//	osThreadDef (Task_Сontrol_Solenoid, Сontrol_Solenoid, osPriorityNormal, 0, 128); 
//	Task_Control_Solenoid_Handle = osThreadCreate (osThread (Task_Сontrol_Solenoid), NULL); //запуск задачи управления состоянием соленоидов
	
	osThreadDef (Task_Show_LCD, Show_LCD, osPriorityNormal, 0, 128); 
	Task_Show_LCD_Handle = osThreadCreate (osThread (Task_Show_LCD), NULL);
			
	osThreadDef (Task_Switch_Led, Switch_Led, osPriorityNormal, 0, 128); 
	Task_Switch_Led_Handle = osThreadCreate(osThread(Task_Switch_Led), NULL);
	
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
//	convert_ip (mod_ip_adress, IP_ADDRESS);
//	TIM4->CCR4 = 50; //ширина импульса шим = 1/2Т
	osTimerStart(osProgTimerIWDG, 8000); //запуск циклического таймера сторожевика
	
 	struct netconn *conn; //указатель на переменную структуры соединения
	struct netconn *newconn;  //структура соединения для входящего подключения
	struct netbuf *inbuffer; //структура для приёмного буффера
	
	char http_put_buffer [800]; //буффер, в который формируется http-ответ, в которое входит тело сообщения и http-заголовок (должен быть больше буффера http_send_buffer)
	char * http_answer; //указатель на буффер тела сформированного http-сообщения
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
						netbuf_data(inbuffer, (void**)&ptr_http_msg, &len); //копирование полученного http-сообщения
						osMessagePut (HTTP_msg_Queue, (uint32_t)ptr_http_msg, 10); //передача в очередь указателя на полученное http-сообщение
						event = osMessageGet(HTTP_answer_Queue, 150); //ожидание появления данных в очереди
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

//******************************************парсинг tcp-сообщения и передача полученных данных другим задачам******************************************//
void Parse_HTTP_msg (void const * argument)
{
	char * http_get_data; //полученный код команды из очереди
	osEvent event;
	RELEASE_DATA parse_output; //обявление структуры с парсированными данными из http-сообщения
	RELEASE_DATA *ptr_data = &parse_output;
	log_out *ptr_logmsg; //указатель на структуры с данными для регистрации на SD
	signed short buf = 0; //буфер для хранения кода команды
	
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
							
							case 2: //если запрос температуры
								osMessagePut (RTC_typedata_Queue, (uint32_t)buf, 10); //передача команды запроса времени/температуры задаче RTC_get_time
								osThreadDef (Task_RTC_get_time, RTC_get_time, osPriorityNormal, 0, 128); 
								Task_RTC_get_time_Handle = osThreadCreate (osThread (Task_RTC_get_time), NULL);
								break;
							
							case 3: //если установка времени
								SetTime (RTC_ADDRESS,  0x0, ptr_data->RTC_setting);		
								buf = 1;
								break;		
							
							case 4: //если установка даты
								SetTime (RTC_ADDRESS,  0x4, ptr_data->RTC_setting);	
								buf = 1;
								break;	
							
							case 5: //если установка ip-адреса 
								osMessagePut(Ip_adress_Queue, (uint32_t)ptr_data->new_ipadress, 10); //передача полученного для установке ip-адреса задаче Set_New_IP
								osThreadDef (Task_SetNewIP, Set_New_IP, osPriorityAboveNormal, 0, 128); 
								Task_SetNewIP_Handle = osThreadCreate (osThread (Task_SetNewIP), NULL);
								break;
							
							case 6: //если чтение лог-файла
								ptr_logmsg = osMailAlloc(Registration_Queue, osWaitForever);
								ptr_logmsg->type = READ_LOG; //чтение из лога
								ptr_logmsg-> id_logfile = ptr_data->number_day; //номер дня для названия файла
								osMailPut(Registration_Queue, ptr_logmsg);
								break;
							
							default:
								break;
						}
					}
				}
			}	
			else //подготовка ответа при некорректном запросе
			{
				sprintf (http_send_buffer, "stoika=%s_result&%s\r\n", mod_ip_adress, ptr_data->answerbuf); //формирования сообщения
				osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //указатель на ответное http-сообщение в очередь для задачи StartDefaultTask
			}
			if (ptr_data->registration_data[0] != '\0') //регистрация полученной команды
			{
				ptr_logmsg = osMailAlloc(Registration_Queue, osWaitForever);
				strncpy (ptr_logmsg->tmpbuffer_registration, ptr_data->registration_data, (strlen (ptr_data->registration_data) + 1));
				ptr_logmsg->type = WRITE_LOG; //запись в лог
				osMailPut(Registration_Queue, ptr_logmsg);
			}
		}		
		osThreadYield (); //Передача управление следующему потоку с наивысшем приоритетом, который находится в состоянии ГОТОВ
	}
}

//*****************************************формирование команды для ячеек по результатам парсинга tcp-сообщения*****************************************//
void Parsing_Cell_command (void const * argument)
{
	osEvent event1, event2;
	uint16_t number_cell = 0; //номер ячейки в числовой форме
	
	uint16_t command = 0; //шифр полученной команды (тип команды + номер ячейки)
	uint8_t typecommand = 0; //тип отправляемой команды
	
	char tmp_buffer [15] = {0}; //времменный буфер-массив
	uint8_t * ptr_cell_array; //указатель на массив с данными о ячейках
	
	RS485_TXbuffer [0] = 0x2; //1 байт при передаче от контроллера к ячейки всегда равен числу 2
	
  for(;;)
  {
		event1 = osMessageGet(Cell_msg_Queue, 50); //ожидание появления данных в очереди
		if (event1.status == osEventMessage) //если данные появились в очереди
		{
			command = event1.value.v; //получение сообщения
			http_send_buffer [0] = '\0'; //буффер, в который записывается сформированный http-ответ
			if (command == 200) //если тип команды - широковещательный state
			{	

				sprintf (http_send_buffer,"stoika=%s&state=ok\r\n", mod_ip_adress);											
				for (size_t count = 1; count <= MAX_CELL; count++)
				{	
					ptr_cell_array = &prev_cell_state [count][0];	//указатель на массив с данными состояния ячеек
					if (*(ptr_cell_array+2) != 0) //если от ячейки получены данные состояния
					{
						sprintf  (tmp_buffer, "cell_%c%c=%c,%c,%c,%c,%c\r\n", *ptr_cell_array, *(ptr_cell_array+1), *(ptr_cell_array+2), *(ptr_cell_array+3),
						*(ptr_cell_array+4), *(ptr_cell_array+5), *(ptr_cell_array+6));	
						strcat (http_send_buffer, tmp_buffer);
					}
					else	//если данные не получены от ячейки
					{ 
						sprintf (tmp_buffer, "cell_%c%c=no\r\n", *ptr_cell_array, *(ptr_cell_array+1));
						strcat (http_send_buffer, tmp_buffer);
					}			
				}
				ptr_cell_array = &prev_cell_state [0][0];	//указатель на массив с данными мастер-ячейки
				sprintf  (tmp_buffer, "cell_%c%c=%c,%c,%c,%c,%c\r\n", *ptr_cell_array, *(ptr_cell_array+1), *(ptr_cell_array+2), *(ptr_cell_array+3),
				*(ptr_cell_array+4), *(ptr_cell_array+5), *(ptr_cell_array+6));	//состояние мастер-ячейки
				strcat (http_send_buffer, tmp_buffer);
				osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
			}			
			else
			{
				RS485_TXbuffer [3] = command/100 + 0x30; //шифр комманды
				if (RS485_TXbuffer [3] == '3') //если зашифрована команда close
					RS485_TXbuffer [3] = '0'; //передадим символ 0
				typecommand = RS485_TXbuffer [3]-0x30; //запоминание типа полученной команды
				number_cell =  command % 100; //номер ячейки
				
				RS485_TXbuffer [1] = (number_cell/10 + 0x30); //старший символ номера ячейки
				RS485_TXbuffer [2] = (number_cell%10 + 0x30); //младший символ номера ячейки
													
				switch (typecommand)
				{
					case 0: //если запрос типа close ячейки
						if (osMutexWait (mutex_RS485_Handle, 100) == osOK)
						{	
							PutCommandToCell (RS485_TXbuffer); //отправка команды ячейке по RS-485		
							osMutexRelease (mutex_RS485_Handle);
						}
						sprintf (http_send_buffer,"stoika=%s&close_port=%c%c&result=accepted", mod_ip_adress, RS485_TXbuffer [1], RS485_TXbuffer [2]);
						event2 = osMessageGet(RS485_msg_Queue, 4); //ожидание появления данных в очереди
						if (event2.status == osEventMessage) //если данные появились в очереди (ответ на команду close, сейчас я их не использую)
							{(void)event2.value.v;}
						osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на сформированный ответ
						break;
				
					case 1: //если запрос типа open ячейки
						if (osMutexWait (mutex_RS485_Handle, 100) == osOK)
						{	
							PutCommandToCell (RS485_TXbuffer); //отправка команды ячейке по RS-485		
							osMutexRelease (mutex_RS485_Handle);
						}
						sprintf (http_send_buffer,"stoika=%s&open_port=%c%c&result=accepted", mod_ip_adress, RS485_TXbuffer [1], RS485_TXbuffer [2]);
						event2 = osMessageGet(RS485_msg_Queue, 4); //ожидание появления данных в очереди
						if (event2.status == osEventMessage) //если данные появились в очереди (ответ на команду open, сейчас я их не использую)
							{(void)event2.value.v;}	
						osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на сформированный ответ
						break;
				
					case 2: //если запрос типа state ячейки
						sprintf (http_send_buffer,"stoika=%s&state=ok", mod_ip_adress);
						ptr_cell_array = &prev_cell_state [number_cell][0];	//указатель на массив с данными состояния ячеек		
						if (*(ptr_cell_array+2) != 0) //если есть данные от ячейки
						{	
							sprintf  (tmp_buffer, "&cell_%c%c=%c,%c,%c,%c,%c", *ptr_cell_array, *(ptr_cell_array + 1), *(ptr_cell_array + 2), *(ptr_cell_array + 3), 
							*(ptr_cell_array + 4), *(ptr_cell_array + 5), *(ptr_cell_array + 6));	
							strcat (http_send_buffer, tmp_buffer);
							osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
						}
						else	 //если не получен ответ от ячейки
						{ 
							sprintf (tmp_buffer, "&cell_%c%c=no", *ptr_cell_array, *(ptr_cell_array + 1));
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

//***********************************************************запись/чтение в лог файл SD карты***********************************************************//
void ReadWrite_Log (void const * argument)
{
	osEvent event; 
	size_t length = 0;
	
	char logbuffer [130]; //буффер сообщений для логирования
	char tmp_buffer[130]; //временный буффер
	
	char write_file [15]; //название файла для логгирования	
	char read_file [15] = {0}; //название файла для логгирования	
	
	log_out *ptr_logstring; //указатель на структуры с данными для регистрации на SD
	uint8_t flag_EOF = 0; //флаг окончания лог-файла, 0 - файл ещё не кончился, 1 - EOF
	unsigned long bytesread = 0; //количество байт которые необходимо прочитать
	FRESULT result; //код возврата функций FatFs
//	unsigned long long file_size = 0; //размер файла
	
	for (;;)
	{
		event = osMailGet(Registration_Queue, 50);
		if (event.status == osEventMail)
    {	
			FATFS_LinkDriver(&SD_Driver, SDPath);
			if ((result = (f_mount(&log_fs, (TCHAR const*)SDPath, 1))) != FR_OK)  //монтирование рабочей области (0 - отложенное, 1 - немедленное монтирование)  
			{
				sprintf (UART3_msg_TX, "SD_card_error=%u\r\n", result);
				UART3_SendString (UART3_msg_TX);
				if ((ptr_logstring->type) == READ_LOG) //если данные было необходимо считать с SD-карты
				{
					sprintf (http_send_buffer, "stoika=%s&file_num=%u&sd_fat_", mod_ip_adress, 	ptr_logstring->id_logfile);	
					sprintf (tmp_buffer, "init=255\r\n"); //если SD-карту не удалось примонтировать 
					strcat (http_send_buffer, tmp_buffer);
					osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на сформированное сообщение					
				}				
			}						
			else
			{				
				ptr_logstring = event.value.p;
				if ((ptr_logstring->type) == WRITE_LOG) //если данные необходимо записать на SD
				{	
					sprintf (write_file, "LOG__%u.txt", get_file_title ());
					sprintf (tmp_buffer, "  %c%c/%c%c/%c%c %c%c:%c%c:%c%c",  time_array[6],  time_array[7], time_array[8],  time_array[9], time_array[10],  
					time_array[11],  time_array[4], time_array[5],  time_array[2], time_array[3],  time_array[0], time_array[1]); //запись даты и времени 
					sprintf (logbuffer, "%s", mod_ip_adress); //запись ip_adressа
					strcat (logbuffer, tmp_buffer);
					if (strncmp (ptr_logstring->tmpbuffer_registration, "__cell", 6) == 0) //если это строка записи статуса состояния ячейки
					{
						strcat (logbuffer, ptr_logstring->tmpbuffer_registration); //запись полученных данных
						length = strlen (logbuffer); //длина буффера
						if (length < 126) 
						{
							for (size_t count = length; count < 126; count++) {
								logbuffer [count] = ' ';} 	//заполнение неиспользуемых символов пробелом по 126 элемент строки включительно
						}
					}
					else  //если это строка записи команды
					{
						sprintf (tmp_buffer, " command");
						strcat (logbuffer, tmp_buffer);
						strcat (logbuffer, ptr_logstring->tmpbuffer_registration); //запись полученных данных
						length = strlen (logbuffer); //длина буффера
						if (length < 126) 
						{
							for (size_t count = length; count < 126; count++) {
								logbuffer [count] = '_';} 	//заполнение неиспользуемых символов нижним подчёркиванием по 126 элемент строки включительно
						}
					}
					logbuffer [126] ='\r'; logbuffer [127] ='\n'; logbuffer [128] ='\0';
					if ((result = f_open (&wlogfile, write_file, FA_OPEN_APPEND|FA_WRITE)) != FR_OK) //если файл существует, то он будет открыт со смещением в конце файла, если нет, то будет создан новый
					{					
						sprintf (UART3_msg_TX,"incorrect_open_writefile. code=%u\r\n", result);
						UART3_SendString (UART3_msg_TX);
					}
					else //если удалось открыть файл
					{
						write_txt (&wlogfile, result, logbuffer);
						if ((result = f_close(&wlogfile)) != FR_OK)
						{
							sprintf (UART3_msg_TX,"incorrect_sync_writefile,code=%u\r\n", result);
							UART3_SendString (UART3_msg_TX);
						}
					}	
					osMailFree(Registration_Queue, ptr_logstring);
				}
				if ((ptr_logstring->type) == READ_LOG) //если данные необходимо считать с SD-карты
				{
					sprintf (read_file, "LOG__%u.txt", 	ptr_logstring->id_logfile); //сохранения искомого имени файла
					sprintf (http_send_buffer, "stoika=%s&file_num=%u&sd_fat_", mod_ip_adress, 	ptr_logstring->id_logfile);	

					if ((result = f_open (&rlogfile, read_file, FA_READ)) != FR_OK) //если файл отсутствует или его не удаётся открыть
					{	
						sprintf (UART3_msg_TX,"incorrect_open_readfile. code=%u\r\n", result);
						UART3_SendString (UART3_msg_TX);
						bytesread = 0;								
						sprintf (tmp_buffer, "init=ok&sd_fat_assign0x00=no_file\r\n"); //если файл не удалось открыть
						strcat (http_send_buffer, tmp_buffer);				
					}	
					else
					{
						if (bytesread == 0) //если это начало файла
						{
							sprintf (tmp_buffer, "init=ok&sd_fat_assign0x00=file_ok&size=xx&sd_fat_read=\r\n");
							strcat (http_send_buffer, tmp_buffer);					
						}
						else //если продолжение чтения файла
						{ 
							sprintf (tmp_buffer, "read=\r\n");
							strcat (http_send_buffer, tmp_buffer);		
						}
						for (size_t count = 0; count < 5; count++) //запись в буффер 5 строчек из лога
						{
							f_lseek(&rlogfile, bytesread); //смещение указателя внутри файла на byteswritten байт
							if (f_eof (&rlogfile)==0) //если файл ещё не кончился
							{
								f_gets (tmp_buffer, sizeof (tmp_buffer), &rlogfile); //копирование одной строки
								bytesread += (strlen (tmp_buffer) + 1); //вычисление смещения	
								strcat (http_send_buffer, tmp_buffer);
							}
							else //если файл кончился (меньше 5 строк в файле)
							{
								flag_EOF = 1; //установка флага окончания чтения файла
								if (count < 4)
								{
									sprintf (tmp_buffer, "\r\n");
									strcat (http_send_buffer, tmp_buffer);
								}
								break;							
							}
						}
						if (f_eof (&rlogfile)!=0) //если файл всё-таки кончился
						{
							flag_EOF = 1; //установка флага окончания чтения файла
						}	
						if (flag_EOF == 0) //если чтение не достигла конца лог-файла
						{
							sprintf (tmp_buffer, "&continued=true\r\n"); 
							strcat (http_send_buffer, tmp_buffer);
						}
						else
						{
							sprintf (tmp_buffer, "&continued=false\r\n"); //если чтение достигло конца лог-файла
							strcat (http_send_buffer, tmp_buffer);
							flag_EOF = 0; //установка флага продолжение чтения файла
							bytesread = 0;	//обнуление смещения	в файле				
						}
						if ((result = f_close(&rlogfile)) != FR_OK)
						{
							sprintf (UART3_msg_TX,"incorrect_close_readfile. code=%u\r\n", result);
							UART3_SendString (UART3_msg_TX);
						}					
					}	
					osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
				}								
			}
			osMailFree(Registration_Queue, ptr_logstring);	
			FATFS_UnLinkDriver(SDPath);			
			osThreadYield ();
		}
	}
}


//************************************************периодическая отправка всем ячейкам запроса типа state************************************************//
void Ping_All_Sell (void const * argument)
{
	osEvent event; 
	uint8_t * ptr_RS485_msg; //указатель на принятое сообщение по RS-485 (UART1_msg_RX)
	uint8_t * ptr_cell_state; //указатель на массив с сохраннёными данными от ячеек
	uint8_t cell_state [5] = {0}; //буффер с промежуточными данными от контроллеров ячеек
	log_out *ptr_logout;  //указатель на структуры с данными для регистрации на SD
	
	uint8_t flag_write_log = 0; //флаг для записи данных в лог-файл
	
	uint8_t * ptr_mastercell = &prev_cell_state [0][0]; //указатель на массив с данными мастер-ячейки
	*(ptr_mastercell+2) = *(ptr_mastercell+6) = '0'; //запись '0' в 0 и 4 бит мастер-ячейки 
	uint16_t status_inputs = 0; //новый статус мастерячеек
	uint16_t old_status_inputs = 0; //сохраннёный статус мастер-ячейки
	uint8_t MasterKeyStatus = (((READ_BIT(GPIOE->IDR, MASTER_KEY_Pin)) >> 4) + 0x30); //статус мастер ключа
	
	ptr_logout = osMailAlloc(Registration_Queue, osWaitForever);
	strcpy(ptr_logout->tmpbuffer_registration," power_on"); //первое сообщение на регистрации должно быть power_on
	ptr_logout->type = WRITE_LOG; //запись в лог
	osDelay (100); //задержка
	osMailPut(Registration_Queue, ptr_logout); //отправка на регистрацию
	
//	osDelay (100); //задержка
	
	for (;;)
	{
		RS485_TXbuffer [0] = 0x2; 	
		for (size_t count = 1; count <= MAX_CELL; count++) //опрос ячеек начиная с 1
		{
			ptr_cell_state = &prev_cell_state [count][0]; //указатель с сохранёнными данными от ячеек
			RS485_TXbuffer [1] = *(ptr_cell_state); //старший символ номера ячейки	
			RS485_TXbuffer [2] = *(ptr_cell_state+1); //младший символ номера ячейки
			RS485_TXbuffer [3] = '2'; //команда state	

			if (osMutexWait (mutex_RS485_Handle, 20) == osOK) //ожидание и захват мьютекса
			{		
				PutCommandToCell (RS485_TXbuffer);			
			
				event = osMessageGet(RS485_msg_Queue, 2); //ожидание появления данных в очереди
				if (event.status == osEventMessage) //если данные появились в очереди
				{	
					ptr_RS485_msg = (uint8_t *)event.value.v;
					memcpy (cell_state, (ptr_RS485_msg+1), 5); //копирование 5 символов сообщения от ячейки, начиная с 2 элемента
				}
				else //если не получен ответ от контроллера ячейки
				{
					cell_state [0] = 0;					
				}
				osMutexRelease (mutex_RS485_Handle);
			}
			
			if (cell_state [0] == 0) //если от ячейки не получены данные
			{
				if ((*(ptr_cell_state+2)) != 0) //если ячейка стала не активна только что
					*(ptr_cell_state+2) = 0;
			}
			else //ВН�?МАН�?Е! Это костыль! инверс 2 бита (статус ригеля замка) для совместимости с программой-клиентом
			{
				if (cell_state [1] == '0') 
					cell_state [1] = '1';
				else
				{
					if (cell_state [1] == '1')
						cell_state [1] = '0';
				}
			}
			if (cell_state [0] != 0) //если от ячейки получены данные
			{
				if((*(ptr_cell_state+2) !=cell_state [0])|| (*(ptr_cell_state+3) !=cell_state [1])|| //если данные изменились
					(*(ptr_cell_state+4) !=cell_state [2])|| (*(ptr_cell_state+5) !=cell_state [3])|| 	(*(ptr_cell_state+6) !=cell_state [4]))  						 						 				
				{
					//					if (*(ptr_cell_state+2) != 0) //не регистрируем данные о ячейках сразу после включения
						{flag_write_log = 1;} //данные необходимо зарегистрировать
							
					*(ptr_cell_state+2) = cell_state [0];  //сохраним полученные данные     
					*(ptr_cell_state+3) = cell_state [1];
					*(ptr_cell_state+4) = cell_state [2];
					*(ptr_cell_state+5) = cell_state [3];
					*(ptr_cell_state+6) = cell_state [4];
						
					if (flag_write_log == 1)	 //если данные необходимо зарегистрировать
					{							
						ptr_logout = osMailAlloc(Registration_Queue, osWaitForever);
						sprintf (ptr_logout->tmpbuffer_registration, "__cell_%c%c=%c,%c,%c,%c,%c", *(ptr_cell_state), *(ptr_cell_state+1), *(ptr_cell_state+2), 
						*(ptr_cell_state+3), *(ptr_cell_state+4), *(ptr_cell_state+5), *(ptr_cell_state+6));
						ptr_logout->type = WRITE_LOG; //запись в лог
						osMailPut(Registration_Queue, ptr_logout);
						flag_write_log = 0;
						osDelay (5); //задержка для записи
					}
				}
			}
		}
		
		//проверка состояния микриков мастер-ячейки
		if ((status_inputs = READ_BIT(GPIOE->IDR,  SIDE_COVER_Pin | MASTER_KEY_Pin | TOP_COVER_Pin)) != old_status_inputs) //если состояние мастер ключей было изменено
		{			
			old_status_inputs = status_inputs; //сохраняем состояние
			osDelay (50); //ожидание окончание дребезга
			if ((status_inputs = READ_BIT(GPIOE->IDR,  SIDE_COVER_Pin | MASTER_KEY_Pin | TOP_COVER_Pin)) == old_status_inputs) //если состояние мастер ключей не измененялось за 50 мс
			{
				for (size_t count = 3; count <= 5; count++)
				{
					if (((status_inputs >> count) & 0x1) == 0x0) { //если считанный бит равен 0, (контакт замкнут)
						*(ptr_mastercell+count) = '1';} //сохраним 1 в буффер состояния ключей
					else {
						*(ptr_mastercell+count) = '0';}					
						
					if (count == 5)  //если это бит состояния мастер ключа
					{
						if (MasterKeyStatus != (*(ptr_mastercell+count))) //если состояние мастер-ключа изменилось
						{
							if (*(ptr_mastercell+count) ==  '1') //если мастер-ключ был переведён в положение включено
							{
								if (flag_masterkey == SOLENOIDS_OFF) //если мастер-ключ не был до этого момента нажат (соленоиды выключены)
								{	
									
									osMessagePut(MasterKey_Command_Queue, SOLENOIDS_ON, 5); //передача в очередь команды на включение всех соленоидов	
		//							osThreadResume (Task_Control_Solenoid_Handle);
									osThreadDef (Task_Сontrol_Solenoid, Сontrol_Solenoid, osPriorityNormal, 0, 128); 
									Task_Control_Solenoid_Handle = osThreadCreate (osThread (Task_Сontrol_Solenoid), NULL); //запуск задачи управления состоянием соленоидов
								}
							}
							else 
							{
								if (*(ptr_mastercell+count) == '0') //если мастер-ключ был переведён в положение выключено 
								{
									if (flag_masterkey == SOLENOIDS_ON)  //если мастер-ключ был до этого момента нажат (соленоиды включены)
									{	
										osMessagePut(MasterKey_Command_Queue, SOLENOIDS_OFF, 5); //передача в очередь команды на выключение всех соленоидов	
			//							osThreadResume (Task_Control_Solenoid_Handle);
									}
								}
							}
							MasterKeyStatus = (*(ptr_mastercell+count)); //сохраним новое состояние мк
						}
					}
				}
				ptr_logout = osMailAlloc(Registration_Queue, osWaitForever);
				sprintf (ptr_logout->tmpbuffer_registration, "__cell_%c%c=%c,%c,%c,%c,%c", *(ptr_mastercell), *(ptr_mastercell+1), *(ptr_mastercell+2), 
				*(ptr_mastercell+3), *(ptr_mastercell+4), *(ptr_mastercell+5), *(ptr_mastercell+6));
				ptr_logout->type = WRITE_LOG; //данные для записи в лог
				osMailPut(Registration_Queue, ptr_logout); //регистрация изменившегося статуса мастер ячейки
				osDelay (5); //задержка для записи
			}
		}
		osDelay (20);
	}
}

//*************************передача команды включения/отключения соленоида всем ячейкам отправка всем ячейкам запроса типа state*************************//
void Сontrol_Solenoid (void const * argument)
{
	osEvent event;
	uint8_t * ptr_cell_state; //указатель на массив с сохраннёными данными от ячеек
	signed char type_MasterKey_command;
//	ptr_cell_state = &prev_cell_state [0][2];	
	
	for (;;) 
	{
		event = osMessageGet(MasterKey_Command_Queue, 50); //ожидание появления данных в очереди
		if (event.status == osEventMessage) //если данные появились в очереди
		{
			type_MasterKey_command = (uint8_t)event.value.v;			
			switch (type_MasterKey_command)
			{
				case SOLENOIDS_OFF:
					osTimerStop (osProgTimerMasterKey);
					flag_masterkey = SOLENOIDS_OFF;
					command_AllCell (SOLENOIDS_OFF, MAX_CELL, &ptr_cell_state, 500); //передача команды выключения всем соленоидам						
					LED_GREEN (0);
					osThreadTerminate(Task_Control_Solenoid_Handle); //засуспендим задачу 
					break;
				
				case SOLENOIDS_ON:
					LED_GREEN (1);
					command_AllCell (SOLENOIDS_ON, MAX_CELL, &ptr_cell_state, 500); //передача команды выключения всем соленоидам	
					flag_masterkey = SOLENOIDS_ON;
					osTimerStart (osProgTimerMasterKey, 60000); //запуск таймера на выключение соленоидов				
					break;
				
				default:
					break;
			}						
		}	
		osThreadYield ();
//		 osThreadSuspend (Task_Control_Solenoid_Handle);
	}
}

//**************************************************************запрос времени/температуры**************************************************************//
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
			reg_adr = 0x4; //адрес начального регистра с данными даты
			GetTime (RTC_ADDRESS, reg_adr, 3, (ptr_RTC_data -> RTC_data)+3); //чтение регистров 0х4-0х6 (дата: дд/мм/гг)
			reg_adr = 0x11; //адрес регистра с данными  температуры
			GetTime (RTC_ADDRESS, reg_adr, 1, &ptr_RTC_data -> temperature); //чтение регистра 0х11 (температура)
				
			if (type_RTC_data == 1) //если запрос времени и даты
			{
				convert_time (time_size, time_array, ptr_RTC_data -> RTC_data);
				sprintf (http_send_buffer, "stoika=%s&rtcr=_%c%c/%c%c/%c%c_%c%c:%c%c:%c%c\r\n", mod_ip_adress, time_array [6], time_array [7], time_array [8], time_array [9], 
				time_array [10], time_array [11], time_array [4], time_array [5], time_array [2], time_array [3], time_array [0], time_array [1]);
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
		osThreadTerminate(Task_RTC_get_time_Handle); //засуспендим задачу 
	}
}

//**************************************************************вывод данных на LCD-дисплей**************************************************************//
void Show_LCD (void const * argument)
{ 

	uint32_t tickcount = osKernelSysTick();
	uint8_t time [time_size]; //массив с данными времени в числовом виде 
	char led_buffer [15]; //буффер с конвертированными данными времени в строковом отображении	
	uint8_t dimension = 2; //междустрочное растояние	
	char tmp_buffer [15]; //временный буффер	
	uint8_t * ptr_RS485_buffer; //указатель на массив с принятыми по RS-485 данными
	
	convert_ip (mod_ip_adress, IP_ADDRESS);
	
	for (;;) 
	{
		GetTime (RTC_ADDRESS,  FIRST_RTC_REGISTR_TIME, time_size/2, time); //чтение регистров 0х0-0х2 со значениями времени (с:м:ч)
		GetTime (RTC_ADDRESS,  FIRST_RTC_REGISTR_DATE, time_size/2, &time[time_size/2]); //чтение регистров 0х4-0х6 со значениями даты (д:м:г)
		convert_time (time_size, time_array, time); //конвертация данных времени в символьный вид
		ClearLcdMemory();
		
		LCD_SetFont(Arial_15x17, black);
		LCD_ShowString(25, dimension , mod_ip_adress);
		sprintf (led_buffer, "%c%c/%c%c/%c%c %c%c:%c%c:%c%c",  time_array[6],  time_array[7], time_array[8],  time_array[9], time_array[10],  
		time_array[11],  time_array[4], time_array[5],  time_array[2], time_array[3],  time_array[0], time_array[1]);
		LCD_ShowString(15, (dimension += 14), led_buffer);	
		
		for (size_t count = 1; count <= MAX_CELL; count++) //опрос ячеек
		{		
			
			ptr_RS485_buffer = &prev_cell_state [count][0];	//указатель на массив с данными состояния ячеек	
			if (count%2) //если номер ячейки нечётный (1, 3, 5 ... 13)
			{	
				if (*(ptr_RS485_buffer+2) != 0) { //если от ячейки получены данные
					sprintf  (led_buffer, "%c%c=%c,%c,%c,%c,%c", *(ptr_RS485_buffer), *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+2), 
					*(ptr_RS485_buffer+3),  *(ptr_RS485_buffer+4), *(ptr_RS485_buffer+5),  *(ptr_RS485_buffer+6));
				}
				else {
					sprintf (led_buffer, "%c%c=no_data",  *(ptr_RS485_buffer), *(ptr_RS485_buffer+1));
				}
			}
			else //если номер ячейки чётный (2, 4 ... 12)
			{
				if (*(ptr_RS485_buffer+2) != 0)	{	//если от ячейки получены данные		
					sprintf (tmp_buffer, " %c%c=%c,%c,%c,%c,%c", *(ptr_RS485_buffer), *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+2), *(ptr_RS485_buffer+3),  
					*(ptr_RS485_buffer+4), *(ptr_RS485_buffer+5),  *(ptr_RS485_buffer+6));
				}
				else {
					sprintf (tmp_buffer, " %c%c=no_data", *(ptr_RS485_buffer), *(ptr_RS485_buffer+1));
				}
				strcat (led_buffer, tmp_buffer); //объединение буфферов двух ячеек в один
				LCD_ShowString(2, (dimension+=14), led_buffer); //вывод на дисплей
				led_buffer [0] = '\0'; //обнуление буффера
			}
		}	
		ptr_RS485_buffer = &prev_cell_state [0][0]; //указатель на мастер ячейку
		if (led_buffer [0] != '\0') //если в буфере что-то есть
		{
			sprintf (tmp_buffer, " %c%c=%c,%c,%c,%c,%c", *(ptr_RS485_buffer), *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+2), 
			*(ptr_RS485_buffer+3), *(ptr_RS485_buffer+4), *(ptr_RS485_buffer+5),  *(ptr_RS485_buffer+6));
			strcat (led_buffer, tmp_buffer); //объединение буфферов двух ячеек в один
		}
		else {
			sprintf (led_buffer, "%c%c=%c,%c,%c,%c,%c", *(ptr_RS485_buffer), *(ptr_RS485_buffer+1), *(ptr_RS485_buffer+2), 
			*(ptr_RS485_buffer+3), *(ptr_RS485_buffer+4), *(ptr_RS485_buffer+5),  *(ptr_RS485_buffer+6));
		}
		LCD_ShowString(2, (dimension += 14), led_buffer);
		LCD_Refresh ();
		dimension = 2; //сброс междустрочного расстояния		
		osDelayUntil (&tickcount, 1000);
	}
} 

//***************************************************************установка ip-адреса***************************************************************//
void Set_New_IP (void const * argument)
{
	uint8_t * getip;
	osEvent event;

	for (;;) 
	{
		event = osMessageGet(Ip_adress_Queue, 100); //ожидание появления данных в очереди
		if (event.status == osEventMessage) //если данные появились в очереди
		{
			getip = (uint8_t *)event.value.v; 
			
			taskENTER_CRITICAL(); //вход в критическую секцию
			write_flash (FLASH_IP_ADDRESS, getip);
			taskEXIT_CRITICAL(); //выход из критической секции	
			
			sprintf (http_send_buffer, "stoika=%s&result=accepted\r\n", mod_ip_adress);
			osMessagePut (HTTP_answer_Queue, (uint32_t)http_send_buffer, 10); //передача в очередь указателя на полученное сообщение
		}
		osThreadTerminate(Task_SetNewIP_Handle); //засуспендим задачу по установки ip-адреса
	}
}

//***************************************************************мигание светодиодом***************************************************************//
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
/**************************************************************************************************************************************************/
void ProgTimerIWDGCallback(void const *argument)
{
	HAL_IWDG_Refresh(&hiwdg); //перезагрузка iwdg
}

/**************************************************************************************************************************************************/
void ProgTimerBeeperCallback(void const *argument)
{
	HAL_TIM_PWM_Stop (&htim4, TIM_CHANNEL_4); //выключение бипера
}

/**************************************************************************************************************************************************/
void ProgTimerMasterKeyCallback (void const *argument)
{
	if (flag_masterkey == SOLENOIDS_ON) { //если мастер-ключ был до этого момента нажат (соленоиды включены)
	{
		osMessagePut(MasterKey_Command_Queue, SOLENOIDS_OFF, 5);} //передача в очередь команды на выключение всех соленоидов
	//	osThreadResume (Task_Control_Solenoid_Handle);
	}		
}
/**************************************************************************************************************************************************/
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
