#include"main.h"

void delay1ms(void)
{
	uint8_t i, j;
	i = 12;
	j = 169;
	do
	{
		while (--j);
	} while (--i);
}

void delay_ms(uint16_t ms)
{
	while(ms--) 
	{
		delay1ms();	
	}	
}

