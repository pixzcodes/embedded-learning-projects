/*
*	See the README for the writ-up for this project
*/
#include <stdint.h>

#define PIN 5

#define RCC_AHB1 (*(volatile uint32_t *)(0x40023800 + 0x30))
#define GPIOA_ODR (*(volatile uint32_t *)(0x40020000 + 0x14))
#define GPIOA_MODER (*(volatile uint32_t *)(0x40020000 + 0x00))

int main(void) {
	RCC_AHB1 |= (1U << 0); // enable GPIOA clock for our pin
	GPIOA_MODER &= ~(3U << (PIN * 2)); // clear mode bits for the pin
	GPIOA_MODER |= (1U << (PIN * 2)); // set pin mode to general purpose output

	while (1) {
		GPIOA_ODR ^= (1U << PIN ); // toggle our pin
		for (volatile int i = 0; i < 500000; i++); // crude delay
	}
}
