/*
 * EE 329 A7 UART
 *******************************************************************************
 * @file           : lpuart1.h
 * @brief          : LPUART1 defines, externs, and function prototypes
 * project         : EE 329 S'26 A7
 * authors         : Alex Tauber, Tyler Ragasa
 * version         : 0.1
 * date            : 260503
 * compiler        : STM32CubeIDE v.1.19.0 Build: 14980_20230301_1550 (UTC)
 * target          : NUCLEO-L4A6ZG
 * clocks          : 4 MHz MSI to AHB2
 * @attention      : (c) 2026 STMicroelectronics.  All rights reserved.
 *******************************************************************************
 * PIN DEFINITIONS
 * LPUART1_PORT = GPIOG
 *******************************************************************************
 * Header format adapted from [Code Appendix by Kevin Vo] pg 5
 */

#ifndef INC_LPUART1_H_
#define INC_LPUART1_H_

#include "stm32l4xx_hal.h"
#include <stdio.h>

extern volatile uint8_t cursorRow;
extern volatile uint8_t cursorCol;
extern volatile uint8_t cursorUpdate;

#define LPUART1_PORT GPIOG

void INIT_Lpuart1( void );
void LPUART_Print( const char* message );
void LPUART_ESC_Print( const char* message );
void LPUART1_IRQHandler( void );
void Instruction_4( void );
void SetCharCenter( void );
void LPUART_PrintCharAt(uint8_t row, uint8_t col, char c);
void LPUART_PrintADC( uint16_t min, uint16_t max, uint16_t avg );
void GameSplashScreen( void );
void GameScreenClear( void );
void LPUART_PrintBorder( void );

#endif /* INC_LPUART1_H_ */
