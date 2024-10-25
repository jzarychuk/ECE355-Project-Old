/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include "cmsis/cmsis_device.h"

// ----------------------------------------------------------------------------
//
// STM32F0 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace-impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

void myADC_Init(void);
void myDAC_Init(void);

/*** Call this function to boost the STM32F0xx clock to 48 MHz ***/

void SystemClock48MHz( void )
{
//
// Disable the PLL
//
    RCC->CR &= ~(RCC_CR_PLLON);
//
// Wait for the PLL to unlock
//
    while (( RCC->CR & RCC_CR_PLLRDY ) != 0 );
//
// Configure the PLL for 48-MHz system clock
//
    RCC->CFGR = 0x00280000;
//
// Enable the PLL
//
    RCC->CR |= RCC_CR_PLLON;
//
// Wait for the PLL to lock
//
    while (( RCC->CR & RCC_CR_PLLRDY ) != RCC_CR_PLLRDY );
//
// Switch the processor to the PLL clock source
//
    RCC->CFGR = ( RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;
//
// Update the system with the new clock frequency
//
    SystemCoreClockUpdate();

}

int main(int argc, char* argv[]) {

	SystemClock48MHz();
	trace_printf("System clock: %u Hz\n", SystemCoreClock);

	myADC_Init();
	myDAC_Init();

	// Infinite loop
	while(1) {
		ADC1->CR |= 0x4;  // Set Bit 2 to start conversion process (see "Interfacing" Slide 8)
		while((ADC1->ISR & 0x2) == 0); // Wait for end of conversion flag (Bit 2) to be set
		unsigned int adc_value = (ADC1->DR & 0xFFF); // Read low 12 bits from ADC1 data register
		trace_printf("Value going to ADC from POT: %u\n", adc_value); // Print value (0-4095) (2^12)
		DAC1->DHR12R1 = ADC1->DR & 0xFFF; // Send value from ADC to DAC
	}
}

void myADC_Init() {

	// Enable clock
	RCC->APB2ENR |= 0x200; // Set Bit 9 (see "Interfacing" Slide 15)

	// Configure PA5 as analog
	GPIOA->MODER |= 0xC00; // Set Bit 10-11 (see "I/O" Slide 25)

	// Configure ADC
	ADC1->CFGR1 &= 0xFFFFFFE7; // Clear Bit 3-4 to choose 12-bit resolution (see "Interfacing" Slide 10)
	ADC1->CFGR1 &= 0xFFFFFFDF; // Clear Bit 5 for right-aligned data (see "Interfacing" Slide 10)
	ADC1->CFGR1 |= 0x1000; // Set Bit 12 to overwrite when overrun detected (see "Interfacing" Slide 10)
	ADC1->CFGR1 |= 0x2000; // Set Bit 13 for continuous conversion mode (see "Interfacing" Slide 10)

	// Select Channel 5 for conversion
	ADC1->CHSELR |= 0x20;// Set Bit 5 (see "Interfacing" Slide 9)

	// Enable taking as many clock cycles as necessary to get reliable sample of analog signal
	ADC1->SMPR |= 0x7; //Set Bit 0-2 (see "Interfacing" Slide 9)

	// Enable ADC process (basic initialization)
	ADC1->CR |= 0x1; // Set Bit 0 (see "Interfacing" Slide 8)

	while(((ADC1->ISR & 0x1) == 0)); // Wait for ADC ready flag to be set

}

void myDAC_Init() {

	// Enable clock
	RCC->APB1ENR |= 0x20000000; // Set Bit 29 (see "Interfacing" Slide 15)

	// Configure PA4 as analog
	GPIOA->MODER |= 0x300; // Set Bit 8-9 (see "I/O" Slide 25)

	// Configure DAC
	DAC1->CR |= 0x1; // Set Bit 0 to enable Channel 1 (see "Interfacing" Slide 14)
	DAC1->CR &= 0xFFFFFFF9; // Clear Bit 1-2 to enable output buffer and disable trigger (see "Interfacing" Slide 14)

}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
