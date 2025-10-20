#include "main.h"

#define led1 P2_0

void main(void) {
	init_mcu();
	while(1) 
	{
		led1 = 0;
		delayms(500);
		led1 = 1;
		delayms(500);					
	}		
}

void init_mcu(void)
{
	P2 = 0xff;
}