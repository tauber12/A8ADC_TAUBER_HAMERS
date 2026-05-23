/*
 * ADC.h
 *
 *  Created on: May 21, 2026
 *      Author: alexm
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "main.h"

extern volatile uint16_t adc_output;
extern volatile uint8_t conversion_ready;
extern volatile uint16_t adc_samples[20];

void ADC_init( void );
void ADC1_2_IRQHandler( void );

uint16_t calc_Max( volatile uint16_t arr[] );
uint16_t calc_Min( volatile uint16_t arr[] );
uint16_t calc_Average( volatile uint16_t arr[] );

#endif /* INC_ADC_H_ */
