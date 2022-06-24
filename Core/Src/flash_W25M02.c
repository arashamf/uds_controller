
#include "spi.h"
#include "flash_W25M02.h"
#include "gpio.h"

uint8_t READ_STATUS_1 [2] = {0x0F, 0xA0};
uint8_t READ_STATUS_2 [2] = {0x0F, 0xB0};
uint8_t READ_STATUS_3 [2] = {0x0F, 0xC0};

/************************************************************************************************/
void ReadID_W25M (uint8_t * rx_buffer)
{
	uint8_t buffer = READ_ID;
	
	SPI4_CS1_ON;
//	HAL_SPI_Transmit_DMA (&hspi4, &buffer, 1);
//	HAL_SPI_Receive_DMA (&hspi4, rx_buffer, 3); 
	HAL_SPI_Transmit (&hspi4, &buffer, 1, 0xFFFF);
	HAL_SPI_Receive (&hspi4, rx_buffer, 4, 0xFFFF); 
	SPI4_CS1_OFF;
	
	SPI4_CS2_ON;
//	HAL_SPI_Transmit_DMA (&hspi4, &buffer, 1);
	//HAL_SPI_Receive_DMA (&hspi4, rx_buffer+3, 3);
	HAL_SPI_Transmit (&hspi4, &buffer, 1, 0xFFFF);
	HAL_SPI_Receive (&hspi4, rx_buffer+4, 4, 0xFFFF); 
	SPI4_CS2_OFF;
}

/************************************************************************************************/
void Read_SR_W25M (uint8_t * rx_buffer)
{
	SPI4_CS1_ON;

	HAL_SPI_Transmit(&hspi4, READ_STATUS_1, 2, 0xFFFF);
	HAL_SPI_Receive(&hspi4, rx_buffer, 1, 0xFFFF);
	
	HAL_SPI_Transmit(&hspi4, READ_STATUS_2, 2, 0xFFFF);
	HAL_SPI_Receive(&hspi4, rx_buffer+1, 1, 0xFFFF);
	
	HAL_SPI_Transmit(&hspi4, READ_STATUS_3, 2, 0xFFFF);
	HAL_SPI_Receive(&hspi4, rx_buffer+2, 1, 0xFFFF);
/*	HAL_SPI_Transmit_DMA (&hspi4, READ_STATUS_1, 2);
	HAL_SPI_Receive_DMA (&hspi4, rx_buffer, 1); 
	
	HAL_SPI_Transmit_DMA (&hspi4, READ_STATUS_2, 2);
	HAL_SPI_Receive_DMA (&hspi4, rx_buffer+1, 1);
	
	HAL_SPI_Transmit_DMA (&hspi4, READ_STATUS_3, 2);
	HAL_SPI_Receive_DMA (&hspi4, rx_buffer+2, 1);*/
	
	SPI4_CS1_OFF;
	
}

/************************************************************************************************/
void Reset_W25M (void)
{
	uint8_t buf =  RESET_MEMORY;
	SPI4_CS1_ON;
	HAL_SPI_Transmit (&hspi4, &buf, 1, 0xFFFF);
	SPI4_CS1_OFF;
	HAL_Delay(100);
}

/************************************************************************************************/
void W25_Write_Enable(void)
{
	SPI4_CS1_ON;
  uint8_t buf = WRITE_ENABLE;
	HAL_SPI_Transmit (&hspi4, &buf, 1, 0xFFFF);
	SPI4_CS1_OFF;
}

/************************************************************************************************/
void W25_Write_Disable(void)
{
	SPI4_CS1_ON;
  uint8_t buf = WRITE_DISABLE;
	HAL_SPI_Transmit (&hspi4, &buf, 1, 0xFFFF);
	SPI4_CS1_OFF;
}
/************************************************************************************************/
