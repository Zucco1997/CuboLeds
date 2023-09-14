/*
 * ramdonAD.c
 *
 *  Created on: 6 de set de 2023
 *      Author: Zucco
 */
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include "randomAD.h"
ADC_HandleTypeDef *hadcR;


void initRandom(ADC_HandleTypeDef *hadc)
{
	hadcR=hadc;
	seedRandom();
}

void seedRandom(void)
{
	uint16_t semente;
	HAL_ADC_Start(hadcR);
	semente=HAL_ADC_GetValue(hadcR);
	srand(semente);
}

uint16_t Random(uint16_t max)
{
	return (rand()%max);
}


uint16_t Randomf(uint16_t min, uint16_t max)
{
	uint16_t range=max-min+1;

	return (rand()%range+min);
}
