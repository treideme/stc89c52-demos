#include "main.h"

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

void delayms(uint16_t x)
{
	uint16_t i,j;
	for(i=x; i>0; i--)
		for(j=111; j>0; j--);
}
