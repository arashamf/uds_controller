#ifndef __MYWEB_H
#define __MYWEB_H

#define LENGTH_COMMAND 15
#define LENGTH_PARAMETR 10
#define LENGTH_VAL 20
#define LENGTH_NAME_CELL 8


typedef struct 
{										
char inCommand [LENGTH_COMMAND];
char param [4][LENGTH_PARAMETR];
char val [4][LENGTH_VAL];
char name_cell [LENGTH_NAME_CELL];
signed short type_command;
}PARSE_DATA;


typedef struct  
{
	char RTC_setting [6];
	unsigned char new_ipadress [4];
	signed short type_data;
	unsigned short number_day;
	char answerbuf [51];	
	char registration_data [51];
}RELEASE_DATA;

typedef signed short errortype;

#define NoGet -1
#define NoCommand -2
#define UnknownParam -3
#define UnknownCommand -4
#define CommandNotSupported -5																																																																						
#define TooManyTexts -6
#define TooLongTextString -7
#define ErrorParam -8
#define ErrorNumberCell -9
#define Ok 0
#define WhatTimeIsIt 1
#define WhatIsTemperature 2
#define SetNewTime 3
#define SetNewDate  4
#define NewIpSet 5
#define Read 6

errortype ParseTCP (char *, PARSE_DATA *, RELEASE_DATA *);
errortype make_command (errortype, PARSE_DATA *, RELEASE_DATA*);
void Read_TCP_Message (char *, RELEASE_DATA *);
void convert_ip (char * , unsigned char * );
#endif
