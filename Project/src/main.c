/*

ECE 355: Microprocessor-Based Systems

Jacob Arychuk
Benjamin B Jackson

*/

#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include "cmsis/cmsis_device.h"
	
// Sample pragmas to cope with warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

// Global variables
unsigned int frequency = 0;    // Calculated frequency from either function generator or NE555 timer
unsigned int resistance = 0;   // Resistance from potentiometer
unsigned int input_signal = 1; // Using input EXTI1(NE555 timer)/EXTI2(function generator) = 0/1
unsigned int first_edge = 0;   // Handle first/second = 0/1 edge of input signal

#define TIM2_PRESCALER ((uint16_t)0x0000)      // Clock prescaler for TIM2 timer (no prescaling)
#define TIM2_PERIOD ((uint32_t)0xFFFFFFFF)     // Maximum possible setting for overflow
#define TIM3_PRESCALER ((uint16_t)(48000 - 1)) // Clock prescaler for TIM3 timer (milliseconds)
#define TIM3_PERIOD ((uint32_t)0xFFFFFFFF)     // Maximum possible setting for overflow

SPI_HandleTypeDef SPI_Handle;

void GPIOA_Init(void);
void GPIOB_Init(void);
void TIM2_Init(void);
void TIM3_Init(void);
void ADC_Init(void);
void DAC_Init(void);
void EXTI_Init(void);
void OLED_Write_Cmd(unsigned char);
void OLED_Write_Data(unsigned char);
void OLED_Write(unsigned char);
void OLED_Config(void);
void Refresh_OLED(void);
void Delay(uint32_t time);

// Initialization commands for OLED display
unsigned char OLED_Init_Cmds[] = {
    0xAE,
    0x20, 0x00,
    0x40,
    0xA0 | 0x01,
    0xA8, 0x40 - 1,
    0xC0 | 0x08,
    0xD3, 0x00,
    0xDA, 0x32,
    0xD5, 0x80,
    0xD9, 0x22,
    0xDB, 0x30,
    0x81, 0xFF,
    0xA4,
    0xA6,
    0xAD, 0x30,
    0x8D, 0x10,
    0xAE | 0x01,
    0xC0,
    0xA0
};

// Character specifications for OLED display (1 row = 8 bytes = 1 ASCII character)
unsigned char Characters[][8] = {
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // SPACE
    {0b00000000, 0b00000000, 0b01011111, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // !
    {0b00000000, 0b00000111, 0b00000000, 0b00000111, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // "
    {0b00010100, 0b01111111, 0b00010100, 0b01111111, 0b00010100,0b00000000, 0b00000000, 0b00000000}, // #
    {0b00100100, 0b00101010, 0b01111111, 0b00101010, 0b00010010,0b00000000, 0b00000000, 0b00000000}, // $
    {0b00100011, 0b00010011, 0b00001000, 0b01100100, 0b01100010,0b00000000, 0b00000000, 0b00000000}, // %
    {0b00110110, 0b01001001, 0b01010101, 0b00100010, 0b01010000,0b00000000, 0b00000000, 0b00000000}, // &
    {0b00000000, 0b00000101, 0b00000011, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // '
    {0b00000000, 0b00011100, 0b00100010, 0b01000001, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // (
    {0b00000000, 0b01000001, 0b00100010, 0b00011100, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // )
    {0b00010100, 0b00001000, 0b00111110, 0b00001000, 0b00010100,0b00000000, 0b00000000, 0b00000000}, // *
    {0b00001000, 0b00001000, 0b00111110, 0b00001000, 0b00001000,0b00000000, 0b00000000, 0b00000000}, // +
    {0b00000000, 0b01010000, 0b00110000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // ,
    {0b00001000, 0b00001000, 0b00001000, 0b00001000, 0b00001000,0b00000000, 0b00000000, 0b00000000}, // -
    {0b00000000, 0b01100000, 0b01100000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // .
    {0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010,0b00000000, 0b00000000, 0b00000000}, // /
    {0b00111110, 0b01010001, 0b01001001, 0b01000101, 0b00111110,0b00000000, 0b00000000, 0b00000000}, // 0
    {0b00000000, 0b01000010, 0b01111111, 0b01000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // 1
    {0b01000010, 0b01100001, 0b01010001, 0b01001001, 0b01000110,0b00000000, 0b00000000, 0b00000000}, // 2
    {0b00100001, 0b01000001, 0b01000101, 0b01001011, 0b00110001,0b00000000, 0b00000000, 0b00000000}, // 3
    {0b00011000, 0b00010100, 0b00010010, 0b01111111, 0b00010000,0b00000000, 0b00000000, 0b00000000}, // 4
    {0b00100111, 0b01000101, 0b01000101, 0b01000101, 0b00111001,0b00000000, 0b00000000, 0b00000000}, // 5
    {0b00111100, 0b01001010, 0b01001001, 0b01001001, 0b00110000,0b00000000, 0b00000000, 0b00000000}, // 6
    {0b00000011, 0b00000001, 0b01110001, 0b00001001, 0b00000111,0b00000000, 0b00000000, 0b00000000}, // 7
    {0b00110110, 0b01001001, 0b01001001, 0b01001001, 0b00110110,0b00000000, 0b00000000, 0b00000000}, // 8
    {0b00000110, 0b01001001, 0b01001001, 0b00101001, 0b00011110,0b00000000, 0b00000000, 0b00000000}, // 9
    {0b00000000, 0b00110110, 0b00110110, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // :
    {0b00000000, 0b01010110, 0b00110110, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // ;
    {0b00001000, 0b00010100, 0b00100010, 0b01000001, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // <
    {0b00010100, 0b00010100, 0b00010100, 0b00010100, 0b00010100,0b00000000, 0b00000000, 0b00000000}, // =
    {0b00000000, 0b01000001, 0b00100010, 0b00010100, 0b00001000,0b00000000, 0b00000000, 0b00000000}, // >
    {0b00000010, 0b00000001, 0b01010001, 0b00001001, 0b00000110,0b00000000, 0b00000000, 0b00000000}, // ?
    {0b00110010, 0b01001001, 0b01111001, 0b01000001, 0b00111110,0b00000000, 0b00000000, 0b00000000}, // @
    {0b01111110, 0b00010001, 0b00010001, 0b00010001, 0b01111110,0b00000000, 0b00000000, 0b00000000}, // A
    {0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b00110110,0b00000000, 0b00000000, 0b00000000}, // B
    {0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00100010,0b00000000, 0b00000000, 0b00000000}, // C
    {0b01111111, 0b01000001, 0b01000001, 0b00100010, 0b00011100,0b00000000, 0b00000000, 0b00000000}, // D
    {0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b01000001,0b00000000, 0b00000000, 0b00000000}, // E
    {0b01111111, 0b00001001, 0b00001001, 0b00001001, 0b00000001,0b00000000, 0b00000000, 0b00000000}, // F
    {0b00111110, 0b01000001, 0b01001001, 0b01001001, 0b01111010,0b00000000, 0b00000000, 0b00000000}, // G
    {0b01111111, 0b00001000, 0b00001000, 0b00001000, 0b01111111,0b00000000, 0b00000000, 0b00000000}, // H
    {0b01000000, 0b01000001, 0b01111111, 0b01000001, 0b01000000,0b00000000, 0b00000000, 0b00000000}, // I
    {0b00100000, 0b01000000, 0b01000001, 0b00111111, 0b00000001,0b00000000, 0b00000000, 0b00000000}, // J
    {0b01111111, 0b00001000, 0b00010100, 0b00100010, 0b01000001,0b00000000, 0b00000000, 0b00000000}, // K
    {0b01111111, 0b01000000, 0b01000000, 0b01000000, 0b01000000,0b00000000, 0b00000000, 0b00000000}, // L
    {0b01111111, 0b00000010, 0b00001100, 0b00000010, 0b01111111,0b00000000, 0b00000000, 0b00000000}, // M
    {0b01111111, 0b00000100, 0b00001000, 0b00010000, 0b01111111,0b00000000, 0b00000000, 0b00000000}, // N
    {0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00111110,0b00000000, 0b00000000, 0b00000000}, // O
    {0b01111111, 0b00001001, 0b00001001, 0b00001001, 0b00000110,0b00000000, 0b00000000, 0b00000000}, // P
    {0b00111110, 0b01000001, 0b01010001, 0b00100001, 0b01011110,0b00000000, 0b00000000, 0b00000000}, // Q
    {0b01111111, 0b00001001, 0b00011001, 0b00101001, 0b01000110,0b00000000, 0b00000000, 0b00000000}, // R
    {0b01000110, 0b01001001, 0b01001001, 0b01001001, 0b00110001,0b00000000, 0b00000000, 0b00000000}, // S
    {0b00000001, 0b00000001, 0b01111111, 0b00000001, 0b00000001,0b00000000, 0b00000000, 0b00000000}, // T
    {0b00111111, 0b01000000, 0b01000000, 0b01000000, 0b00111111,0b00000000, 0b00000000, 0b00000000}, // U
    {0b00011111, 0b00100000, 0b01000000, 0b00100000, 0b00011111,0b00000000, 0b00000000, 0b00000000}, // V
    {0b00111111, 0b01000000, 0b00111000, 0b01000000, 0b00111111,0b00000000, 0b00000000, 0b00000000}, // W
    {0b01100011, 0b00010100, 0b00001000, 0b00010100, 0b01100011,0b00000000, 0b00000000, 0b00000000}, // X
    {0b00000111, 0b00001000, 0b01110000, 0b00001000, 0b00000111,0b00000000, 0b00000000, 0b00000000}, // Y
    {0b01100001, 0b01010001, 0b01001001, 0b01000101, 0b01000011,0b00000000, 0b00000000, 0b00000000}, // Z
    {0b01111111, 0b01000001, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // [
    {0b00010101, 0b00010110, 0b01111100, 0b00010110, 0b00010101,0b00000000, 0b00000000, 0b00000000}, // back slash
    {0b00000000, 0b00000000, 0b00000000, 0b01000001, 0b01111111,0b00000000, 0b00000000, 0b00000000}, // ]
    {0b00000100, 0b00000010, 0b00000001, 0b00000010, 0b00000100,0b00000000, 0b00000000, 0b00000000}, // ^
    {0b01000000, 0b01000000, 0b01000000, 0b01000000, 0b01000000,0b00000000, 0b00000000, 0b00000000}, // _
    {0b00000000, 0b00000001, 0b00000010, 0b00000100, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // `
    {0b00100000, 0b01010100, 0b01010100, 0b01010100, 0b01111000,0b00000000, 0b00000000, 0b00000000}, // a
    {0b01111111, 0b01001000, 0b01000100, 0b01000100, 0b00111000,0b00000000, 0b00000000, 0b00000000}, // b
    {0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00100000,0b00000000, 0b00000000, 0b00000000}, // c
    {0b00111000, 0b01000100, 0b01000100, 0b01001000, 0b01111111,0b00000000, 0b00000000, 0b00000000}, // d
    {0b00111000, 0b01010100, 0b01010100, 0b01010100, 0b00011000,0b00000000, 0b00000000, 0b00000000}, // e
    {0b00001000, 0b01111110, 0b00001001, 0b00000001, 0b00000010,0b00000000, 0b00000000, 0b00000000}, // f
    {0b00001100, 0b01010010, 0b01010010, 0b01010010, 0b00111110,0b00000000, 0b00000000, 0b00000000}, // g
    {0b01111111, 0b00001000, 0b00000100, 0b00000100, 0b01111000,0b00000000, 0b00000000, 0b00000000}, // h
    {0b00000000, 0b01000100, 0b01111101, 0b01000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // i
    {0b00100000, 0b01000000, 0b01000100, 0b00111101, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // j
    {0b01111111, 0b00010000, 0b00101000, 0b01000100, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // k
    {0b00000000, 0b01000001, 0b01111111, 0b01000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // l
    {0b01111100, 0b00000100, 0b00011000, 0b00000100, 0b01111000,0b00000000, 0b00000000, 0b00000000}, // m
    {0b01111100, 0b00001000, 0b00000100, 0b00000100, 0b01111000,0b00000000, 0b00000000, 0b00000000}, // n
    {0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00111000,0b00000000, 0b00000000, 0b00000000}, // o
    {0b01111100, 0b00010100, 0b00010100, 0b00010100, 0b00001000,0b00000000, 0b00000000, 0b00000000}, // p
    {0b00001000, 0b00010100, 0b00010100, 0b00011000, 0b01111100,0b00000000, 0b00000000, 0b00000000}, // q
    {0b01111100, 0b00001000, 0b00000100, 0b00000100, 0b00001000,0b00000000, 0b00000000, 0b00000000}, // r
    {0b01001000, 0b01010100, 0b01010100, 0b01010100, 0b00100000,0b00000000, 0b00000000, 0b00000000}, // s
    {0b00000100, 0b00111111, 0b01000100, 0b01000000, 0b00100000,0b00000000, 0b00000000, 0b00000000}, // t
    {0b00111100, 0b01000000, 0b01000000, 0b00100000, 0b01111100,0b00000000, 0b00000000, 0b00000000}, // u
    {0b00011100, 0b00100000, 0b01000000, 0b00100000, 0b00011100,0b00000000, 0b00000000, 0b00000000}, // v
    {0b00111100, 0b01000000, 0b00111000, 0b01000000, 0b00111100,0b00000000, 0b00000000, 0b00000000}, // w
    {0b01000100, 0b00101000, 0b00010000, 0b00101000, 0b01000100,0b00000000, 0b00000000, 0b00000000}, // x
    {0b00001100, 0b01010000, 0b01010000, 0b01010000, 0b00111100,0b00000000, 0b00000000, 0b00000000}, // y
    {0b01000100, 0b01100100, 0b01010100, 0b01001100, 0b01000100,0b00000000, 0b00000000, 0b00000000}, // z
    {0b00000000, 0b00001000, 0b00110110, 0b01000001, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // {
    {0b00000000, 0b00000000, 0b01111111, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // |
    {0b00000000, 0b01000001, 0b00110110, 0b00001000, 0b00000000,0b00000000, 0b00000000, 0b00000000}, // }
    {0b00001000, 0b00001000, 0b00101010, 0b00011100, 0b00001000,0b00000000, 0b00000000, 0b00000000}, // ~
    {0b00001000, 0b00011100, 0b00101010, 0b00001000, 0b00001000,0b00000000, 0b00000000, 0b00000000}  // <-
};

// Call this function to boost the STM32F0xx clock to 48 MHz
void SystemClock48MHz (void) {

    // Disable the PLL
    RCC->CR &= ~(RCC_CR_PLLON);

    // Wait for the PLL to unlock
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);

    // Configure the PLL for 48-MHz system clock
    RCC->CFGR = 0x00280000;

    // Enable the PLL
    RCC->CR |= RCC_CR_PLLON;

    // Wait for the PLL to lock
    while ((RCC->CR & RCC_CR_PLLRDY) != RCC_CR_PLLRDY);

    // Switch the processor to the PLL clock source
    RCC->CFGR = (RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;

    // Update the system with the new clock frequency
    SystemCoreClockUpdate();

}

int main (int argc, char* argv[]) {

    SystemClock48MHz();

    GPIOA_Init();
    GPIOB_Init();
    TIM2_Init();
    TIM3_Init();
    ADC_Init();
    DAC_Init();
    EXTI_Init();
    OLED_Config();

	while (1) {

        // Start conversion process
        ADC1->CR |= 0x4;

        // Wait until end of conversion flag is set
        while ((ADC1->ISR & 0x2) == 0);

        // Read the low 12 bits from data register
        unsigned int adc_value = (ADC1->DR & 0xFFF);

        resistance = (int)((adc_value * 5000 ) / 4095); // Max resistance is 5 kΩ

        // Send value from ADC to DAC
        DAC1->DHR12R1 = ADC1->DR & 0xFFF;

        Refresh_OLED();

    }

    return 0;

}

void GPIOA_Init () {

    // Enable clock for GPIOA
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    // Configure PA0-2 as input mode
    GPIOA->MODER &= ~(GPIO_MODER_MODER0);
    GPIOA->MODER &= ~(GPIO_MODER_MODER1);
    GPIOA->MODER &= ~(GPIO_MODER_MODER2);

    // Set no pull-up/pull-down for PA0-2
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR1);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR2);

}

void GPIOB_Init (void) {

    // Enable Clock for GPIOB
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Configure PB3 and PB5 as alternate function (AF) mode
    GPIOB->MODER |= (GPIO_MODER_MODER3_1 | GPIO_MODER_MODER5_1);

    // Set PB3 and PB5 to AF0
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL3 | GPIO_AFRL_AFSEL5);

    // Set no pull-up/pull-down for PB3 and PB5
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR3 | GPIO_PUPDR_PUPDR5);

    // Configure PB4, PB6, and PB7 as output mode
    GPIOB->MODER |= (GPIO_MODER_MODER4_0 | GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0);

    // Set no pull-up/pull-down for PB4, PB6, and PB7
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR4 | GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7);

}

void TIM2_Init (void) {

    // Enable clock for TIM2
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // Set configuration: buffer TIM2_ARR register, count up, stop on overflow, generate interrupt only on overflow/underflow
    TIM2->CR1 = ((uint16_t)0x008C);

    // Set clock prescaler value
    TIM2->PSC = TIM2_PRESCALER;

    // Set auto-reloading delay
    TIM2->ARR = TIM2_PERIOD;

    // Update timer registers
    TIM2->EGR = ((uint16_t)0x0001);

    // Assign TIM2 interrupt priority in NVIC
    NVIC_SetPriority(TIM2_IRQn, 0);

    // Enable TIM2 interrupts in NVIC
    NVIC_EnableIRQ(TIM2_IRQn);

    // Enable TIM2 interrupts
    TIM2->DIER |= TIM_DIER_UIE;

}

void TIM2_IRQHandler (void) {

    // Check if update interrupt flag is not set
    if ((TIM2->SR & TIM_SR_UIF) == 0) return;

    // Clear update interrupt flag
    TIM2->SR &= ~(TIM_SR_UIF);

    // Restart stopped timer
    TIM2->CR1 |= TIM_CR1_CEN;

}

void TIM3_Init (void) {

    // Enable clock for TIM3
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Set configuration: buffer TIM2_ARR register, count up
    TIM3->CR1 = ((uint16_t)0x0080);

    // Set clock prescaler value
    TIM3->PSC = TIM3_PRESCALER;

    // Set auto-reloading delay
    TIM3->ARR = 0xFFFF;

    // Update timer registers
    TIM3->EGR = ((uint16_t)0x0001);

}

void TIM3_IRQHandler (void) {

    // Check if update interrupt flag is not set
    if ((TIM3->SR & TIM_SR_UIF) == 0) return;

    // Clear update interrupt flag
    TIM3->SR &= ~(TIM_SR_UIF);

    // Restart stopped timer
    TIM3->CR1 |= TIM_CR1_CEN;

}

void ADC_Init () {

    // Enable clock for ADC
    RCC->APB2ENR |= 0x200;

    // Configure PA5 as analog mode
    GPIOA->MODER |= 0xC00;

    // Configure ADC
    ADC1->CFGR1 &= 0xFFFFFFE7; // Set 12-bit resolution
    ADC1->CFGR1 &= 0xFFFFFFDF; // Set right-aligned data
    ADC1->CFGR1 |= 0x1000;     // Set to overwrite contents when an overrun is detected
    ADC1->CFGR1 |= 0x2000;     // Enable continuous conversion mode

    // Select ADC_IN5 for conversion
    ADC1->CHSELR |= 0x20;

    // Set maximum sampling time
    ADC1->SMPR |= 0x7;

    // Enable ADC process
    ADC1->CR |= 0x1;

    // Wait until ADC ready flag is set
    while (((ADC1->ISR & 0x1) == 0));

}

void DAC_Init () {

    // Enable clock for DAC
    RCC->APB1ENR |= 0x20000000;

    // Configure PA4 as analog mode
    GPIOA->MODER |= 0x300;

    // Enable output channel
    DAC1->CR |= 0x1;

    // Enable output buffer and disable trigger
    DAC1->CR &= 0xFFFFFFF9;

}

void EXTI_Init () {

    // Map EXTI0-2 to PA0-2
    SYSCFG->EXTICR[0] &= ((uint16_t)0xF000);

    // Set rising-edge trigger for EXTI0-2
    EXTI->RTSR |= ((uint32_t)0x00000007);

    // Unmask interrupts from EXTI0 line
    EXTI->IMR |= ((uint32_t)0x00000001);

    // Mask interrupts from EXTI1 line (not starting with NE555 timer)
    EXTI->IMR &= 0xFFFFFFFD;

    // Unmask interrupts from EXTI2 line (starting with function generator)
    EXTI->IMR |= 0x4;

    // Set EXTI0-3 interrupt priority in NVIC
    NVIC_SetPriority(EXTI0_1_IRQn, 0);
    NVIC_SetPriority(EXTI2_3_IRQn, 0);

    // Enable EXTI interrupts in NVIC
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_EnableIRQ(EXTI2_3_IRQn);

}

void EXTI0_1_IRQHandler () {

    // Check if interrupt pending flag is set for EXTI0 (button)
    if ((EXTI->PR & EXTI_PR_PR0) != 0) {

        // Switch input: EXTI1(NE555 timer)/EXTI2(function generator) = 0/1
        input_signal ^= 0x1;

        // Use EXTI1 (NE555 timer)
        if (input_signal == 0) {
            // Unmask interrupts from EXTI1 line
            EXTI->IMR |= 0x2;
            // Mask interrupts from EXTI2 line
            EXTI->IMR &= 0xFFFFFFFFB;
        }

        // Use EXTI2 (function generator)
        else {
            // Mask interrupts from EXTI1 line
            EXTI->IMR &= 0xFFFFFFFD;
            // Unmask interrupts from EXTI2 line
            EXTI->IMR |= 0x4;
        }

        // Clear interrupt pending flag for EXTI0
        EXTI->PR |= ((uint32_t)0x00000001);

    }

    // Check if interrupt pending flag is set for EXTI1 (NE555 timer)
    if ((EXTI->PR & EXTI_PR_PR1) != 0) {

        // Handle first edge
        if (first_edge == 0) {
            // Clear count register
            TIM2->CNT = ((uint32_t)0x00000000);
            // Start timer
            TIM2->CR1 |= ((uint16_t)0x0001);
            // Update flag
            first_edge = 1;
        }

        // Handle second edge
        else {
            // Stop timer
            TIM2->CR1 &= ((uint16_t)0xFFFE);
            // Read out count register
            unsigned int count = TIM2->CNT;
            // Period calculation: divide count by 48 MHz to get ms
            double period = count/48.0;
            // Frequency calculation: divide 1 by period and multiply by 1,000,000 to get Hz
            frequency = (unsigned int)((1000000.0 / period) + 0.5);
            // Reset flag
            first_edge = 0;
        }

        // Clear interrupt pending flag for EXTI1
        EXTI->PR |= ((uint32_t)0x00000002);

    }

}

void EXTI2_3_IRQHandler () {

    // Check if interrupt pending flag is set for EXTI2 (function generator)
    if ((EXTI->PR & EXTI_PR_PR2) != 0) {

        // Handle first edge
        if (first_edge == 0) {
            // Clear count register
            TIM2->CNT = ((uint32_t)0x00000000);
            // Start timer
            TIM2->CR1 |= ((uint16_t)0x0001);
            // Update flag
            first_edge = 1;
        }

        // Handle second edge
        else {
            // Stop timer
            TIM2->CR1 &= ((uint16_t)0xFFFE);
            // Read out count register
            unsigned int count = TIM2->CNT;
            // Period calculation: divide count by 48 MHz to get microseconds
            double period = count/48.0
            // Frequency calculation: divide 1 by period and multiply by 1,000,000 to get hertz
            frequency = (unsigned int)((1000000.0 / period) + 0.5);
            // Reset flag
            first_edge = 0;
        }

        // Clear interrupt pending flag for EXTI2
        EXTI->PR |= ((uint32_t)0x00000004);

    }

}

void OLED_Write_Cmd (unsigned char cmd) {

    // Set PB6 = CS# = 1
    GPIOB->BSRR |= GPIO_BSRR_BS_6;

    // Set PB7 = D/C# = 0
    GPIOB->BSRR |= GPIO_BSRR_BR_7;

    // Set PB6 = CS# = 0
    GPIOB->BSRR |= GPIO_BSRR_BR_6;

    OLED_Write(cmd);

    // Set PB6 = CS# = 1
    GPIOB->BSRR |= GPIO_BSRR_BS_6;

}

void OLED_Write_Data (unsigned char data) {

    // Set PB6 = CS# = 1
    GPIOB->BSRR |= GPIO_BSRR_BS_6;

    // Set PB7 = D/C# = 1
    GPIOB->BSRR |= GPIO_BSRR_BS_7;

    //Set PB6 = CS# = 0
    GPIOB->BSRR |= GPIO_BSRR_BR_6;

    OLED_Write(data);

    // Set PB6 = CS# = 1
    GPIOB->BSRR |= GPIO_BSRR_BS_6;

}

void OLED_Write (unsigned char Value) {

    // Wait until SPI is ready (TXE bit is set)
    while ((SPI1->SR & 0x2) == 0);

    // Send one byte (this function also sets BIDIOE = 1 in SPI1_CR1)
    HAL_SPI_Transmit(&SPI_Handle, &Value, 1, HAL_MAX_DELAY);

    // Wait until transmission is complete (TXE bit is set)
    while ((SPI1->SR & 0x2) == 0);

}

void OLED_Config (void) {

    // Enable clock for SPI
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // Set configuration
    SPI_Handle.Instance = SPI1;
    SPI_Handle.Init.Direction = SPI_DIRECTION_1LINE;
    SPI_Handle.Init.Mode = SPI_MODE_MASTER;
    SPI_Handle.Init.DataSize = SPI_DATASIZE_8BIT;
    SPI_Handle.Init.CLKPolarity = SPI_POLARITY_LOW;
    SPI_Handle.Init.CLKPhase = SPI_PHASE_1EDGE;
    SPI_Handle.Init.NSS = SPI_NSS_SOFT;
    SPI_Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    SPI_Handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    SPI_Handle.Init.CRCPolynomial = 7;

    // Initialize the SPI interface
    HAL_SPI_Init(&SPI_Handle);

    // Enable the SPI
    __HAL_SPI_ENABLE(&SPI_Handle);

    // Reset OLED display
    GPIOB->BSRR |= GPIO_BSRR_BR_4; 		          // PB4 (RES#) = 0
    Delay(3); 							          // Wait for 3 ms
    GPIOB->BSRR |= GPIO_BSRR_BS_4; 		          // PB4 (RES#) = 1
    Delay(3);							          // Wait for 3 ms

    // Send initialization commands to LED display
    for (unsigned int i = 0; i < sizeof(OLED_Init_Cmds); i++) {
        OLED_Write_Cmd(OLED_Init_Cmds[i]);
    }

    // Go through all pages to fill OLED display data memory with zeros
    for (unsigned char i = 0xB0; i < 0xB8; i++) {

        // Select PAGEi
        OLED_Write_Cmd(i);

        // Set SEG0 (lower half)
        OLED_Write_Cmd(0x02);

        // Set SEG0 (upper half)
        OLED_Write_Cmd(0x10);

        // Write zeros to every SEG (128 times)
        for (int j = 0; j < 128; j++) {
            OLED_Write_Data(0x00);
        }

    }

}

void Refresh_OLED (void) {

    // Buffer size (at most 16 characters per PAGE + null terminator)
    unsigned char buffer[17];

    // Format resistance value into the buffer
    snprintf((char*)buffer, sizeof(buffer), "R: %5u Ohms", resistance);

    // Select PAGE2
    OLED_Write_Cmd(0xB2);
    // Select SEG3 (lower half)
    OLED_Write_Cmd(0x03);
    // Select SEG3 (upper half)
    OLED_Write_Cmd(0x10);

    // Read characters from buffer until reaching null terminator
    for (int i = 0; buffer[i] != '\0'; i++) {
        // Retrieve 8 bytes and send them one at a time to OLED display
        for (int j = 0; j < 8; j++) {
            OLED_Write_Data(Characters[(int)buffer[i]][j]); // Cast buffer[i] to int to get ASCII code
        }
    }

    // Format frequency value into the buffer
    snprintf((char*)buffer, sizeof(buffer), "F: %5u Hz", frequency);

    // Select PAGE4
    OLED_Write_Cmd(0xB4);
    // Select SEG3 (lower half)
    OLED_Write_Cmd(0x03);
    // Select SEG3 (upper half)
    OLED_Write_Cmd(0x10);
    // Read characters from buffer until reaching null terminator
    for (int i = 0; buffer[i] != '\0'; i++) {
        // Retrieve 8 bytes and send them one at a time to LED display
        for (int j = 0; j < 8; j++) {
            OLED_Write_Data(Characters[(int)buffer[i]][j]); // Cast buffer[i] to int to get ASCII code
        }
    }

    // Wait for ~100 ms to get ~10 frames/sec refresh rate
    Delay(100);

}

void Delay (uint32_t time) {	// Milliseconds

    // Clear count register
    TIM3->CNT = ((uint32_t)0x00000000);

    // Start timer
    TIM3->CR1 |= TIM_CR1_CEN;

    // Wait for updated event
    while (TIM3->CNT < time);

    // Stop timer
    TIM3->CR1 &= ~TIM_CR1_CEN;

}

// Used to pop the compiler diagnostics status
#pragma GCC diagnostic pop
