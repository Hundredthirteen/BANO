//******************************************************************************
//  MSP430F20x2 Demo - ADC10, DTC Sample A0 -> TA1, AVcc, DCO
//
//  Description: Use DTC to sample A0 with reference to AVcc and directly
//  transfer code to TACCR1. Timer_A has been configured for 10-bit PWM mode.
//  TACCR1 duty cycle is automatically proportional to ADC10 A0. WDT_ISR used
//  as a period wakeup timer approximately 45ms based on default ~1.2MHz
//  DCO/SMCLK clock source used in this example for the WDT clock source.
//  Timer_A also uses default DCO.
//
//                MSP430F20x2
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//        >---|P1.0/A0      P1.2|--> TACCR1 - 0-1024 PWM
//
//  L. Westlund
//  Texas Instruments Inc.
//  May 2006
//  Built with CCE Version: 3.2.0 and IAR Embedded Workbench Version: 3.41A
//******************************************************************************
#include  "msp430x20x2.h"

//#define  MAX_PWM 31 //679kHz
//#define  MAX_PWM 60 //355kHz
//#define  MAX_PWM 90 //238.6kHz
//#define  MAX_PWM 150 //127kHz
  #define  MAX_PWM 200 //96kHz
//#define  MAX_PWM 300 //72.3kHz
//#define  MAX_PWM 500 //43.4kHz
//#define  MAX_PWM 1000 //15kHz
//#define  MAX_PWM 4000 //4kHz

//	100ms/1500ms
//  48.498/727.470

#define	STROBE_PULSE	40//80ms
#define STROBE_DELAY    25//50ms /40ms
#define	STROBE_PERIOD	749//1498ms /1400ms
//#define	STROBE_PERIOD	550//1100ms

#define MASTER          1
#define SLAVE           2

//6   -> 94ms
//7	  -> 110ms
//100 -> 1.5s
//130 -> 1.95s
//133 -> 2s
//#define	STROBE_PULSE	6 //94ms
//#define	STROBE_PERIOD	100 //1.5s

//data
unsigned long int ADC_temp;
unsigned long int ADC_avg = 0;
unsigned long int i = 0;

signed long int Threshold = 0;
signed long int reg_counter = 0;
signed long tmp;
signed long int wdt_counter = 0;
signed long corrector1 = 0;
signed long corrector2 = 0;
unsigned char mode;
unsigned char P14OLD;
volatile unsigned int PREVENTER;

#define _delay(num) \
  {for(unsigned long i = 0; i<=num; i++) \
		_NOP(); \
	}

//=========main fuction
void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
	DCOCTL = CALDCO_1MHZ;                  // Set DCO to 16MHz
	BCSCTL1 = (CALBC1_1MHZ & 0x0F);                 // MCLC = SMCLK = DCOCLK = 16MHz
//	WDTCTL = WDT_MDLY_32;                    /* 512 / 12000Hz = 43ms  " */

        _delay(100000);

        BCSCTL2 = 0x00;
	//BCSCTL3 = LFXT1S_0 + XCAP_1; // 32768KHz crystal, 10 pF
	BCSCTL3 = LFXT1S_0 + XCAP_3; // 32768KHz crystal, 12.5 pF



///*

//        _delay(10000);  //~0.5 sec

	//=====System Clock
//	DCOCTL = CALDCO_16MHZ;                  // Set DCO to 16MHz
//	BCSCTL1 = CALBC1_16MHZ;                 // MCLC = SMCLK = DCOCLK = 16MHz
        BCSCTL1 &= ~0x40; //Low-frequency mode
        BCSCTL1 &= ~0x30; //Divider for ACLK = 1

	BCSCTL2 = 0x00;
	BCSCTL3 = LFXT1S_0 + XCAP_3; // 32768KHz crystal, 12.5 pF
//	BCSCTL3 = LFXT1S_0 + XCAP_2; // 32768KHz crystal, 10 pF
//	BCSCTL3 = LFXT1S_0 + XCAP_1; // 32768KHz crystal, 6 pF
//	BCSCTL3 = LFXT1S_0 + XCAP_0; // 32768KHz crystal, 1 pF


        WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
//	WDTCTL = WDT_ADLY_16; //WDT_MDLY_0_064;  //2.062ms
       	WDTCTL = WDT_ADLY_1_9;

	IE1 |= WDTIE;
//	IE1 &= ~WDTIE;                             // Disable WDT interrupt


        _delay(100000);

        wdt_counter = 0;

	DCOCTL = CALDCO_16MHZ;                  // Set DCO to 16MHz
	BCSCTL1 = CALBC1_16MHZ;                 // MCLC = SMCLK = DCOCLK = 16MHz

//*/

	//=====ADC Setup

	ADC10CTL0 = ADC10SHT_3 + SREF_1 + REFON + ADC10ON;
	ADC10CTL1 = INCH_1 + ADC10DIV_0 + ADC10SSEL_2; //  2   A1 input
	ADC10AE0 = 0x02;                          // P1.1 ADC option select
	ADC10DTC1 = 0x001;                        // 1 conversion


        P1DIR = 0x00;								// All input
	P2DIR = 0x00;								// All input
//	P1DIR &= ~0x08;                             // P1.3 = input
	P1DIR |= 0x01;                       		// P1.0 = output
	P1DIR |= 0x04;                       		// P1.2 = output
	P1DIR |= 0x80;                       		// P1.7 = output
	P1SEL |= 0x04;                  			// P1.2 = TA1 output
	P1SEL &= ~0x01;                  			// P1.0 = GPIO
//	P1SEL |= 0x01;                  			// P1.0 = ACLK
	P1SEL &= ~0x08;                  			// P1.3 = GPIO
	P1SEL &= ~0x10;                  			// P1.4 = GPIO
	P1SEL &= ~0x80;                  			// P1.7 = GPIO

	P1OUT &= ~0x01;

        P1REN |= 0x08;                                          // P1.3 Pull up/down enable;
        P1OUT |= 0x08;                                          // P1.3 Pulled up

        P1REN |= 0x10;                                          // P1.4 Pull up/down enable;
        P1OUT |= 0x10;                                          // P1.4 Pulled up

	P2DIR |= 0x80;								// P2.7	= output
	P2SEL |= 0x80;								// P2.7 = Xtal
//	P2DIR &= ~0x40;								// P2.6 = input
	P2SEL |= 0x40;								// P2.6 = Xtal

	TACCTL1 = OUTMOD_7;                			// TACCR1 reset/set
	TACCR0 = MAX_PWM;                  			// PWM Period
	TACCR1 = 0;                                             // TACCR1 PWM Duty Cycle
	TACTL = TASSEL_2 + MC_1;                  		// SMCLK, upmode

//	P1IES |= 0x08;								// P1.3 Hi/lo edge
	P1IES &= ~0x08;								// P1.3 lo/hi edge
	P1IFG &= ~0x08;								// P1.3 IFG cleared
	P1IE &= ~0x08;								// P1.3 interrupt disabled

	_EINT();


//	tmp = TACCR1;
//	tmp = MAX_PWM*0.5;
	tmp = 0;
	TACCR1 = 0;

	if(P1IN & 0x08) //RED
	  mode = MASTER;
	else		//GREEN
	  mode = SLAVE;

        P14OLD = (P1IN & 0x10);

        //Delay(100)
        //P1OUT |= 0x80;


	//=====main loop
	while(1)
	{

		if(P1IN & 0x08) //RED
		  Threshold = 205;//300mV /350 mV
		else		//GREEN
		  Threshold = 137;//200mv
//		  Threshold = 82;//120mv for tail white light / хвостовой (полная комплектация)
//		  Threshold = 102;//150mv for tail white light / позиционный (только стабилизатор, без кварца)

//    		  Threshold = 171; //For tail
//		  Threshold = 119;//175mv /220mV;

/*
205/119 <=> 300/175
*1.16
238/138 <=> 350/200

y=0.68254x
50mV  ->  34
100mV ->  68
120mV ->  82
150mV ->  102
175mV ->  119
200mV ->  137
220mV ->  150
250mV ->  171
300mV ->  205
350mV ->  239
400mV ->  273
450mV ->  307
500mV ->  341
*/


                ADC10CTL0 |= ENC + ADC10SC; // start sampling
 		while(ADC10CTL1 & ADC10BUSY);
		ADC_avg = (ADC10MEM + ADC_avg * 31)>>5;
//		ADC_avg = (ADC10MEM + ADC_avg * 7)>>3;
//		ADC_avg = (ADC10MEM + ADC_avg * 3)>>2;
//		ADC_avg = ADC10MEM;

                reg_counter += (Threshold - ADC_avg);

         	if(reg_counter >1)//50
	        {
		  reg_counter = 0;
        	  if(tmp < MAX_PWM) tmp++;
	        }
		else
          	if(reg_counter <-1)//-50
		{
            	  reg_counter = 0;
                  if(tmp > 0) tmp--;
          	}

       		TACCR1 = tmp;
		ADC10CTL0 &= ~ENC;

                PREVENTER++;
                if(PREVENTER > 5000)//100ms
                {
                  PREVENTER = 0;
                  P1OUT &= ~0x01;//Turn off strobe
                  //P1OUT ^= 0x01;//Toggle strobe
                }

	}

}


//==========WDT Route
#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR(void) //~2ms
{

  PREVENTER = 0;

//   P1OUT ^= 0x01;
{
   wdt_counter++;

   if(wdt_counter == ((STROBE_PERIOD - STROBE_PULSE - STROBE_PULSE - STROBE_DELAY)/2))
     P1OUT |= 0x80;

//   if(mode == SLAVE)
//if(P1IN & 0x08)

   {
     if( (P14OLD != (P1IN & 0x10)) && (!(P1IN & 0x10)) )
     {
         wdt_counter = ((STROBE_PERIOD - STROBE_PULSE - STROBE_PULSE - STROBE_DELAY)/2)+1;
         P1OUT &= ~0x01;
     }
     P14OLD = (P1IN & 0x10);
   }

   if(wdt_counter == STROBE_PERIOD - STROBE_PULSE - STROBE_PULSE - STROBE_DELAY)
	 P1OUT |= 0x01;
   else
     if(wdt_counter == STROBE_PERIOD - STROBE_PULSE - STROBE_DELAY)
	   P1OUT &= ~0x01;
     else
       if(wdt_counter == STROBE_PERIOD - STROBE_PULSE)
	     P1OUT |= 0x01;
       else
	     if(wdt_counter == STROBE_PERIOD)
             {
	        P1OUT &= ~0x81;
                wdt_counter = 0;
	     }
}
}
