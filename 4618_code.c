/*------------------------------------------------------------------------------
 * Function:    Morse Code Communication Platform (MPS430Fg4618 code)
 * Description: Using the MSP-EXP430FG4618 Development Tool establish a data
 *              exchange between the MSP430FG4618 and MSP430F2013 devices using
 *              the I2C mode. The MSP430FG4618 uses the USCI module while the
 *              MSP430F2013 uses the USI module. MSP430FG4618 communicates with
 *              PC via RS232 module using USCI Serial Communication peripheral
 *              interface. This program takes user prompts to input a
 *              message that gets converted to morse.
 *				This program also handles incoming characters from the capacitive
 *				pad (handled by MSP430F2013).
 *
 * Input:       Serial terminal, SW1, SW2
 * Output:      Serial terminal, LED2, Buzzer
 * Author:		David Tougaw
 *------------------------------------------------------------------------------*/

#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include <msp430xG46x.h>
#include <definitions.h>

#define FALSE 0
#define TRUE (!FALSE)

#define SW2 BIT1&P1IN
#define SW1 BIT0&P1IN

char isSending = 0;

#define maxCharacters 25

unsigned long int id;
char pos =0;
char released = FALSE;
char INCOMING_STATE = FALSE;

#define dotDelay for (id=0;id<10000 ; id++)		// 0.1 sec delay
#define lineDelay for (id=0;id<30000 ; id++)	// 0.3 sec delay

char morseCode = 0;

char touchMsg [5]= {0};
char inputMsg [maxCharacters] = {0};

//First blank, and the hex codes, which all display quite
//well on a 7-segment display.
#define CHAR_SPACE          0
#define CHAR_ALL            (SEG_a|SEG_b|SEG_c|SEG_d|SEG_e|SEG_f|SEG_g|SEG_h)
#define CHAR_0              (SEG_a|SEG_b|SEG_c|SEG_d|SEG_e|SEG_f)
#define CHAR_1              (SEG_b|SEG_c)
#define CHAR_2              (SEG_a|SEG_b|SEG_d|SEG_e|SEG_g)
#define CHAR_3              (SEG_a|SEG_b|SEG_c|SEG_d|SEG_g)
#define CHAR_4              (SEG_b|SEG_c|SEG_f|SEG_g)
#define CHAR_5              (SEG_a|SEG_c|SEG_d|SEG_f|SEG_g)
#define CHAR_6              (SEG_a|SEG_c|SEG_d|SEG_e|SEG_f|SEG_g)
#define CHAR_7              (SEG_a|SEG_b|SEG_c)
#define CHAR_8              (SEG_a|SEG_b|SEG_c|SEG_d|SEG_e|SEG_f|SEG_g)
#define CHAR_9              (SEG_a|SEG_b|SEG_c|SEG_d|SEG_f|SEG_g)
#define CHAR_A              (SEG_a|SEG_b|SEG_c|SEG_e|SEG_f|SEG_g)
#define CHAR_B              (SEG_c|SEG_d|SEG_e|SEG_f|SEG_g)
#define CHAR_C              (SEG_a|SEG_d|SEG_e|SEG_f)
#define CHAR_D              (SEG_b|SEG_c|SEG_d|SEG_e|SEG_g)
#define CHAR_E              (SEG_a|SEG_d|SEG_e|SEG_f|SEG_g)
#define CHAR_F              (SEG_a|SEG_e|SEG_f|SEG_g)
#define CHAR_MINUS          (SEG_g)

#define SEG_a       0x01
#define SEG_b       0x02
#define SEG_c       0x04
#define SEG_d       0x08
#define SEG_e       0x40
#define SEG_f       0x10
#define SEG_g       0x20
#define SEG_h       0x80

const uint8_t lcd_digit_table[18] =
{
    CHAR_0,
    CHAR_1,
    CHAR_2,
    CHAR_3,
    CHAR_4,
    CHAR_5,
    CHAR_6,
    CHAR_7,
    CHAR_8,
    CHAR_9,
    CHAR_A,
    CHAR_B,
    CHAR_C,
    CHAR_D,
    CHAR_E,
    CHAR_F,
    CHAR_MINUS,
    CHAR_SPACE
};

void init_lcd(void)
{
    int i;

    /* Basic timer setup */
    /* Set ticker to 32768/(256*256) */
    BTCTL = BT_fCLK2_DIV128 | BT_fCLK2_ACLK_DIV256;

    for (i = 0;  i < 20;  i++)
        LCDMEM[i] = 0;

    /* Turn on the COM0-COM3 and R03-R33 pins */
    P5SEL |= (BIT4 | BIT3 | BIT2);
    P5DIR |= (BIT4 | BIT3 | BIT2);

    /* LCD-A controller setup */
    LCDACTL = LCDFREQ_128 | LCD4MUX | LCDSON | LCDON;
    LCDAPCTL0 = LCDS0 | LCDS4 | LCDS8 | LCDS12 | LCDS16 | LCDS20 | LCDS24;
    LCDAPCTL1 = 0;
    LCDAVCTL0 = LCDCPEN;
    LCDAVCTL1 = 1 << 1;
}

void LCDchar(int ch, int pos)
{
    /* Put a segment pattern at a specified position on the LCD display */
    LCDMEM[9 - pos] = ch;
}

void LCDdigit(uint16_t val, int pos)
{
    LCDchar(lcd_digit_table[val], pos);
}

void LCDdec(uint16_t val, int pos)
{
    int i;

    for (i = 2;  i >= 0;  i--)
    {
        LCDchar(lcd_digit_table[val%10], pos + i);
        val /= 10;
    }
}

void configure_uart_usci0(void)
{
    /* Configure USCI0A as a UART */

    /* Configure the port with the reset bit held high */
    UCA0CTL1 |= UCSWRST;

    P2SEL |= (BIT4 | BIT5);

    UCA0CTL0 = 0;                   /* 8-bit character */
    UCA0CTL1 = UCSSEL_2;            /* SMCLK */
    /* Set the bit rate registers for 115200 baud */
    UCA0BR0 = 9;
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS_1;
    IE2 |= UCA0RXIE;               // Enable USCI_A0 RX interrupt
    UCA0STAT = 0;
    UCA0TXBUF = 0;

    UCA0CTL1 &= ~UCSWRST;           // **Initialize USCI state machine**
}

void configure_i2c_usci0(void)
{
    /* Configure USCI0B as an I2C port */
    P3SEL |= (BIT2 | BIT1);
    UCB0CTL1 |= UCSWRST;                    /* Enable SW reset */

    UCB0CTL0 = UCMODE_3 | UCSYNC;           /* I2C Slave, synchronous mode */
    UCB0I2COA = 0x48;                       /* Own Address */
    UCB0CTL1 &= ~UCSWRST;                   /* Clear SW reset, resume operation */
    UCB0I2CIE |= (UCSTPIE | UCSTTIE);       /* Enable STT and STP interrupt */
    IE2 |= UCB0RXIE;                        /* Enable RX interrupt */
}

uint8_t xxx = 0;

void send_DMA( char * char_arr, int size)
{
    DMACTL0 = DMA0TSEL_4;               // DMAREQ, software trigger, TX is ready
    DMA0SA = (int)char_arr;             // Source block address
    DMA0DA = (int)&UCA0TXBUF;           // Destination single address
    DMA0SZ = size;                      // Length of the String
    DMA0CTL = DMASRCINCR_3 + DMASBDB + DMALEVEL; // Increment src address
    DMA0CTL |= DMAIE;                   // Interrupt Enable DMA

    DMA0CTL |= DMAEN;                   // Enable DMA transfer
}

void sendTitle ()                       // Send Morse Code Char Title using DMA
{
    send_DMA(title0, sizeof(title0));
}

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               /* Stop watchdog */

    configure_uart_usci0();
    configure_i2c_usci0();
    init_lcd();

    sendTitle();					// Send title

    // Buzzer setup
    P3SEL |= BIT5;                  // P3 BIT 5 set to TB4
    TB0CCTL4 = OUTMOD_4;            // TB0 output is in toggle mode
    TB0CTL = TBSSEL_2 + MC_1;       // SMCLK  is clock source, UP mode (ticks +1)
    TB0CCR0 = 149;                  // f = 3500Hz
    TB0CCR4 = 149;

    // debug
    P5DIR |= BIT1;                  // toggle

    //------------------
    P2DIR |= BIT2;                  // led2 dir output

    // TIMERA
    timerASetUp();

    // WDT  Setup and Start
    IE1 |= WDTIE;                   // Enable WDT interrupt
    P1IE |= BIT0+BIT1;              // P1.0,P1.1 interrupt enabled (SW1,SW2)

    _EINT();                        // Enable Global Interrupt


    for (;;)
    {
#if 1
        /* Normal operation */
        LPM0;
        LCDdec(xxx, 3);

#else
        Checking out the host interface
        UCA0TXBUF = xxx++;
        {
          long int i;
          for (i = 0;  i < 10000;  i++)
              _NOP();
        }
#endif
    }
}

void setMorseCode(char * c, int size)
{
    // morseCode Op-Code
    // BYTE [XXX43210]
    // XXX is LENGTH of letter in . & _
    // 43210 is msg, order of execution from BIT4 to BIT4 + LENGTH-1
    // BIT at 0 if DOT
    // BIT at 1 if LINE
    char k;
    char p = BIT4;
    for(k=0; k < size-1; k++)
    {
        if(c[k] == '_')
            morseCode |= (BIT4 >> k);     // set bit if LINE, Shift right by K
    }
    switch (size-1)                       // Skip NULL END char
    {  // Set Bits for size
        case 1:morseCode |=BIT5; break;
        case 2:morseCode |=BIT6; break;
        case 3:morseCode |=BIT5+BIT6; break;
        case 4:morseCode |=BIT7; break;
        case 5:morseCode |=BIT7+BIT5; break;
    }
}

void morseToLetter (char p)
{
    char found = FALSE;
    char i=0;
    int size = sizeof(MC_chars);
    while(i< size && found==FALSE)
    {
        char mc_size = MC_code_size[i];
        if(mc_size == (p+1))                            // Check Size of MorseCode to narrow down characters
        {
            if(strcmp(MC_code[i],touchMsg)==0)          // Check if MorseCode[i] == received MorseCode[i]
            {
                send_DMA(incomingChar,sizeof(incomingChar));
                found = TRUE;							// Stop searching if FOUND
                while(!(IFG2&UCA0TXIFG));               // Wait until TXBUF is free
                UCA0TXBUF = MC_chars[i];                // Send Corresponding Character
            }
        }
        i++;
    }
    if(found == 0)
        send_DMA(specChar,sizeof(specChar));            // MSG: char not in MORSE CODE Table
    // clear touchMSg array
    for (i=0; i<5;i++)
    {
        touchMsg[i] = 0;
    }

    // Enable Transmission through Serial terminal
    isSending = FALSE;

}

void letterToMorse (char c)
{
    c = tolower(c);
    int size = sizeof(MC_chars);
    char i;
    for(i=0;i< size;i++)
    {
        if(c == MC_chars[i])                            // find c in Morse Code Chars Array
        {
            setMorseCode(MC_code[i],MC_code_size[i]);   // Create Morse Code Op-Code Byte
            break;
        }
    }
}

// start WDT to play MorseCode msg
void playMorseCode ()
{
    WDTCTL = WDT_ADLY_250;              		// 250ms interval (default)
}

// Plays morse code on Buzzer
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    char k = 0;
    send_DMA(sending,sizeof(sending));          // Print "Sending Morse Code..."
    while(inputMsg[k] != 0)                     // Parse Every Char of the message
    {
        letterToMorse(inputMsg[k]);             // Initiate letterToMorseConversion
        char size = morseCode >> 5;             // Shift 5 bits to get size, Last MSB bits
        int i;
        for (i=0; i<size ; i++)
        {
            P3DIR |= BIT5;                      // Buzzer on
            P5OUT |= BIT1;                      // Led4 on

            if(morseCode & (BIT4 >> i))         // Check msg bits from BIT4 to BIT0
                lineDelay;                      // Line Delay
            else
                dotDelay;                       // Dot Delay
            id=0;
            P5OUT &= ~BIT1;                     // Led4 off
            P3DIR &= ~BIT5;                     // Led4 off
            // space between letter parts
            dotDelay;

        }
        send_DMA(&progress,sizeof(progress));   // Send Progress #

        // space between two letters
        dotDelay;
        dotDelay;
        k++;
        // clear current morse letter
        morseCode=0;
    }
    resetMorseBuzzer();                         // reset
}

void resetMorseBuzzer()
{
    P5OUT &= ~BIT1;                     // STOP Buzzer
    P3DIR &= ~BIT5;                     // clear Led4
    morseCode=0;
    isSending = FALSE;
    char k=0;
    // clear input message
    while(inputMsg[k] != 0)
    {
        inputMsg[k] = 0;
        k++;
    }
    send_DMA(NLprompt,sizeof(NLprompt));
    WDTCTL = WDTPW | WDTHOLD;
}

#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
    xxx = UCB0RXBUF;                    // Get Value from MSP 2013 i2C

    //xxx = UCA0TXBUF = UCB0RXBUF;
    if (xxx == 255)						// Capacitive Pad Pressed
    {
        released = FALSE;
        P3DIR |= BIT5;					// Buzzer dir output (ON)
        P2OUT |= BIT2;					// LED2 ON
        startTimerA();
    }
    else if (xxx == 0)					// Capacitive Pad Released
    {
        released = TRUE;
        P3DIR &= ~BIT5;					// Buzzer dir input (OFF)
        P2OUT &=~BIT2;					// LED2 OFF
    }
}

#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
{
    if (IFG2&UCA0RXIFG && isSending == FALSE)           // If NOT SENDING and UCASCI Module
    {
        char c = UCA0RXBUF;                             // get char
        IFG2 &=~UCA0RXIFG;                              // CLEAR UCA0RXIFG
        while(!(IFG2&UCA0TXIFG));                       // wait for TX to be free
        UCA0TXBUF = c;                                  // echo char
        if (isalnum(c) || c==8 || c==13)                // is it alpha or Number or Enter or Backspace
        {
            if(pos < maxCharacters-1)
            {
                if(c==8)                                // backspace handling
                {
                    if(pos >0)
                    {
                        pos--;
                        inputMsg[pos] = 0;
                        while(!(IFG2&UCA0TXIFG));       // Wait until TXBUF is free
                        UCA0TXBUF = ' ';                // Sprint Space
                        while(!(IFG2&UCA0TXIFG));       // Wait until TXBUF is free
                        UCA0TXBUF = 8;                  // Print backSpace
                    }
                    else                                // Don't delete other chars other than the one inserted
                    {
                        while(!(IFG2&UCA0TXIFG));       // Wait until TXBUF is free
                        UCA0TXBUF = ' ';                // TXBUF <= RXBUF (echo)
                    }
                }
                else
                {
                    if(c != 13)							// if not Carrage Return
                    {
                        inputMsg[pos] = c;              // Store Char in inputMsg array
                        pos++;
                    }
                    else                                // carriage return (ENTER))
                    {
                        pos=0;
                        isSending = TRUE;               // Don't allow insertion through terminal
                        playMorseCode();                // Play Morse Code WDtimer

                    }
                }
            }
            else                                        
            {// HANDLE OVERFLOW Maximum chars inserted, send to Morse
                pos=0;
                isSending = TRUE;
                send_DMA(newLine,sizeof(newLine));
                playMorseCode();
            }
        }
        else
        {
            send_DMA(specChar,sizeof(specChar));        // Error message if char not in MorseCode
        }
    }

    UCB0STAT &= ~(UCSTPIFG | UCSTTIFG);
    LPM0_EXIT;
}

void startTimerA()							// Check Capacitive Pad State every 0.1 sec using Interrupt
{
    TACCTL0 |= CCIE;                        // interrupt enabled on CC0
    TACTL |= MC_1;                          // Up Mode
}

void timerASetUp()
{
    // TIMERA
    TACTL |= MC_0 + TASSEL_2 + ID_3;        // STOP MODE, Interrupt Enabled, SMCLK, f = SMCLK/8 = 131072Hz
    TACCR0 = 13106;                         // .1s
}


#pragma vector=TIMERA0_VECTOR                   // Interrupt triggered every .1sec
__interrupt void TA0_ISR(void)
{
    int static k = 0;
    INCOMING_STATE = TRUE;
    isSending = TRUE;
    if (released == TRUE)                       // capacitive button
    {
        if(k>0 && k <=4 )                       // 0-0.5 sec
        {
            touchMsg[pos]= DOT;
            TACCTL0 &= ~CCIE;                   // CLEAR interrupt ENABLED
            k=0;
            pos++;
        }
        if(k>=5)                                // 0.6 sec and up
        {
            touchMsg[pos] = LINE;
            TACCTL0 &= ~CCIE;                   // CLEAR interrupt ENABLED
            k=0;
            pos++;
        }
        if (pos==5)
        {
            TACCTL0 &= ~CCIE;                   // CLEAR interrupt ENABLED
            morseToLetter(pos);
            pos=0;
            k=0;
        }
    }

    k++;
}

#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR (void)
{
    // End of Character
    if((P1IFG & BIT1) && pos>0 && pos <5)           // SW2 Pressed
    {
        TACCTL0 &= ~CCIE;                           // CLEAR interrupt ENABLED
        morseToLetter(pos);                         // Morse To Letter
    }
    pos=0;

    // END of Capacitive MESSAGE
    if((P1IFG & BIT0) && (INCOMING_STATE==TRUE))    // SW1 Pressed
    {
        isSending = FALSE;
        INCOMING_STATE=FALSE;
        send_DMA(NLprompt,sizeof(NLprompt));
    }

    P1IFG &= ~(BIT1+BIT0);             				// clear IFG SW1 & SW2
}

// Interrupt for DMA
#pragma vector=DMA_VECTOR
__interrupt void DMA_ISR (void)
{
    int static ii = 0;
    DMAIV &= ~DMAIV_DMA0IFG;            			// clear DMA IFG flag
    if(ii==0)                           			// SEND Prompt Text after TITLE
        send_DMA(prompt,sizeof(prompt));

    if(DMA0SA == specChar)              			// print after non morse char
        send_DMA(NLprompt,sizeof(NLprompt));
    ii++;
}