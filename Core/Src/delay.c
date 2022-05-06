#include "delay.h"

void delay_ms(uint16_t delay)
{

	SET_BIT (RCC->APB1ENR, RCC_APB1ENR_TIM6EN ); //тактирование таймера 6
	CLEAR_REG(TIM6->CNT);
	WRITE_REG (TIM6->PSC, (uint16_t)(HAL_RCC_GetPCLK1Freq()/1000) - 1); //GetPCLK1Freq - частота шины таймера 6 						
	WRITE_REG (TIM6->ARR, 2*delay-1); //время задержки
	SET_BIT (TIM6->EGR, TIM_EGR_UG); //запись новых настроек в таймер 
	CLEAR_BIT (TIM6->SR, TIM_SR_UIF);// сброс флага обновления
	
	SET_BIT (TIM6->CR1, TIM_CR1_CEN); //включение таймера
	while (!READ_BIT(TIM6->SR, TIM_SR_UIF)) {} //ожидание установки флага обновления таймера 6
	CLEAR_BIT (TIM6->CR1, TIM_CR1_CEN);  //отключение таймера
	CLEAR_BIT (TIM6->SR, TIM_SR_UIF);
}

void delay_us(uint16_t delay)
{

	SET_BIT (RCC->APB1ENR, RCC_APB1ENR_TIM6EN ); //тактирование таймера 6
	CLEAR_REG(TIM6->CNT);
//	WRITE_REG (TIM6->PSC, (uint16_t)(HAL_RCC_GetPCLK1Freq()/1000000) - 1);  	
	WRITE_REG (TIM6->PSC, 47);  
	WRITE_REG (TIM6->ARR, 2*delay-1); //время задержки
	SET_BIT (TIM6->EGR, TIM_EGR_UG); //запись новых настроек в таймер 
	CLEAR_BIT (TIM6->SR, TIM_SR_UIF);// сброс флага обновления
	
	SET_BIT (TIM6->CR1, TIM_CR1_CEN); //включение таймера
	while (!READ_BIT(TIM6->SR, TIM_SR_UIF)) {} //ожидание установки флага обновления таймера 6
	CLEAR_BIT (TIM6->CR1, TIM_CR1_CEN);  //отключение таймера
	CLEAR_BIT (TIM6->SR, TIM_SR_UIF);
}
