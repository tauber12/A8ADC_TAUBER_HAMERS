/*
 *******************************************************************************
 * EE 329 A3 LCD TIMER
 *******************************************************************************
 * @file           : delay.h
 * @brief          : header file for delay.c; contains function prototypes for
 * SysTick timer initialization and microsecond delay; note: SysTick_Init()
 * disables HAL_Delay() as a side effect of repurposing SysTick for polling
 * project         : EE 329 S'26 A3
 * version         : 0.1
 * date            : 260415
 * compiler        : STM32CubeIDE v.1.19.0 Build: 14980_20230301_1550 (UTC)
 * target          : NUCLEO-L4A6ZG
 * clocks          : 4 MHz MSI to AHB2
 * @attention      : (c) 2026 STMicroelectronics.  All rights reserved.
 *******************************************************************************
 * Header format adapted from [Code Appendix by Kevin Vo] pg 5
 */

#ifndef SRC_DELAY_H_
#define SRC_DELAY_H_

#include "stm32l4xx_hal.h"

// delay.c function declarations

// initialize SysTick for delay_us polling
void SysTick_Init(void);
// blocking microsecond delay via SysTick
void delay_us(const uint32_t time_us);

#endif /* SRC_DELAY_H_ */
