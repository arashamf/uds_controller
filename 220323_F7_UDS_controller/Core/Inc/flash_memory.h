#ifndef __FLASH_MEMORY_H__
#define __FLASH_MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "usart.h"
#include "stm32f7xx.h"
	
void write_flash (uint32_t , uint8_t * );
uint8_t read_flash (uint32_t , uint8_t *);
void erase_flash_7sector (void);
	
#ifdef __cplusplus
}
#endif

#endif
