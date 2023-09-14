/*
 * randomAD.h
 *
 *  Created on: 6 de set de 2023
 *      Author: Zucco
 */

#ifndef INC_RANDOMAD_H_
#define INC_RANDOMAD_H_

void initRandom(ADC_HandleTypeDef *hadc);
void seedRandom(void);
uint16_t Random(uint16_t max);
uint16_t Randomf(uint16_t min, uint16_t max);

#endif /* INC_RANDOMAD_H_ */
