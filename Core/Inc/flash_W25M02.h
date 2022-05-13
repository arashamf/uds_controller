
#ifndef __FLASH_W25M02_H__
#define __FLASH_W25M02_H__

#include "main.h"

#define RESET_MEMORY 0xFF
#define READ_ID 0x9F
#define READ_SR 0x0F
#define READ_DATA 0x03
#define FAST_READ 0x0B
#define WRITE_ENABLE 0x06
#define WRITE_DISABLE 0x06

void ReadID_W25M (uint8_t * );
void Read_SR_W25M (uint8_t * );
void Reset_W25M (void);
void W25_Write_Enable(void);
void W25_Write_Disable(void);
#endif /* __FLASH_W25M02_H__ */

