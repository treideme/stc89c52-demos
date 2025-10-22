/*
	������������С����
	by hongke 2016-11-9

*/
#include "main.h"

#define key1 P3_0
#define led1 P2_0

#define key2 P3_1
#define led2 P2_1

uint16_t timer = 0;

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
	}	
}

void init_mcu(void)
{
	key1 = 1;
	key2 = 1;
	P2 = 0xff;	
}
