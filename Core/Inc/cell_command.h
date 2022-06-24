
#ifndef __CELL_COMMAND_H__
#define __CELL_COMMAND_H__

#ifdef __cplusplus
extern "C" {
#endif

/* USER CODE BEGIN Includes */
#include "main.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "usart.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Prototypes */
void PutCommandToCell (char * );
void command_AllCell (uint8_t , uint8_t, uint8_t **, uint16_t );
//void command_AllCell (uint8_t , uint8_t, uint16_t );
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __CELL_COMMAND_H__ */

