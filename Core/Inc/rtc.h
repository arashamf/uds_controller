#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"	
#include "i2c.h"
#include "usart.h"

#define FIRST_RTC_REGISTR_TIME 0x0
#define FIRST_RTC_REGISTR_DATE 0x4
	
void GetTime (uint8_t , uint8_t , uint8_t , uint8_t * );
void GetTemp (uint8_t  , uint8_t ,  uint8_t * );
void SetTime (uint8_t , uint8_t , char *);
uint8_t RTC_ConvertToDec(uint8_t );
uint8_t RTC_ConvertToBinDec(uint8_t );
void edit_RTC_data (I2C_HandleTypeDef hi, uint8_t , char * );
void read_reg_RTC (I2C_HandleTypeDef hi, uint8_t );
void convert_time (unsigned char , unsigned char * , unsigned char * );
	
#ifdef __cplusplus
}
#endif

#endif /* RTC_H */

