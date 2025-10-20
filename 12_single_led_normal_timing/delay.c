#include "main.h"

void delay1ms(void)		//@12.000MHz
{
	uint8_t i, j;

	i = 12;
	j = 169;
	do
	{
		while (--j);
	} while (--i);
}

void delay500ms(void)		//@12.000MHz
{
	uint8_t i, j, k;
	__asm__("nop");;
	__asm__("nop");;
	i = 23;
	j = 205;
	k = 120;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void delayms(uint16_t x)
{
	uint16_t i , j;
	for(i =x ; i>0; i--)
		for(j=111; j>0 ; j--);
}


