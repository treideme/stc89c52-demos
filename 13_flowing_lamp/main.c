#include "main.h"

uint8_t i = 0;
uint8_t t = 0;

void main(void)
{
	init_mcu();
	while(1) 
	{
		dis_cled();				
	}		
}

/*
	��λ��ʵ����ˮ��
	by hongXkeX
	2016.11.3
*/
void dis_cled(void)
{
	t = 0xfe;
	for(i=0; i<8; i++)
	{
		P2 = t;			
		t = (t << 1) | 0x01;
		delayms(100);
	}

	t = 0xbf;
	for(i=0; i<6; i++)
	{
		P2 = t;
		t = (t >> 1) | 0x80;
		delayms(100);
	}	
}

void init_mcu(void)
{
	P2 = 0xff;
}
