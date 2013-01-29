/*
*
*Project Embedded 1
*Rev v2.1 release date 20/01/2013
*Author Anthony Scranney
*
*/
// Includes
#include  "msp430g2231.h"
// Defines and variables
#define     LED1                  BIT6
#define     LED2                  BIT5
#define BUTTON BIT3
unsigned int adc[2] = {0};
unsigned int divadc = 0;
unsigned int result = 0;

// Setup hardware
void SetHW()
{
	WDTCTL = WDTPW + WDTHOLD;			// Stop WDT
	BCSCTL1 = CALBC1_1MHZ;				// Set range DCOCTL = CALDCO_1MHZ;
	BCSCTL2 &= ~(DIVS_3);				// SMCLK = DCO = 1MHz
	P1IE |= BUTTON;						// S2 on P1.3 enabled for interrupt
	P1IFG &= ~BUTTON;					// P1.3 interrupt flag cleared
	P1DIR |= LED1;						// P1.6 output
	P1SEL |= LED1;						// P1.6 selection
	P1DIR |= LED2;						// P1.5 output
	P1OUT &= ~LED2;						// P1.5 selection
}

/********************     Input related code     ********************/
// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
	__bic_SR_register_on_exit(CPUOFF);        // Return to active mode
}
//Configure the ADC10
void ConfigureAdc(void)
{
	//ADC10 Control Register 1
	ADC10CTL0 = ADC10SHT_2 + MSC + ADC10ON + ADC10IE + REFON + REF2_5V;
	// ADC10SHT - selects the sample period in CLK cycles, MSC - multiple sample and conversion
	// ADC10ON  - turn the ADC10 on, ADC10IE - enable ADC10 interrupt
	// REFON - turn internal reference on, REF2_5V - reference voltage 2.5V
	//ADC10 Control Register 2
	ADC10CTL1 = INCH_2 + CONSEQ_1;	// Sets the ADC ports to monitor + defines a sequence of reads
	__delay_cycles(1000);			// Wait for ADC ref voltage to settle
	ADC10DTC1 = 0x02;				// Enable data transfer controller, copy ADC10MEM to RAM
	ADC10AE0 |= 0x03;				// Disable digital operation on the ports being used for the ADC
	ADC10SA = (unsigned int)adc;  	// Copies data in ADC10SA to unsigned int adc
}
// Start ADC and enable continous sequence read
void run_ADC()
{
		//ADC10SA = (unsigned int)adc;			// Copies data in ADC10SA to unsigned int adc
		ADC10CTL0 |= ENC + ADC10SC;             // Enable Conversion + Start Conversion
		__bis_SR_register(CPUOFF + GIE);        // LPM0 Low Power Mode 0 with interrupts enabled
		ADC10SA = (unsigned int)adc;			// Copies data in ADC10SA to unsigned int adc
}

/********************     Output related code     ********************/
// Change ADC value to suit Switch Case routine
void div_by10()
{
	int div_by_10 = 10;
	divadc = adc[1];							// Copies the second ADC array to integer divadc
	result = divadc / div_by_10 / div_by_10;	// Divides the ADC value by 10 then by 10 again
}
// Software PWM using Switch Case
void soft_PWM()
{
	int count;
	switch (result)
		{
		case 0:
			for (count = 1; count <=5; count++)		// For loop to allow additional control of PWM
						{
						P1OUT |= LED2;			    // turns the LED on
						__delay_cycles(400);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(100);		// space
						}
			break;
		case 1:
			for (count = 1; count <=5; count++)
						{
						P1OUT |= LED2;			    // turns the LED on
						__delay_cycles(100);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(100);		// space
						}
			break;
		case 2:
			for (count = 1; count <=25; count++)
						{
						P1OUT |= LED2;			    // turns the LED on
						__delay_cycles(1000);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(2000);		// space
						}
			break;
		case 3:
			for (count = 1; count <=15; count++)
						{
						P1OUT |= LED2;			  	// turns the LED on
						__delay_cycles(1000);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(3000);		// space
						}
			break;
		case 4:
			for (count = 1; count <=12; count++)
						{
						P1OUT |= LED2;			  	// turns the LED on
						__delay_cycles(1000);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(4000);		// space
						}
			break;
		case 5:
			for (count = 1; count <=12; count++)
						{
						P1OUT |= LED2;			  	// turns the LED on
						__delay_cycles(1000);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(5000);		// space
						}
			break;
		case 6:
			for (count = 1; count <=12; count++)
						{
						P1OUT |= LED2;			  	// turns the LED on
						__delay_cycles(1000);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(6000);		// space
						}
			break;
		case 7:
			for (count = 1; count <=10; count++)
						{
						P1OUT |= LED2;			  	// turns the LED on
						__delay_cycles(1000);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(8000);		// space
						}
			break;
		case 8:
			for (count = 1; count <=7; count++)
						{
						P1OUT |= LED2;				// turns the LED on
						__delay_cycles(1000);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(9000);		// space
						}
			break;
		case 9:
			for (count = 1; count <=5; count++)
						{
						P1OUT |= LED2;				// turns the LED on
						__delay_cycles(200);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(10000);		// space
						}
			break;
		case 10:
			for (count = 1; count <=5; count++)
						{
						//P1OUT |= LED2;			// turns the LED on
						//__delay_cycles(10);		// mark
						P1OUT &= ~ LED2;			// turns the LED off
						__delay_cycles(10000);		// space
						}
			break;
		}
}

// MSP430G2 Timer_A generate PWM
void MSP_PWM()
{
//P1DIR |= LED1;				// P1.6 output
//P1SEL |= LED1;				// P1.6 options
	CCR0 = 200;					// PWM Period
	CCTL1 = OUTMOD_2;			// CCR1 toggle/rest - output mode
	CCR1 = (adc[0]/2);			// CCR1 PWM duty cycle - adc[0] dynamically controls the duty cycle
	TACTL = TASSEL_2 + MC_1;	// TACTL = Timer_A Control Register, TASSEL-Timer_A clock source
		  	  	  	  	  		// MC_1-Mode Control, count up to CCR0 value, continuous and up/down.
//_BIS_SR(CPUOFF);              // Enter LPM0
}

/********************     Start MAIN     ********************/
void main(void)
{
	SetHW();
	ConfigureAdc();
	__enable_interrupt();

// Infinite while loop
while(1)
	{
	run_ADC();		// Continuous read of ADC
	MSP_PWM();		// Timer_A PWM
	div_by10();		// Divide by 10 function
	soft_PWM();		// Software PWM function using switch case
	}
}
