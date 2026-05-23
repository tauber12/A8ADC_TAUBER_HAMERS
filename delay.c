/*
 * EE 329 A3 LCD TIMER
 *******************************************************************************
 * @file           : delay.c
 * @brief          : SysTick timer configuration and microsecond delay function
 * project         : EE 329 S'26 A3
 * version         : 0.1
 * date            : 260415
 * compiler        : STM32CubeIDE v.1.19.0 Build: 14980_20230301_1550 (UTC)
 * target          : NUCLEO-L4A6ZG
 * clocks          : 4 MHz MSI to AHB2
 * @attention      : (c) 2026 STMicroelectronics.  All rights reserved.
 *******************************************************************************
 * DELAY PLAN :
 * configure SysTick timer to use CPU clock as source with interrupts disabled;
 * implement blocking microsecond delay by loading SysTick LOAD register with
 * cycle count derived from SystemCoreClock, clearing VAL and COUNTFLAG, then
 * polling COUNTFLAG until countdown completes.
 * warning: SysTick_Init() disables SysTick interrupt, breaking HAL_Delay()
 * which relies on SysTick interrupt to increment its tick counter. Do not use
 * HAL_Delay() after calling SysTick_Init().
 * note: do not call delay_us(0) - results in maximum possible delay.
 * note: small values produce longer delays than specified due to function
 *       overhead (e.g. @4MHz, delay_us(1) = 10-15us actual delay).
 *******************************************************************************
 * Header format adapted from [Code Appendix by Kevin Vo] pg 5
 * Functions used from Lab Manual A3
 */

#include "delay.h"

/*-----------------------------------------------------------------------------
 * function : SysTick_Init( )
 * INs      : none
 * OUTs     : none
 * action   : enables SysTick timer using CPU clock (HCLK) as source;
 *            disables SysTick interrupt to allow polling via COUNTFLAG;
 *            must be called before any delay_us() call
 * version  : 0.1
 * date     : 260415
 * usage    : called once in LCD_init() before any delay_us() invocations;
 *            note: disables HAL_Delay() as a side effect
 *----------------------------------------------------------------------------*/
void SysTick_Init(void) {

	SysTick->CTRL |=  (SysTick_CTRL_ENABLE_Msk |    // enable SysTick timer
	                   SysTick_CTRL_CLKSOURCE_Msk);  // select CPU clock (HCLK)
	SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk);   // disable interrupt; use COUNTFLAG polling

}

/*-----------------------------------------------------------------------------
 * function : delay_us( )
 * INs      : time_us - delay duration in microseconds (do not pass 0)
 * OUTs     : none
 * action   : converts time_us to CPU clock cycles using SystemCoreClock;
 *            loads cycle count into SysTick LOAD register; clears VAL and
 *            COUNTFLAG; blocks until COUNTFLAG set (countdown reaches zero)
 *            formula: LOAD = (time_us * (SystemCoreClock / 1000000)) - 1
 * version  : 0.1
 * date     : 260415
 * usage    : called throughout lcd.c and main.c for timing delays;
 *            @4MHz: delay_us(1) yields ~10-15us due to function overhead
 *----------------------------------------------------------------------------*/
void delay_us(const uint32_t time_us) {

	/* load cycle count for requested delay duration
	 * SystemCoreClock / 1000000 = cycles per microsecond
	 * -1 accounts for zero-indexed countdown (counts LOAD down to 0 inclusive) */
	SysTick->LOAD = (uint32_t)((time_us * (SystemCoreClock / 1000000)) - 1);
	// clear current timer count
	SysTick->VAL   = 0;
	// clear stale COUNTFLAG from previous call
	SysTick->CTRL &= ~(SysTick_CTRL_COUNTFLAG_Msk);
	// poll until countdown reaches zero
	while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));

}
