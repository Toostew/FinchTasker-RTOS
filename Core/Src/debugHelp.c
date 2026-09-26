/*
 * debugHelp.c
 *
 *  Created on: 24 Jul 2026
 *      Author: tooka
 */
#include "main.h"

//config to configure pin PB1 as GPIO out
void blinkConfig(){

	RCC->AHB2ENR |= (1 << 1); //enable AHB2 for GPIOB
	GPIOB->MODER &= ~((0b11 << 2) | (0b11 << 8) | (0b11 << 10)); //2 bit region
	GPIOB->MODER |= ((1 << 2) | (1 << 4) | (1 << 8) | (1 << 10)); //set mode to GP output
}

void setResetBlink(int setReset){
	if(setReset == 0){
		GPIOB->BSRR |= (1 << 17); // reset blink
	} else {
		GPIOB->BSRR |= (1 << 1); //set blink
	}
}

void toggleBlink(int pin){
	if(pin == 0){
		if(GPIOB->ODR & (1 << 1)){
			GPIOB->BSRR |= (1 << 17); //reset
		} else {
			GPIOB->BSRR |= (1 << 1); //set
		}
	}
	else if (pin == 1) {
		if(GPIOB->ODR & (1 << 4)){
			GPIOB->BSRR |= (1 << 20);
		} else {
			GPIOB->BSRR |= (1 << 4);
		}
	}
	else if (pin == 2){
		if(GPIOB->ODR & (1 << 5)){
			GPIOB->BSRR |= (1 << 21);
		} else {
			GPIOB->BSRR |= (1 << 5);
		}
	}

}
