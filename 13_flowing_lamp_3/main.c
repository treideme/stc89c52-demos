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
	�⺯��ʵ����ˮ��
	by hongXkeX
	2016.11.3
*/
void dis_cled(void)
{
	t = 0xfe;
	for(i=0; i<7; i++)
	{
		P2 = t;
		delayms(100);
		t = _crol_(t,1);
	}

	t = 0x7f;
	for(i=0; i<7; i++)
	{
		P2 = t;
		delayms(100);
		t = _cror_(t,1);
	}	
}

void init_mcu(void)
{
	P2 = 0xff;
}
