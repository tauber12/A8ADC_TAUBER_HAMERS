/*
 * EE 329 A7 UART
 *******************************************************************************
 * @file           : lpuart1.c
 * @brief          : LPUART1 initialization, print utilities, and terminal game
 * project         : EE 329 S'26 A7
 * authors         : Alex Tauber, Tyler Ragasa
 * version         : 0.1
 * date            : 260503
 * compiler        : STM32CubeIDE v.1.19.0 Build: 14980_20230301_1550 (UTC)
 * target          : NUCLEO-L4A6ZG
 * clocks          : 4 MHz MSI to AHB2
 * @attention      : (c) 2026 STMicroelectronics.  All rights reserved.
 *******************************************************************************
 * LPUART1 PLAN :
 * configure PG7 (TX) and PG8 (RX) as alternate function 8 for LPUART1;
 * initialize LPUART1 at 115200 baud from 4 MHz MSI clock; enable RX interrupt
 * for arrow-key ESC sequence parsing via 3-state FSM; provide print utilities
 * for raw strings, ESC code sequences, and cursor-positioned characters;
 * implement terminal game: splash screen, wipe animation, bordered play field,
 * and arrow-key-driven cursor movement with boundary wrapping.
 *******************************************************************************
 * LPUART1 WIRING
 *      peripheral – Nucleo I/O
 * LPUART1 TX - PG7 = CN10-17 - OUT
 * LPUART1 RX - PG8 = CN10-15 - IN
 *******************************************************************************
 * PIN DEFINITIONS (see lpuart1.h)
 * LPUART1_PORT = GPIOG
 *******************************************************************************
 * Header format adapted from [Code Appendix by Kevin Vo] pg 5
 */
#include "lpuart1.h"

volatile uint8_t cursorRow = 12;
volatile uint8_t cursorCol = 40;
volatile uint8_t cursorUpdate = 0;

/*-----------------------------------------------------------------------------
 * function : INIT_Lpuart1( )
 * INs      : N/A
 * OUTs     : N/A
 * action   : Initializes LPUART1 virtual coms ports PG7(TX) and PG8(RX)
 *            Configures LPUART1 via CR1
 * authors  : Alex Tauber
 * usage    : main.c: main()
 *----------------------------------------------------------------------------*/
void INIT_Lpuart1( void ){

	PWR->CR2 |= (PWR_CR2_IOSV);              // power avail on PG[15:2] (LPUART1)
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOGEN);   // enable GPIOG clock
	RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN; // enable LPUART clock bridge

	// PG7 (TX) and PG8 (RX) initialization for LPUART functionality
	// alternate function mode 2b'10
	LPUART1_PORT->MODER &= ~(GPIO_MODER_MODE7 | GPIO_MODER_MODE8);
	LPUART1_PORT->MODER |= (GPIO_MODER_MODE7_1 | GPIO_MODER_MODE8_1);

	// tx no pull-up/pull-down, rx pull-up
	LPUART1_PORT->PUPDR &= ~(GPIO_PUPDR_PUPD7 | GPIO_PUPDR_PUPD8);
	LPUART1_PORT->PUPDR |= (GPIO_PUPDR_PUPD8_0);

	// push-pull
	LPUART1_PORT->OTYPER &= ~(GPIO_OTYPER_OT7 | GPIO_OTYPER_OT8);

	// low output speed configured - fine for 115.2 kbps baud
	LPUART1_PORT->OSPEEDR &= ~(0x3U << GPIO_OSPEEDR_OSPEED7_Pos |
										  0x3U << GPIO_OSPEEDR_OSPEED8_Pos);

	// configure alternate function 8 for PG7 and PG8 LPUART1_TX & LPUART1_RC
	LPUART1_PORT->AFR[0] &= ~(0xFU << 4*7);
	LPUART1_PORT->AFR[0] |= (0x8U << 4*7);
	LPUART1_PORT->AFR[1] &= ~(0xFU);
	LPUART1_PORT->AFR[1] |= (0x8U);

	//set 115.2kbps baud rate with 4MHz clk

	LPUART1->BRR = (256UL * 4000000UL) / 115200UL;

	LPUART1->CR1 &= ~(USART_CR1_M1 | USART_CR1_M0); // 8-bit data
	LPUART1->CR1 |= USART_CR1_UE;                   // enable LPUART1
	LPUART1->CR1 |= (USART_CR1_TE | USART_CR1_RE);  // enable xmit & recv
	LPUART1->CR1 |= USART_CR1_RXNEIE;        // enable LPUART1 recv interrupt
	LPUART1->ISR &= ~(USART_ISR_RXNE);       // clear Recv-Not-Empty flag


	NVIC->ISER[2] = (1 << (LPUART1_IRQn & 0x1F));   // enable LPUART1 ISR
	__enable_irq();                          // enable global interrupts

}

/*-----------------------------------------------------------------------------
 * function : LPUART_Print( )
 * INs      : String of characters (inc. string-terminating NULL)
 * OUTs     : N/A
 * action   : Takes string input and transmits each character via TDR
 * authors  : Alex Tauber
 * usage    : lpuart1.c: LPUART_ESC_Print(), Instruction_4(),
 *            SetCharCenter(), LPUART_PrintCharAt()
 *----------------------------------------------------------------------------*/
void LPUART_Print( const char* message ) {
   uint16_t iStrIdx = 0;
   while ( message[iStrIdx] != 0 ) {
      while(!(LPUART1->ISR & USART_ISR_TXE)) // wait for empty xmit buffer
         ;
      LPUART1->TDR = message[iStrIdx];       // send this character
	iStrIdx++;                             // advance index to next char
   }
}

/*-----------------------------------------------------------------------------
 * function : LPUART_ESC_Print( )
 * INs      : String of characters (inc. string-terminating NULL)
 * OUTs     : N/A
 * action   : Takes string input and transmits ESC codes
 * authors  : Alex Tauber
 * usage    : lpuart1.c: LPUART_IRQHandler(), Instruction_4(),
 *            GameSplashScreen(), LPUART_PrintBorder()
 *            main.c: main()
 *----------------------------------------------------------------------------*/
void LPUART_ESC_Print( const char* message ){

	while( !(LPUART1->ISR & USART_ISR_TXE) );
	LPUART1->TDR = 0x1B;
	LPUART_Print(message);

}

/*-----------------------------------------------------------------------------
 * function : LPUART1_IRQHandler( ) - Game Version
 * INs      : N/A
 * OUTs     : updates cursorRow and cursorCol variable
 * 			  depending on user input
 * action   :
 * authors  : Alex Tauber
 * usage    : main.c: main()
 *----------------------------------------------------------------------------*/

void LPUART1_IRQHandler( void ) {

	// hold previous state **must have static declaration
	static uint8_t escState = 0;
	// current char received on rx
   uint8_t charRecv;

   if ( LPUART1->ISR & USART_ISR_RXNE ) {
      charRecv = LPUART1->RDR;

      // use state machine to check if any arrow key pressed
      switch ( escState ){
			case 0: // Anticipating ESC code
				escState = (charRecv == 0x1B) ? 1:0;
				break;
			case 1: //
				escState = (charRecv == '[') ? 2:0;
				break;
			case 2:
				// Clear character @ current pos before moving. 'O' -> ' '
				LPUART_PrintCharAt(cursorRow,cursorCol, ' ');
				switch (charRecv){
					//up: if @ top border go to bottom else move up 1
					case 'A': cursorRow = (cursorRow == 2) ? 23 : cursorRow - 1;
					break;
					//down: if @ bottom border go to top else move down 1
					case 'B': cursorRow = (cursorRow == 23) ? 2 : cursorRow + 1;
					break;
					//right: if @ right border go to left else move right 1
					case 'C': cursorCol = (cursorCol == 79) ? 2 : cursorCol + 1;
					break;
					//left: if @ left border go to right, else move left 1
					case 'D': cursorCol = (cursorCol == 2) ? 79 : cursorCol - 1;
					break;
				}
				cursorUpdate = 1; // set flag
				escState = 0;
				break;
      }

    }

}

/*-----------------------------------------------------------------------------
 * function : LPUART1_IRQHandler( ) - Echo Characters Version
 * INs      :
 * OUTs     :
 * action   : Uses ESC codes to send char to and from (echo) Nucleo. Once the
 *            terminal recieves char, ESC code sets appropriate char color
 * authors  : Alex Tauber
 * usage    : main.c: main()
 *----------------------------------------------------------------------------*/
/*
void LPUART1_IRQHandler( void ) {
   uint8_t charRecv;
   if ( LPUART1->ISR & USART_ISR_RXNE ) {
      charRecv = LPUART1->RDR;
      switch ( charRecv ) {

			case 'R': LPUART_ESC_Print("[31m");  break;
			case 'G': LPUART_ESC_Print("[32m");  break;
			case 'B': LPUART_ESC_Print("[34m");  break;
			case 'W': LPUART_ESC_Print("[37m");   break;

	   default:
	      while( !(LPUART1->ISR & USART_ISR_TXE) ); // wait for empty TX buffer
	      LPUART1->TDR = charRecv;  // echo char to terminal
	    }
   }
}
*/
/*-----------------------------------------------------------------------------
 * function : Instruction_4( )
 * INs      : N/A
 * OUTs     : N/A
 * action   : Executes series of ESC codes and Print to terminal per EE329
 *            lab manual Instruction 4 - "Use VT100 Escape Codes"
 * authors  : Alex Tauber
 * usage    : main.c: main()
 *----------------------------------------------------------------------------*/
void Instruction_4( void ){
	LPUART_ESC_Print("[2J"); // Clear entire screen
	LPUART_ESC_Print("[3B"); // Cursor down 3 lines
	LPUART_ESC_Print("[5C"); // Cursor right 5 spaces
	LPUART_Print("All good students read the");
	LPUART_ESC_Print("[1B"); // Cursor down 1 line
	LPUART_ESC_Print("[21D"); // Cursor left 21 spaces
	LPUART_ESC_Print("[5m"); // Change text to blinking mode
	LPUART_Print("Reference Manual");
	LPUART_ESC_Print("[H"); // Cursor top left
	LPUART_ESC_Print("[0m"); // Remove atributes
	LPUART_Print("Input: ");
}

/*-----------------------------------------------------------------------------
 * function : SetCharCenter()
 * INs      : N/A
 * OUTs     : N/A
 * action   : Helper function used while developing game
 *            sets character at center of screen (terminal)
 * authors  : Alex Tauber
 * usage    : N/A
 *----------------------------------------------------------------------------*/
void SetCharCenter( void ){
	cursorRow = 12;
	cursorCol = 40;
	LPUART_Print("\x1B[12;40H");  // move cursor to center
	LPUART_Print("o");            // print your character
}


/*-----------------------------------------------------------------------------
 * function : LPUART_PrintCharAt( )
 * INs      : Terminal {Row, Col} and Char to print
 * OUTs     : N/A
 * action   : Prints Char at {Row, Col}
 * authors  : Alex Tauber
 * usage    : lpuart1.c: LPUART1_IRQHandler(), GameSplashScreen(),
 *            GameScreenClear(), LPuART_PrintBorder()
 *----------------------------------------------------------------------------*/
void LPUART_PrintCharAt(uint8_t row, uint8_t col, char c) {
    char buf[16];
	// Equivalently: "ESC[{row};{col}H"+"{c}"
    sprintf(buf, "\x1B[%d;%dH%c", row, col, c);
    LPUART_Print(buf);
}

void LPUART_PrintADC( uint16_t min, uint16_t max, uint16_t avg ){

    char buf[2] = {'\0', '\0'};

    LPUART_ESC_Print("[2J");    // clear screen
    LPUART_ESC_Print("[1;1H");
    LPUART_Print("ADC counts volts");

    // min row
    LPUART_ESC_Print("[2;1H"); // start cursor on 2nd row
    LPUART_Print("MIN  ");

    buf[0] = (min / 1000)      + '0';  LPUART_Print(buf);
    buf[0] = (min / 100) % 10  + '0';  LPUART_Print(buf);
    buf[0] = (min / 10)  % 10  + '0';  LPUART_Print(buf);
    buf[0] =  min        % 10  + '0';  LPUART_Print(buf);
    LPUART_Print("  ");

    uint16_t mv = (uint16_t)((float)min / 4095.0f * 3300.0f); //convert to voltage

    buf[0] =  mv / 1000        + '0';  LPUART_Print(buf);
    LPUART_Print(".");
    buf[0] = (mv / 100) % 10   + '0';  LPUART_Print(buf);
    buf[0] = (mv / 10)  % 10   + '0';  LPUART_Print(buf);
    buf[0] =  mv        % 10   + '0';  LPUART_Print(buf);
    LPUART_Print(" V");

    // max row
    LPUART_ESC_Print("[3;1H"); // start cursor on 3rd row
    LPUART_Print("MAX  ");

    buf[0] = (max / 1000)      + '0';  LPUART_Print(buf);
    buf[0] = (max / 100) % 10  + '0';  LPUART_Print(buf);
    buf[0] = (max / 10)  % 10  + '0';  LPUART_Print(buf);
    buf[0] =  max        % 10  + '0';  LPUART_Print(buf);
    LPUART_Print("  ");

    mv = (uint16_t)((float)max / 4095.0f * 3300.0f); //convert to voltage

    buf[0] =  mv / 1000        + '0';  LPUART_Print(buf);
    LPUART_Print(".");
    buf[0] = (mv / 100) % 10   + '0';  LPUART_Print(buf);
    buf[0] = (mv / 10)  % 10   + '0';  LPUART_Print(buf);
    buf[0] =  mv        % 10   + '0';  LPUART_Print(buf);
    LPUART_Print(" V");

    // avg row
    LPUART_ESC_Print("[4;1H"); // start cursor on 4th row
    LPUART_Print("AVG  ");

    buf[0] = (avg / 1000)      + '0';  LPUART_Print(buf);
    buf[0] = (avg / 100) % 10  + '0';  LPUART_Print(buf);
    buf[0] = (avg / 10)  % 10  + '0';  LPUART_Print(buf);
    buf[0] =  avg        % 10  + '0';  LPUART_Print(buf);
    LPUART_Print("  ");

    mv = (uint16_t)((float)avg / 4095.0f * 3300.0f); //convert to voltage

    buf[0] =  mv / 1000        + '0';  LPUART_Print(buf);
    LPUART_Print(".");
    buf[0] = (mv / 100) % 10   + '0';  LPUART_Print(buf);
    buf[0] = (mv / 10)  % 10   + '0';  LPUART_Print(buf);
    buf[0] =  mv        % 10   + '0';  LPUART_Print(buf);
    LPUART_Print(" V");
}

/*-----------------------------------------------------------------------------
 * function : GameSplashScreen( )
 * INs      : N/A
 * OUTs     : N/A
 * action   : Displays splash screen for game (Designed using textpaint.com)
 * authors  : Alex Tauber, Tyler Ragasa
 * usage    : main.c: main()
 *----------------------------------------------------------------------------*/
void GameSplashScreen( void ) {
    const char *art[] = { // Splash screen art
"********************************************************************************",
"*                                                                              *",
"*                                                                              *",
"*                    __  __  _____  _  _  ____  _  _   ___                     *",
"*                   (  \/  )(  _  )( \/ )(_  _)( \( ) / __)                    *",
"*                    )    (  )(_)(  \  /  _)(_  )  ( ( (_-.                    *",
"*                   (_/\/\_)(_____)  \/  (____)(_)\_) \___/                    *",
"*                                                                              *",
"*                                    _____                                     *",
"*                                   (  _  )                                    *",
"*                                    )(_)(                                     *",
"*                                   (_____)                                    *",
"*                                                                              *",
"*                     __    ____  _____  __  __  _  _  ____                    *",
"*                    /__\  (  _ \(  _  )(  )(  )( \( )(  _ \                   *",
"*                   /(  )\  )   / )(_)(  )(__)(  )  (  )(_) )                  *",
"*                  (__)(__)(_)\_)(_____)(______)(_)\_)(____/                   *",
"*                                                                              *",
"*                                                                              *",
"*                                                                              *",
"*                           Game Loading ..........                            *",
"*                                                                              *",
"*                                                                              *",
"********************************************************************************"
    };

    int row, col;

    LPUART_ESC_Print("[2J"); // Clear screen
    LPUART_ESC_Print("[H"); // Cursor top left

    for (row = 0; row < 24; row++) {
        for (col = 0; col < 80; col++) {
            LPUART_PrintCharAt(row + 1, col + 1, art[row][col]);
        }
    }

}

/*-----------------------------------------------------------------------------
 * function : GameScreenClear( )
 * INs      : N/A
 * OUTs     : N/A
 * action   : "Animation" between GameSplashScreen and running game.
 *            Fills screen top to bottom, left to right with '#'
 * authors  : Alex Tauber
 * usage    : main.c: main()
 *----------------------------------------------------------------------------*/
void GameScreenClear( void ){
	int row, col;
	for (row = 0; row < 24; row++) {
	        for (col = 0; col < 80; col++) {
	            LPUART_PrintCharAt(row + 1, col + 1, '#');
	        }
	    }
}

/*-----------------------------------------------------------------------------
 * function : LPUART_PrintBorder( )
 * INs      : N/A
 * OUTs     : N/A
 * action   : Displays border to outline "gameplay area"
 * authors  : Tyler Ragasa :>
 * usage    : main.c: main()
 *----------------------------------------------------------------------------*/
void LPUART_PrintBorder( void ){
	int rowMax = 24;
	int colMax = 80;

	// CLEAR SCREEN
	LPUART_ESC_Print("[2J");

	// T/B ROW BORDER
	LPUART_ESC_Print("[H"); //MOVE CURSOR TO UPPER LEFT CORNER
	for(int iCol = 1; iCol <= colMax; iCol++){
		LPUART_PrintCharAt(1, iCol, '-'); // PRINT TOP ROW
		LPUART_PrintCharAt(rowMax, iCol, '-'); // PRINT BOTTOM ROW
	}

	// L/R SIDE BORDER
	for(int iRow = 2; iRow <= (rowMax - 1); iRow++){
		LPUART_PrintCharAt(iRow, 1, '|'); // PRINT LEFT COL
		LPUART_PrintCharAt(iRow, colMax, '|'); // PRINT RIGHT COL
	}
}

