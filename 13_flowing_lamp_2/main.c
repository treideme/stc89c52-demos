#include "main.h"
				           // 1    2    3    4    5    6   7    8
const uint8_t table[]={0,0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};

uint8_t  i = 0;
uint8_t  t = 0;

void main(void) {
	init_mcu();
	while(1)
	{
		dis_chaLed();
	}
}

/*
	by hongXkeX
	2016.11.4
*/
void dis_chaLed(void)
{
	t = 2;
	while(t--)
	{
		for(i=8; i>0; i--)
		{
			P2 = table[i];
			delayms(100);
		}
	}

	t = 2;
	while(t--)
	{
		for(i=1; i<9; i++) {
			P2 = table[i];
			delayms(100);
		}
	}

	t = 2;
	while(t--) {
		P2 = 0xff;
		delayms(500);
		P2 = 0;
		delayms(500);
	}

	t = 2;
	while(t--)
	{
		for(i=8; i>1; i--)
		{
			P2 = table[i];
			delayms(100);
		}
		for(i=1; i<8; i++)
		{
			P2 = table[i];
			delayms(100);
		}
	}

	t = 2;
	while(t--) {
		P2 = 0xff;
		delayms(500);
		P2 = 0;
		delayms(500);
	}

	t = 2;
	while(t--)
	{
		for(i=1; i<5; i++)
		{
			P2 = (table[i] & table[9-i]);
			delayms(100);
		}
		for(i=3; i>1; i--)
		{
			P2 = (table[i] & table[9-i]);
			delayms(100);
		}
	}

	t = 2;
	while(t--) {
		P2 = 0xff;
		delayms(500);
		P2 = 0;
		delayms(500);
	}
}

void init_mcu(void)
{
	P2 = 0xff;
}