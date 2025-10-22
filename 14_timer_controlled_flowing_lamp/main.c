#include "main.h"

uint8_t i = 0;
uint8_t flag = 1;

void main(void)
{
	init_mcu();
	t0_init();
	EA = 1;
	while(1) 
	{
	
	}		
}

void init_mcu(void)
{
	P2 = 0xff;
}

void t0_init(void)
{
	TMOD &= (0xf0);        // TMOD = XXXX0000          
	TMOD |= (0x01);		    // TMOD = XXXX0001
	// 12M һ����������1us   
	TH0 = (65536-5000)/256; // 5ms �ж�һ��
	TL0 = (65536-5000)%256;
	
	ET0 = 1;				// ����ʱ��0�ж�
	TR0 = 1;				// ������ʱ��0
}

void t0_isr(void) __interrupt(TF0_VECTOR)
{
	static uint8_t k = 0;
	TH0 = (65536-5000)/256;
	TL0 = (65536-5000)%256;
	k++;
	if( k>=20 && flag ==1 ) {
		k = 0;
		P2 = ~( 0x80 >> i );
		i++;
		if( i > 6 ){
		  	i = 0;
			flag = 0;
		} 		
	}

	if( k >= 20 && flag == 0 ){
		k = 0;
		P2 = ~( 0x01 << i );
		i++;
		if( i > 6 ){
		  	i = 0;
			flag = 1;
		} 
	}
}
