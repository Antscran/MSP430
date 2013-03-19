#include  "ExtFunc/itoa.h"  // "" means local include <> external
#include  "ExtFunc/lcd.h"  // "" means local include <> external
#include  "msp430g2432.h"

#define 	OnOff_B 		BIT3		// BIT3 on P1, S2 button on Launchpad LOW to make
#define 	Select_B 		BIT0		// BIT0 on P2, break out veroboard HIGH to make
#define 	Inc_B 			BIT2		// BIT1 on P2, break out veroboard HIGH to make
#define 	Dec_B 			BIT1		// BIT2 on P2, break out veroboard HIGH to make
#define		LED				BIT3		// BIT3 on P2, LED
#define		LED2			BIT5		// BIT5 on P2, LED

// State definitions, states are the routine the program jumps to, these are prefixed with an 'S'
enum states {S_OFF, S_ON, S_BRIADJ, S_PRESET, S_AUTO, S_MAX};
// Event definitions, events trigger a transition and are prefixed with an 'E'
enum events {E_OFF, E_ON, E_BRIADJ, E_PRESET, E_AUTO, E_INC, E_DEC, E_TIMEOUT, E_MAX};

// Global variables
int				Current_State = S_OFF;	// Default state the system starts in
volatile int	sysTick = 0;		// Volatile to prevent main code accessing a register copy
unsigned int	LongDelay = 0;		// Used to create a 15 Sec delay
unsigned int	ScrRefresh = 0;		// Used to slow the screen refresh rate down

unsigned int brightness = 5;		// variable to store brightness external to StateM
unsigned int preset_mode = 1;		// variable to store preset mode external to StateM
unsigned int adc;					// Used to store the ADC value
unsigned int briadjSys = 0;			// Used as a flag when S_BRIADJ = Cyrrent_State
unsigned int presetSys = 0;			// Used as a flag when S_PRESET = Cyrrent_State
unsigned int autoSys = 0;			// Used as a flag when S_AUTO = Cyrrent_State

void StateM(int event);		// Contains states S_OFF, S_ON, S_BRIADJ, S_PRESET, S_MAX
int CheckButtons();  		// Generates events from button presses
//void OnExit( int State);	// Upon exiting a state call the function
void OnEnter( int State);	// Upon entering a new state call the function
void Do (int State);		// While in the current state call the function

int bri_adj();				// Brightness adjust function
int preset_modes();			// Preset mode function
void soft_PWM();			// Software PWM used in conjunction with bri_adj() function
void preset_level();		// Software PWM used in conjunction with preset_modes() function
void automatic();			// Starts ADC then alters value to suit soft_PWM() function
void automatic_lcd();		// Uses integer ScrRefresh to slow the timing for the LCD
void CustDelay();			// Custom delay approx. 500mS
void TimeOut();				// Function returns to S_ON state after approx. 15 sec

int setup_HW()				// Setup HW clocks, ports etc
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;		// Set range
	DCOCTL = CALDCO_1MHZ;		// Set DCO step and modulation
	ADC10CTL0 = ADC10SHT_3 + ADC10ON;	// Setup ADC
	ADC10CTL1 = INCH_2;					// Setup ADC
	P2DIR |= LED;						// P2.3 output
	P2OUT &= ~LED;						// P2.3 selection
	P2DIR |= LED2;						// P2.5 output
	P2OUT &= ~LED2;						// P2.5 selection
}

/*     **********     Start of sysTick timer and interrupt code    **********     */
int counter()
{
	TA0CCR0 = 65500;					// Count limit 137.5kHz / 65750 = 2.09, therefore interrupt triggers roughly every 478mS
	TA0CCTL0 = CCIE;					// Enable counter interrupts
	TA0CTL = TASSEL_2 + MC_1 + ID_3; 	// Timer A 0 with  SMCLK at 1.1MHZ / 8 =  137.5kHz, count UP
	_BIS_SR( GIE ); 	        		// Enable interrupts
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A0 (void)		// Timer0 A0 interrupt service routine
{
		sysTick = 1;

		LongDelay++;
		if(LongDelay >= 32)
		{LongDelay = 0;}

		ScrRefresh = 1;
}
/*     **********     End of sysTick timer and interrupt code    **********     */


/*     **********     Start Main    **********     */
int main()
{
	setup_HW();					// Function to setup hardware
	InitializeLcm();			// Initialise the LCM into 4bit mode
	ClearLcmScreen();			// Clear the LCM buffer
	counter();					// Timer_A determines the time for the sysTick

    while ( Current_State != S_MAX )		// Continue while Current_State is (NOT =) to S_MAX
    {
        if (sysTick == 1)
        	{
        	if (~P1IN & OnOff_B || P2IN & Select_B || P2IN & Inc_B || P2IN & Dec_B)	// If sysTick coincides with OnOff or Select button being pressed, run state machine
        	  	  {
        			StateM( CheckButtons() );	// Execute state machine function
        			sysTick = 0;				// Resets sysTick to 0
        			LongDelay = 0;				// Resets LongDelay if key pressed
        	  	  }
        	}

        if (briadjSys == 1 && Current_State != S_OFF)
        {soft_PWM();}
        if (presetSys == 1 && Current_State != S_OFF)
        {preset_level();}
        if (autoSys == 1 && Current_State != S_OFF)
        {automatic();
        soft_PWM();
        automatic_lcd();}

        TimeOut();
    }
    return 0;
}
/*     **********     End Main    **********     */

int CheckButtons()	// Buttons trigger an event which is then parsed to the StateM function
{
	if (LongDelay >= 30)
		{
		return E_TIMEOUT;
		}

	if(~P1IN & OnOff_B)			// ON/OFF button pressed **S2 LOW to make
	{
		if (Current_State != S_OFF) // If the Current_State is NOT OFFSTATE execute IF
			{
			return E_OFF;		// Pass OFF to the StateM Function
			}
		else
			{
			return E_ON;		// Pass ON to the StateM Function
			}
	}

	if(P2IN & Select_B)			// SELECT button pressed **HIGH to make
	{
		if (Current_State == S_ON)		// If the Current_State is equal to S_ON execute IF
			{
			return E_BRIADJ;	// Pass E_BRIADJ to the StateM Function
			}
		if (Current_State == S_BRIADJ)	// If the Current_State is equal to S_BRIADJ execute IF
			{
			return E_PRESET;	// Pass E_PRESET to the StateM Function
			}
		if (Current_State == S_PRESET)	// If the Current_State is equal to S_BRIADJ execute IF
			{
			return E_AUTO;		// Pass E_AUTO to the StateM Function
			}
		else					// If the Current_State is equal to  execute ELSE
			{
			return E_BRIADJ;	// Pass E_BRIADJ to the StateM Function
			}
	}
	if(P2IN & Inc_B)		// INCREASE button pressed **HIGH to make
		{
		return E_INC; 		// Pass E_INC to the StateM Function
		}
	if(P2IN & Dec_B)		// DECREASE button pressed **HIGH to make
		{
		return E_DEC;		// Pass E_DEC to the StateM Function
		}
	else
		{
		return S_MAX;
		}
}

/*     **********     Start of state machine code    **********     */
void StateM(int event)
{
	int NextState = Current_State;	// NextState is equal to the Current_State value

    switch ( Current_State )		// Switch the Current_State value using the event
    {								//value parsed from the CheckButtons function
    case S_OFF:						// Current state S_OFF, accepts 1 event and
        switch (event)				//can therefore only move to 1 other state S_ON
        {
		case E_ON:
			NextState = S_ON;
			break;
        }
        			break;

    case S_ON:						// Current state S_ON, accepts 2 events and can
    	switch (event)				//therefore only accept 2 transitions to other states
        {
		case E_BRIADJ:
		    NextState = S_BRIADJ;
		    break;
		case E_OFF:
		    NextState = S_OFF;
		    break;
        }
        			break;

    case S_BRIADJ:					// Current state S_BRIADJ, accepts 5 events but can
    	switch (event)				//only accept 3 transitions to other states
        {
		case E_PRESET:
		    NextState = S_PRESET;
		    break;
		case E_OFF:
		    NextState = S_OFF;
		    break;
		case E_TIMEOUT:
			NextState = S_ON;
			break;
		case E_INC:
		    NextState = Current_State;		// No state change, but changes a parameter
		    break;							//inside the current state
		case E_DEC:
		    NextState = Current_State;		// No state change, but changes a parameter
		    break;							//inside the current state
        }
        			break;
    case S_PRESET:					// Current state S_PRESET, accepts 5 events but can
    	switch (event)				//only accept 3 transitions to other states
        {
		case E_AUTO:
		    NextState = S_AUTO;
		    break;
		case E_OFF:
		    NextState = S_OFF;
		    break;
		case E_TIMEOUT:
			NextState = S_ON;
			break;
		case E_INC:
		    NextState = Current_State;		// No state change, but changes a parameter
		    break;							//inside the current state
		case E_DEC:
		    NextState = Current_State;		// No state change, but changes a parameter
		    break;							//inside the current state
        }
        			break;
     case S_AUTO:					// Current state S_PRESET, accepts 5 events but can
        switch (event)				//only accept 3 transitions to other states
        {
		case E_BRIADJ:
		    NextState = S_BRIADJ;
		    break;
		case E_OFF:
		    NextState = S_OFF;
		    break;
		case E_TIMEOUT:
			NextState = S_ON;
			break;
		case E_INC:
		    NextState = Current_State;		// No state change, but changes a parameter
		    break;							//inside the current state
		case E_DEC:
		    NextState = Current_State;		// No state change, but changes a parameter
		    break;							//inside the current state
            }
            			break;
    // The program should never get to the default case
    default:
        break;

    }
    // The code below executes every time the state machine function
    //is called, and runs after the transition into the next state

    if (NextState != Current_State)	// NextState (NOT =) to Current_State
    {								//which should always be the case
        //OnExit(Current_State);
        OnEnter(NextState);
        Current_State = NextState;
    }
    if ( event != E_MAX)
    	{Do( Current_State );}
}

// Function is called when entering a new state
void OnEnter( int State)
{
    switch (State)
    {
	case S_OFF:
		ClearLcmScreen();
		LcmSetCursorPosition(0,3);
		PrintStr("SYSTEM OFF");
    	briadjSys = 0;
    	presetSys = 0;
    	autoSys = 0;
		break;
	case S_ON:
		ClearLcmScreen();
		LcmSetCursorPosition(0,3);
		PrintStr("SYSTEM ON");
		break;
	case S_BRIADJ:
		ClearLcmScreen();
		LcmSetCursorPosition(0,1);
		PrintStr("BRIGHTNESS ADJ");
		briadjSys = 1;
		presetSys = 0;
		autoSys = 0;
		break;
	case S_PRESET:
    	ClearLcmScreen();
    	LcmSetCursorPosition(0,2);
    	PrintStr("PRESET MODES");
    	briadjSys = 0;
    	presetSys = 1;
    	autoSys = 0;
		break;
	case S_AUTO:
    	ClearLcmScreen();
    	LcmSetCursorPosition(0,4);
    	PrintStr("AUTOMATIC");
    	briadjSys = 0;
    	presetSys = 0;
    	autoSys = 1;
		break;
    }
}

// Function is called whenever an event is parsed, no
// matter if a transition has occurred or not.
void Do( int State)
{
    switch (State)
    {
    case S_OFF:
        break;
    case S_ON:
    	break;
    case S_BRIADJ:
    	bri_adj();
        break;
    case S_PRESET:
    	preset_modes();
        break;
    case S_AUTO:
    	automatic();
        break;
   }
}

/*
// Function is called when exiting a state *not implemented here
void OnExit( int State)
{
    switch (State)
    {
    switch (State)
    {
    case S_OFF:
        break;
    case S_ON:
    	break;
    case S_BRIADJ:
        break;
    case S_PRESET:
        break;
    }
}
*/
/*     **********     End of state machine code    **********     */

int bri_adj()
{
	ClearLcmScreen();
	LcmSetCursorPosition(0,1);
	PrintStr("BRIGHTNESS ADJ");

	if(P2IN & Inc_B)		// INCREASE button pressed **HIGH to make
	{
		if (brightness >= 10)
			{brightness = 10;}
		else
			{brightness++;}
	}

	if(P2IN & Dec_B)		// DECREASE button pressed **HIGH to make
	{
		if (brightness <= 1)
			{brightness = 1;}
		else
			{brightness--;}
	}

	switch (brightness)
		{
		case 1:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  1");
			break;
		case 2:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  2");
			break;
		case 3:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  3");
			break;
		case 4:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  4");
			break;
		case 5:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  5");
			break;
		case 6:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  6");
			break;
		case 7:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  7");
			break;
		case 8:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  8");
			break;
		case 9:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  9");
			break;
		case 10:
	    	LcmSetCursorPosition(1,3);
			PrintStr("Level  10");
			break;
		}
}

int preset_modes()
{
    	ClearLcmScreen();
    	LcmSetCursorPosition(0,2);
    	PrintStr("PRESET MODES");

    	if(P2IN & Inc_B)		// INCREASE button pressed **HIGH to make
    	{
    		if (preset_mode >= 3)
    			{preset_mode = 3;}
    		else
    			{preset_mode++;}
    	}

    	if(P2IN & Dec_B)		// DECREASE button pressed **HIGH to make
    	{
    		if (preset_mode <= 1)
    			{preset_mode = 1;}
    		else
    			{preset_mode--;}
    	}

    switch (preset_mode)
    {
    	case 1:
    	    LcmSetCursorPosition(1,3);
    		PrintStr("Night Light");
    		break;
    	case 2:
    	   	LcmSetCursorPosition(1,3);
    		PrintStr("TV Room");
    		break;
    	case 3:
    	   	LcmSetCursorPosition(1,3);
    		PrintStr("Work Room");
    		break;
    }
}

// Software PWM using Switch Case
void soft_PWM()
{
	switch (brightness)
	{
		case 1:
			P2OUT &= ~ LED2;			// turns the LED off
			break;
		case 2:
			P2OUT |= LED2;			    // turns the LED on
			__delay_cycles(20);			// mark
			P2OUT &= ~ LED2;			// turns the LED off
			__delay_cycles(600);		// space
			break;
		case 3:
			P2OUT |= LED2;			  	// turns the LED on
			__delay_cycles(30);			// mark
			P2OUT &= ~ LED2;			// turns the LED off
			__delay_cycles(500);		// space
			break;
		case 4:
			P2OUT |= LED2;			  	// turns the LED on
			__delay_cycles(40);			// mark
			P2OUT &= ~ LED2;			// turns the LED off
			__delay_cycles(400);		// space
			break;
		case 5:
			P2OUT |= LED2;			  	// turns the LED on
			__delay_cycles(50);			// mark
			P2OUT &= ~ LED2;			// turns the LED off
			__delay_cycles(400);		// space
			break;
		case 6:
			P2OUT |= LED2;			  	// turns the LED on
			__delay_cycles(60);			// mark
			P2OUT &= ~ LED2;			// turns the LED off
			__delay_cycles(350);		// space
			break;
		case 7:
			P2OUT |= LED2;			  	// turns the LED on
			__delay_cycles(70);			// mark
			P2OUT &= ~ LED2;			// turns the LED off
			__delay_cycles(250);		// space
			break;
		case 8:
			P2OUT |= LED2;				// turns the LED on
			__delay_cycles(80);			// mark
			P2OUT &= ~ LED2;			// turns the LED off
			__delay_cycles(200);		// space
			break;
		case 9:
			P2OUT |= LED2;				// turns the LED on
			__delay_cycles(100);		// mark
			P2OUT &= ~ LED2;			// turns the LED off
			__delay_cycles(50);			// space
			break;
		case 10:
			P2OUT |= LED2;				// turns the LED on
			__delay_cycles(1000);		// mark
			P2OUT &= ~ LED2;			// turns the LED off
			__delay_cycles(1);			// space
			break;
	}
}
void preset_level()
{
	switch (preset_mode)
	{
		case 1:
			P2OUT |= LED;			  	// turns the LED on
			__delay_cycles(10);			// mark
			P2OUT &= ~ LED;				// turns the LED off
			__delay_cycles(400);		// space
			break;
		case 2:
			P2OUT |= LED;			  	// turns the LED on
			__delay_cycles(50);			// mark
			P2OUT &= ~ LED;				// turns the LED off
			__delay_cycles(100);		// space
			break;
		case 3:
			P2OUT |= LED;				// turns the LED on
			__delay_cycles(1000);		// mark
			P2OUT &= ~ LED;				// turns the LED off
			__delay_cycles(1);			// space
			break;
	}
}

void automatic()
{
	adc = ADC10MEM;					// Copies data in ADC10MEM to unsigned int adc
	ADC10CTL0 |= ENC + ADC10SC;		// Enable Conversion + Start Conversion

	int divby = 9;
	brightness = adc / divby / divby;	// Divides the ADC value by 10 then by 10 again
	if (brightness >= 11)
	{brightness = 10;}
	if (brightness <= 1)
	{brightness = 1;}
}

void automatic_lcd()
{
   	if(ScrRefresh == 1 && Current_State != S_ON)
   	{
    ClearLcmScreen();
    LcmSetCursorPosition(0,4);
    PrintStr("AUTOMATIC");

    LcmSetCursorPosition(1,0);
    PrintStr("Level   =  ");

   	LcmSetCursorPosition(1,12);
   	int base = 10;				// Sets the base of the conversion to 10 i.e. Decimal
    unsigned char buffer[8];	// Buffer stores the string
	int counter = 4;			// ****Check this as should be 0 I think
	itoa(adc, buffer, base);	// Itoa function call with 3 parameters
    while(buffer[counter])
	{
    	PrintStr(buffer);  // This will print one character, the loop is used to print the whole string
    	counter++;
    }
   	}
   	ScrRefresh = 0;		// Reset screen refresh counter to zero
}

void TimeOut()  // Return menu to S_ON state after 15 seconds approximately
{
    if (LongDelay >= 30 && Current_State != S_ON && Current_State != S_OFF)
    	{
		StateM( CheckButtons() );	// Execute state machine function
		LongDelay = 0;				// Resets LongDelay counter
    	}
}

/*
void CustDelay()		// 900mS delay at 1MHZ approx.
{
	int x;
    	for (x = 0; x < 10; x++)
    	{
    		__delay_cycles(50000);
    	}
}
*/
