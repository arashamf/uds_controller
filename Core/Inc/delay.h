#ifndef __DELAY_H__
#define __DELAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stm32f746xx.h"
#include "stm32f7xx.h"
	
void delay_ms(uint16_t delay)	;
void delay_us(uint16_t delay)	;
	
#ifdef __cplusplus
}
#endif

#endif
