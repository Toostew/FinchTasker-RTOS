/*
 * config.c
 *
 *  Created on: 26 Jul 2026
 *      Author: tooka
 */
#include "main.h"

//config systick, does not fire
void systick_config(){

	SysTick->CTRL &= ~((1 << 2) | (1 << 1) | (1 << 0));
	SysTick->CTRL |= ((1 << 2) | (1 << 1)); //CLKSOURCE, TICKINT

	SysTick->LOAD = 1699999; //(clock frequency(hz) x desired delay(seconds)) - 1 = (170,000,000 x 0.001)
	SysTick->VAL = 0; //read VAL once to reset VAL to 0
}


//toggles the systick
void systick_toggle(int i){
	if (i == 0){
		SysTick->CTRL &= ~(1 << 0);
	} else {
		SysTick->CTRL |= (1 << 0);
	}
}
