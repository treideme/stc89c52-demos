/*
	������������С����
	by hongke 2016-11-9

*/
#include "main.h"

#define key1 P3_0
#define key2 P3_1

#define led1 P2_0
#define led2 P2_1
#define led3 P2_2
#define led4 P2_3
#define led5 P2_4

uint16_t timer = 0;
uint8_t key;

void main(void)
{
	init_mcu();
	while(1)
	{
		if(!key1) {
			delayms(20);
			if(!key1) {
				led1 ^= 1;
				timer = 0; 
				while(!key1 && timer++<800) 
				{		
					delay1ms(); 
				}
			}		
		}
					
		if(!key2) {
			delayms(20);
			if(!key2) {
				led2 ^= 1;
				timer = 0; 
				while(!key2 && timer++<800) 
				{		
					delay1ms(); 
				}
			}		
		}
		
		key = get_keyval();
		if(key != 0xff) {
		   	delayms(20);
			if(key != 0xff) {
				timer = 0;
				switch(key) 
				{
					case 0: led1^= 1; break;
					case 1: led2^= 1; break;
					case 2: led3^= 1; break;
					case 3: led4^= 1; break;
					case 4: led5^= 1; break;
				}
				while(key!=0xff && timer++<200) 
				{		
					delay1ms(); 
				}
			}
		}			
	}	
}

void init_mcu(void)
{
	key1 = 1;
	key2 = 1;
	P2 = 0xff;	
}

uint8_t get_keyval(void)
{
	uint8_t tmp,ke=0;
	uint8_t flag = 0;
	uint8_t tmp1 = 0x7f;
	uint8_t i;
	for(i=0; i<4; i++) 
	{
		P1 = tmp1;
		tmp = P1 & 0x0f;
		if(tmp != 0x0f) {
			delayms(20);
			if(tmp != 0x0f){
				flag = 1;
				switch(tmp) {
					case 0x07: ke = 0+i*4; break;
					case 0x0b: ke = 1+i*4; break;
					case 0x0d: ke = 2+i*4; break;
					case 0x0e: ke = 3+i*4; break;
				}
			}else {
				flag = 0;		
			}
		}	
		tmp1 = _cror_(tmp1, 1);
	}
	if(flag == 1) {
		return ke;
	}else {
		return 0xff;
	}
}




