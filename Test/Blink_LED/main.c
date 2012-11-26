#include <msp430g2231.h>

void main(void) {


	WDTCTL = WDTPW + WDTHOLD;



P1DIR = 0x01 + 0x40;
P1OUT = 0;


while(1)
{
	P1OUT = 0x01;
	_delay_cycles(1000000);
	P1OUT = 0;
	_delay_cycles(1000000);


	P1OUT = 0x40;
	_delay_cycles(500000);
	P1OUT = 0;
	_delay_cycles(500000);


}

}
