# Bare-Metal LED Blink on the STM32 Nucleo-F446RE
*My first STM32 embedded project configuring clocks, GPIO, and timing directly from the datasheet and reference manual with no HAL or external drivers.*

## Overview
This project blinks an external LED connected to a STM32 Nucleo-F446RE board using bare-metal C with no HAL, no external drivers, and no CubeMX generated code. Every peripheral address and register bit was derived directly from ST's documentation. 
The goal of this project was to dip my toes into the water of the STM32 ecosystem and learn more about how register addresses work, what happens when writing 1 to a pin address and seeing that actually affect the state of the LED rather than relying upon vendor abstractions. The only embedded electronics project I had done before this was blinking a LED on an Arduino. Arduino hides a lot of the more complex things about embedded from the developer. 

### Hardware:
- NUCLEO-F446RE (STM32F446RE, Arm Cortex-M4) 
- One external LED
- One current-limiting Resistor
- Breadboard
- Jumper wires
### Documentation Used:
- STM32F446xC/E Datasheet (DS10693)
- STM32F446xx Reference Manual (RM0390)
- STM32 Nucleo-64 User Manual (UM1724)

## Why no HAL?
The HAL is convenient, but it abstracts from what is really happening. Working at the register level forces embedded basics: what peripheral am I using? What bus is it sitting on? Which configuration options do I need and where can they be set?

## Finding the Registers
Nothing was copied from example code. Each address was derived from the manuals:
- Memory map (RM0390, Memory and bus architecture): The memory map table gives the base address for each peripheral and shows which bus it lives on. The GPIO ports are on the AHB1 bus.
- RCC (RM0390, Reset and clock control): GPIO peripherals are clock-gated after reset, so the port's clock must be enabled first. This is done with the port's enable bit in `RCC_AHB1ENR` found at the RCC base address (`0x40023800`) plus the offset from the register map (`0x30`)
- GPIO registers (RM0390, General-purpose I/Os): Each port has a block of registers at fixed offsets from the port base address (`0x40020000`)

| Register    | Offset | Purpose                                                                      |
| ----------- | ------ | ---------------------------------------------------------------------------- |
| GPIOx_MODER | 0x00   | Configures for input, output, alternate function, or analog (2 bits per pin) |
| GPIOx_ODR   | 0x14   | Output data                                                                  |
| GPIOx_BSRR  | 0x18   | Atomic bit set/reset                                                         |
## The Code
We are using GPIOA and pin 5.
```c
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
```

A few design notes:
- I'm using CubeIDE to flash this onto my board, writing my own linker script and startup file is a little out of scope for this project
- `volatile` is important to ensure the compiler does not optimize away or reorder the hardware registers.
- Clock enable comes first. Writing to the GPIO register before enabling the clock silently does nothing.
- For the delay I just wrote a crude busy-wait loop, this is fine for a simple LED blinky project but for anything more it is not recommended
- I chose to use `ODR` instead of `BSRR` because it makes the code a little simpler being able to toggle the same bit with `^=` instead of writing to two different bits with `BSRR`. If this were a more complex project that might touch the same pin from an interrupt or we needed to read from the pin I would choose `BSRR`.

## Bugs and Troubleshooting
### The Problem
I ran into an issue with my first version of the project when I first flashed the board, my LED did not turn on at all. The code matched the reference manual, the clock was enabled, the wiring made sense and yet the LED misbehaved.

### Diagnosis
I first verified with a multimeter that the circuit was able to flow with electricity and then I tested the LED to make sure it wasn't a dud. Everything turned out fine so far.

So if the problem wasn't the code and it wasn't the hardware used in the circuit what could possibly have gone wrong?

After a little bit of internet research of forums and pouring through the documentation more I found out in the user manual that the problem was with the pin I had selected. Even though it was a valid GPIO pin it was conflicting with the on-board ST-LINK debugger/programmer. The pin was being over-written with information through the ST-LINK even though I had set the pin to general purpose output! In the Nucleo user manual (UM1724) there is a section about solder-bridge tables that show which MCU pin is wired to ST-LINK circuitry. 

### Resolution
I moved the LED to a pin with no ST-LINK connection, confirmed against the schematic, and updated the code to match. The pin I had chosen is also connected to the on-board LED `LD2` but still serves as a GPIO without interference (so now I have two blinking LEDs).

## Lessons Learned
1. Read the board docs, not just the chip docs. A pin that is free on the chip may not be free on the board
2. Reading the reference manual is a skill in itself. Being able to navigate the docs effectively and knowing exactly where to go for what you want is a huge advantage in saving time and effort.
3. Clock gating is easy to forget and hard to debug, because it happens silently. Peripheral clocks are off by default. 
4. Embedded C code is a lot different than systems C code

## Possible Next Steps:
- Replace the busy-wait delay with the SysTick timer or a hardware timer
- Drive the LED with PWM output for brightness control
- Add a button using `IDR` and EXTI interrupts
- Write a minimal inker script and startup file instead of relying on vendor-provided files
- Write a UART driver to relay information about the LED state
