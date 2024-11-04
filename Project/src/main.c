#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include "cmsis/cmsis_device.h"

// Sample pragmas to cope with warnings. Please note the related line at the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

unsigned int freq = 0;
unsigned int res = 0;
unsigned int inSig = 0; // Using input EXTI1(NE555 timer)/EXTI2(function generator) = 0/1
unsigned int first_edge = 1;  // Handle first/second = 1/0 edge of input signal

#define myTIM2_PRESCALER ((uint16_t)0x0000) // Clock prescaler for TIM2 timer (no prescaling)
#define myTIM2_PERIOD ((uint32_t)0xFFFFFFFF) // Maximum possible setting for overflow

void myGPIOA_Init(void);
void myADC_Init(void);
void myDAC_Init(void);
void myTIM2_Init(void);
void myEXTI_Init(void);

void oled_Write(unsigned char);
void oled_Write_Cmd(unsigned char);
void oled_Write_Data(unsigned char);
void oled_config(void);
void refresh_OLED(void);

SPI_HandleTypeDef SPI_Handle;

// Initialization commands for LED display
unsigned char oled_init_cmds[] = {
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

// Character specifications for LED display (1 row = 8 bytes = 1 ASCII character)
unsigned char Characters[][8] = {
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // SPACE
    {0b00000000, 0b00000000, 0b01011111, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // !
    {0b00000000, 0b00000111, 0b00000000, 0b00000111, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // "
    {0b00010100, 0b01111111, 0b00010100, 0b01111111, 0b00010100,0b00000000, 0b00000000, 0b00000000},  // #
    {0b00100100, 0b00101010, 0b01111111, 0b00101010, 0b00010010,0b00000000, 0b00000000, 0b00000000},  // $
    {0b00100011, 0b00010011, 0b00001000, 0b01100100, 0b01100010,0b00000000, 0b00000000, 0b00000000},  // %
    {0b00110110, 0b01001001, 0b01010101, 0b00100010, 0b01010000,0b00000000, 0b00000000, 0b00000000},  // &
    {0b00000000, 0b00000101, 0b00000011, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // '
    {0b00000000, 0b00011100, 0b00100010, 0b01000001, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // (
    {0b00000000, 0b01000001, 0b00100010, 0b00011100, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // )
    {0b00010100, 0b00001000, 0b00111110, 0b00001000, 0b00010100,0b00000000, 0b00000000, 0b00000000},  // *
    {0b00001000, 0b00001000, 0b00111110, 0b00001000, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // +
    {0b00000000, 0b01010000, 0b00110000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // ,
    {0b00001000, 0b00001000, 0b00001000, 0b00001000, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // -
    {0b00000000, 0b01100000, 0b01100000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // .
    {0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010,0b00000000, 0b00000000, 0b00000000},  // /
    {0b00111110, 0b01010001, 0b01001001, 0b01000101, 0b00111110,0b00000000, 0b00000000, 0b00000000},  // 0
    {0b00000000, 0b01000010, 0b01111111, 0b01000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // 1
    {0b01000010, 0b01100001, 0b01010001, 0b01001001, 0b01000110,0b00000000, 0b00000000, 0b00000000},  // 2
    {0b00100001, 0b01000001, 0b01000101, 0b01001011, 0b00110001,0b00000000, 0b00000000, 0b00000000},  // 3
    {0b00011000, 0b00010100, 0b00010010, 0b01111111, 0b00010000,0b00000000, 0b00000000, 0b00000000},  // 4
    {0b00100111, 0b01000101, 0b01000101, 0b01000101, 0b00111001,0b00000000, 0b00000000, 0b00000000},  // 5
    {0b00111100, 0b01001010, 0b01001001, 0b01001001, 0b00110000,0b00000000, 0b00000000, 0b00000000},  // 6
    {0b00000011, 0b00000001, 0b01110001, 0b00001001, 0b00000111,0b00000000, 0b00000000, 0b00000000},  // 7
    {0b00110110, 0b01001001, 0b01001001, 0b01001001, 0b00110110,0b00000000, 0b00000000, 0b00000000},  // 8
    {0b00000110, 0b01001001, 0b01001001, 0b00101001, 0b00011110,0b00000000, 0b00000000, 0b00000000},  // 9
    {0b00000000, 0b00110110, 0b00110110, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // :
    {0b00000000, 0b01010110, 0b00110110, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // ;
    {0b00001000, 0b00010100, 0b00100010, 0b01000001, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // <
    {0b00010100, 0b00010100, 0b00010100, 0b00010100, 0b00010100,0b00000000, 0b00000000, 0b00000000},  // =
    {0b00000000, 0b01000001, 0b00100010, 0b00010100, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // >
    {0b00000010, 0b00000001, 0b01010001, 0b00001001, 0b00000110,0b00000000, 0b00000000, 0b00000000},  // ?
    {0b00110010, 0b01001001, 0b01111001, 0b01000001, 0b00111110,0b00000000, 0b00000000, 0b00000000},  // @
    {0b01111110, 0b00010001, 0b00010001, 0b00010001, 0b01111110,0b00000000, 0b00000000, 0b00000000},  // A
    {0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b00110110,0b00000000, 0b00000000, 0b00000000},  // B
    {0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00100010,0b00000000, 0b00000000, 0b00000000},  // C
    {0b01111111, 0b01000001, 0b01000001, 0b00100010, 0b00011100,0b00000000, 0b00000000, 0b00000000},  // D
    {0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b01000001,0b00000000, 0b00000000, 0b00000000},  // E
    {0b01111111, 0b00001001, 0b00001001, 0b00001001, 0b00000001,0b00000000, 0b00000000, 0b00000000},  // F
    {0b00111110, 0b01000001, 0b01001001, 0b01001001, 0b01111010,0b00000000, 0b00000000, 0b00000000},  // G
    {0b01111111, 0b00001000, 0b00001000, 0b00001000, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // H
    {0b01000000, 0b01000001, 0b01111111, 0b01000001, 0b01000000,0b00000000, 0b00000000, 0b00000000},  // I
    {0b00100000, 0b01000000, 0b01000001, 0b00111111, 0b00000001,0b00000000, 0b00000000, 0b00000000},  // J
    {0b01111111, 0b00001000, 0b00010100, 0b00100010, 0b01000001,0b00000000, 0b00000000, 0b00000000},  // K
    {0b01111111, 0b01000000, 0b01000000, 0b01000000, 0b01000000,0b00000000, 0b00000000, 0b00000000},  // L
    {0b01111111, 0b00000010, 0b00001100, 0b00000010, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // M
    {0b01111111, 0b00000100, 0b00001000, 0b00010000, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // N
    {0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00111110,0b00000000, 0b00000000, 0b00000000},  // O
    {0b01111111, 0b00001001, 0b00001001, 0b00001001, 0b00000110,0b00000000, 0b00000000, 0b00000000},  // P
    {0b00111110, 0b01000001, 0b01010001, 0b00100001, 0b01011110,0b00000000, 0b00000000, 0b00000000},  // Q
    {0b01111111, 0b00001001, 0b00011001, 0b00101001, 0b01000110,0b00000000, 0b00000000, 0b00000000},  // R
    {0b01000110, 0b01001001, 0b01001001, 0b01001001, 0b00110001,0b00000000, 0b00000000, 0b00000000},  // S
    {0b00000001, 0b00000001, 0b01111111, 0b00000001, 0b00000001,0b00000000, 0b00000000, 0b00000000},  // T
    {0b00111111, 0b01000000, 0b01000000, 0b01000000, 0b00111111,0b00000000, 0b00000000, 0b00000000},  // U
    {0b00011111, 0b00100000, 0b01000000, 0b00100000, 0b00011111,0b00000000, 0b00000000, 0b00000000},  // V
    {0b00111111, 0b01000000, 0b00111000, 0b01000000, 0b00111111,0b00000000, 0b00000000, 0b00000000},  // W
    {0b01100011, 0b00010100, 0b00001000, 0b00010100, 0b01100011,0b00000000, 0b00000000, 0b00000000},  // X
    {0b00000111, 0b00001000, 0b01110000, 0b00001000, 0b00000111,0b00000000, 0b00000000, 0b00000000},  // Y
    {0b01100001, 0b01010001, 0b01001001, 0b01000101, 0b01000011,0b00000000, 0b00000000, 0b00000000},  // Z
    {0b01111111, 0b01000001, 0b00000000, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // [
    {0b00010101, 0b00010110, 0b01111100, 0b00010110, 0b00010101,0b00000000, 0b00000000, 0b00000000},  // back slash
    {0b00000000, 0b00000000, 0b00000000, 0b01000001, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // ]
    {0b00000100, 0b00000010, 0b00000001, 0b00000010, 0b00000100,0b00000000, 0b00000000, 0b00000000},  // ^
    {0b01000000, 0b01000000, 0b01000000, 0b01000000, 0b01000000,0b00000000, 0b00000000, 0b00000000},  // _
    {0b00000000, 0b00000001, 0b00000010, 0b00000100, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // `
    {0b00100000, 0b01010100, 0b01010100, 0b01010100, 0b01111000,0b00000000, 0b00000000, 0b00000000},  // a
    {0b01111111, 0b01001000, 0b01000100, 0b01000100, 0b00111000,0b00000000, 0b00000000, 0b00000000},  // b
    {0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00100000,0b00000000, 0b00000000, 0b00000000},  // c
    {0b00111000, 0b01000100, 0b01000100, 0b01001000, 0b01111111,0b00000000, 0b00000000, 0b00000000},  // d
    {0b00111000, 0b01010100, 0b01010100, 0b01010100, 0b00011000,0b00000000, 0b00000000, 0b00000000},  // e
    {0b00001000, 0b01111110, 0b00001001, 0b00000001, 0b00000010,0b00000000, 0b00000000, 0b00000000},  // f
    {0b00001100, 0b01010010, 0b01010010, 0b01010010, 0b00111110,0b00000000, 0b00000000, 0b00000000},  // g
    {0b01111111, 0b00001000, 0b00000100, 0b00000100, 0b01111000,0b00000000, 0b00000000, 0b00000000},  // h
    {0b00000000, 0b01000100, 0b01111101, 0b01000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // i
    {0b00100000, 0b01000000, 0b01000100, 0b00111101, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // j
    {0b01111111, 0b00010000, 0b00101000, 0b01000100, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // k
    {0b00000000, 0b01000001, 0b01111111, 0b01000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // l
    {0b01111100, 0b00000100, 0b00011000, 0b00000100, 0b01111000,0b00000000, 0b00000000, 0b00000000},  // m
    {0b01111100, 0b00001000, 0b00000100, 0b00000100, 0b01111000,0b00000000, 0b00000000, 0b00000000},  // n
    {0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00111000,0b00000000, 0b00000000, 0b00000000},  // o
    {0b01111100, 0b00010100, 0b00010100, 0b00010100, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // p
    {0b00001000, 0b00010100, 0b00010100, 0b00011000, 0b01111100,0b00000000, 0b00000000, 0b00000000},  // q
    {0b01111100, 0b00001000, 0b00000100, 0b00000100, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // r
    {0b01001000, 0b01010100, 0b01010100, 0b01010100, 0b00100000,0b00000000, 0b00000000, 0b00000000},  // s
    {0b00000100, 0b00111111, 0b01000100, 0b01000000, 0b00100000,0b00000000, 0b00000000, 0b00000000},  // t
    {0b00111100, 0b01000000, 0b01000000, 0b00100000, 0b01111100,0b00000000, 0b00000000, 0b00000000},  // u
    {0b00011100, 0b00100000, 0b01000000, 0b00100000, 0b00011100,0b00000000, 0b00000000, 0b00000000},  // v
    {0b00111100, 0b01000000, 0b00111000, 0b01000000, 0b00111100,0b00000000, 0b00000000, 0b00000000},  // w
    {0b01000100, 0b00101000, 0b00010000, 0b00101000, 0b01000100,0b00000000, 0b00000000, 0b00000000},  // x
    {0b00001100, 0b01010000, 0b01010000, 0b01010000, 0b00111100,0b00000000, 0b00000000, 0b00000000},  // y
    {0b01000100, 0b01100100, 0b01010100, 0b01001100, 0b01000100,0b00000000, 0b00000000, 0b00000000},  // z
    {0b00000000, 0b00001000, 0b00110110, 0b01000001, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // {
    {0b00000000, 0b00000000, 0b01111111, 0b00000000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // |
    {0b00000000, 0b01000001, 0b00110110, 0b00001000, 0b00000000,0b00000000, 0b00000000, 0b00000000},  // }
    {0b00001000, 0b00001000, 0b00101010, 0b00011100, 0b00001000,0b00000000, 0b00000000, 0b00000000},  // ~
    {0b00001000, 0b00011100, 0b00101010, 0b00001000, 0b00001000,0b00000000, 0b00000000, 0b00000000}   // <-
};

// Call this function to boost the STM32F0xx clock to 48 MHz
void SystemClock48MHz(void) {

    // Disable the PLL
    RCC->CR &= ~(RCC_CR_PLLON);

    // Wait for the PLL to unlock
    while (( RCC->CR & RCC_CR_PLLRDY ) != 0 );

    // Configure the PLL for 48-MHz system clock
    RCC->CFGR = 0x00280000;

    // Enable the PLL
    RCC->CR |= RCC_CR_PLLON;

    // Wait for the PLL to lock
    while (( RCC->CR & RCC_CR_PLLRDY ) != RCC_CR_PLLRDY );

    // Switch the processor to the PLL clock source
    RCC->CFGR = ( RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;

    // Update the system with the new clock frequency
    SystemCoreClockUpdate();

}

int main(int argc, char* argv[]) {

	SystemClock48MHz();
	trace_printf("System clock: %u Hz\n", SystemCoreClock);

	myGPIOA_Init();
	myADC_Init();
	myDAC_Init();
	myTIM2_Init();
	myEXTI_Init();
	//oled_config();

	// Infinite loop
	while(1) {
		
		// Set Bit 2 to start conversion process (see "Interfacing" Slide 8)
		ADC1->CR |= 0x4;
		
		// Wait for end of conversion flag (Bit 2) to be set
		while((ADC1->ISR & 0x2) == 0);
		
		// Read low 12 bits from ADC1 data register
		unsigned int adc_value = (ADC1->DR & 0xFFF);
		
		// Print value (0-4095)
		trace_printf("Value going to ADC from POT: %u\n", adc_value);

		// Send value from ADC to DAC
		DAC1->DHR12R1 = ADC1->DR & 0xFFF;
		
		//refresh_OLED();

	}

}

void myGPIOA_Init() {
	
	// Enable clock
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	
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

	// Wait for ADC ready flag to be set
	while(((ADC1->ISR & 0x1) == 0));

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

void myTIM2_Init() {
	
	// Enable clock
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// Configure: buffer auto-reload, count up, stop on overflow, enable update events, interrupt on overflow only
	TIM2->CR1 = ((uint16_t)0x008C);

	// Set clock prescaler value
	TIM2->PSC = myTIM2_PRESCALER;
	
	// Set auto-reloaded delay
	TIM2->ARR = myTIM2_PERIOD;

	// Update timer registers
	TIM2->EGR = ((uint16_t)0x0001);

	// Assign TIM2 interrupt priority = 0 in NVIC
	NVIC_SetPriority(TIM2_IRQn, 0); // Same as: NVIC->IP[3] = ((uint32_t)0x00FFFFFF);

	// Enable TIM2 interrupts in NVIC
	NVIC_EnableIRQ(TIM2_IRQn); // Same as: NVIC->ISER[0] = ((uint32_t)0x00008000) */

	// Enable update interrupt generation
	TIM2->DIER |= TIM_DIER_UIE;
	
	// Start counting timer pulses
	// TIM2->CR1 |= TIM_CR1_CEN; // THIS IS COMMENTED OUT IN THE LAB 2 CODE
	
}

void TIM2_IRQHandler() {
	
	// Check if update interrupt flag is set
	if ((TIM2->SR & TIM_SR_UIF) != 0) {
		
		trace_printf("\nOverflow!\n");

		// Clear update interrupt flag
		TIM2->SR &= ~(TIM_SR_UIF);

		// Restart stopped timer
		TIM2->CR1 |= TIM_CR1_CEN;
		
	}
	
}

void myEXTI_Init() {

	// Map EXTI0-2 lines to PA0-2
	SYSCFG->EXTICR[0] &= ((uint16_t)0xF000); //  to enable PA0-2 and keep other bits same 1111 0000 0000 0000 (see reference manual Page 172)

	// EXTI0-2 line interrupts: set rising-edge trigger
	EXTI->RTSR |= ((uint32_t)0x00000007);  // Set bits TR0-2 to 1 to enable (see reference manual Page 200)

	// Unmask interrupts from EXTI0-2 lines
	EXTI->IMR |= ((uint32_t)0x00000007); // Set bits MR0-2 to 1 to unmask (see reference manual Page 199)

	// Assign EXTI0-3 interrupt priority = 0 in NVIC (we are not using EXTI3)
	NVIC_SetPriority(EXTI0_1_IRQn, 0); // Found in header, setting lines 0-1 to 0 // (can also use NVIC->IP)
	NVIC_SetPriority(EXTI2_3_IRQn, 0); // Found in header, setting lines 2-3 to 0 // (can also use NVIC->IP)

	// Enable EXTI interrupts in NVIC (we are not using EXTI3)
	NVIC_EnableIRQ(EXTI0_1_IRQn); // Enable interrupts for EXTI0 (button) and EXTI1 (NE555 timer)
	NVIC_EnableIRQ(EXTI2_3_IRQn); // Enable interrupts for EXTI2 (function generator) and EXTI3 (not used)

}

// This handler is declared in system/src/cmsis/vectors_stm32f051x8.c
void EXTI0_1_IRQHandler() {
	
    volatile unsigned int count = 0;
    volatile double sig_period = 0;
    volatile double sig_frequency = 0;
    volatile unsigned int uint_sig_period = 0;
    volatile unsigned int uint_sig_frequency = 0;

    // Check if EXTI0 interrupt pending flag is set (for the button)
    if ((EXTI->PR & EXTI_PR_PR0) != 0) {
	
	// Clear EXTI0 interrupt pending flag
	EXTI->PR |= ((uint32_t)0x00000001); // A pending register (PR) bit is cleared by writing 1 to it

	// Switch input: EXTI1(NE555 timer)/EXTI2(function generator) = 0/1        
	inSig ^= 0x1;					

	// Using EXTI1 (NE555 timer)
	if(inSig == 0){
		// Unmask interrupts from EXTI1 line
		EXTI->IMR |= 0x2; // Set bit MR1 to 1 to unmask (see reference manual Page 199)
		// Mask interrupts from EXTI2 line
		EXTI->IMR &= 0xFFFFFFFFB; // Clear bit MR2 to 0 to mask (see reference manual Page 199)
	} 

	// Using EXTI2 (function generator)
	else {
		// Mask interrupts from EXTI1 line
		EXTI->IMR &= 0xFFFFFFFD; // Clear bit MR1 to 0 to mask (see reference manual Page 199)
		// Unmask interrupts from EXTI2 line
		EXTI->IMR |= 0x4; // Set bit MR2 to 1 to unmask (see reference manual Page 199)
	}
	    
    }

    // Check if EXTI1 interrupt pending flag is set (for the NE555 timer)
    if ((EXTI->PR & EXTI_PR_PR1) != 0) {
	
	// Clear EXTI1 interrupt pending flag
	EXTI->PR |= ((uint32_t)0x00000002); // A pending register (PR) bit is cleared by writing 1 to it

	// Handle first edge
	if(first_edge == 1) {
		// Clear count register
		TIM2->CNT = ((uint32_t)0x00000000);
		// Start timer
		TIM2->CR1 |= ((uint16_t)0x0001);
		// Update flag
		first_edge = 0;
	}
		
	// Handle second edge
	else {
		// Stop timer
		TIM2->CR1 &= ((uint16_t)0xFFFE);
		// Read out count register
		count = TIM2->CNT;
		// Calculate signal period by changing from CPU frequency (48MHz) to microseconds
		sig_period = count/48.0;
		// Calculate signal frequency (+0.5 for rounding)
		sig_frequency = (1000000.0/sig_period) + 0.5;
		// Convert to unsigned int for printing
		uint_sig_period = (unsigned int) sig_period;
		uint_sig_frequency = (unsigned int) sig_frequency;
		// Print calculated values to the console
		trace_printf("Signal (Function Generator) Period:    %u us\n", uint_sig_period);
		trace_printf("Signal (Function Generator) Frequency: %u Hz\n", uint_sig_frequency);
		// Reset flag
		first_edge = 1;
	}

    }

}

// This handler is declared in system/src/cmsis/vectors_stm32f051x8.c
void EXTI2_3_IRQHandler() {

    volatile unsigned int count = 0;
    volatile double sig_period = 0;
    volatile double sig_frequency = 0;
    volatile unsigned int uint_sig_period = 0;
    volatile unsigned int uint_sig_frequency = 0;

	// Check if EXTI2 interrupt pending flag is set (for the function generator)
	if ((EXTI->PR & EXTI_PR_PR2) != 0) {
		
		// Clear EXTI2 interrupt pending flag
		EXTI->PR |= ((uint32_t)0x00000004); // A pending register (PR) bit is cleared by writing 1 to it

		// Handle first edge
		if(first_edge == 1) {
			// Clear count register
			TIM2->CNT = ((uint32_t)0x00000000);
			// Start timer
			TIM2->CR1 |= ((uint16_t)0x0001);
			// Update flag
			first_edge = 0;
		}
			
		// Handle second edge
		else {
			// Stop timer
			TIM2->CR1 &= ((uint16_t)0xFFFE);
			// Read out count register
			count = TIM2->CNT;
			// Calculate signal period by changing from CPU frequency (48MHz) to microseconds
			sig_period = count/48.0;
			// Calculate signal frequency (+0.5 for rounding)
			sig_frequency = (1000000.0/sig_period) + 0.5;
			// Convert to unsigned int for printing
			uint_sig_period = (unsigned int) sig_period;
			uint_sig_frequency = (unsigned int) sig_frequency;
			// Print calculated values to the console
			trace_printf("Signal (Function Generator) Period:    %u us\n", uint_sig_period);
			trace_printf("Signal (Function Generator) Frequency: %u Hz\n", uint_sig_frequency);
			// Reset flag
			first_edge = 1;
		}
		
	}
	
}





// NOT USED YET

void refresh_OLED(void) {

    // Buffer size = at most 16 characters per PAGE + terminating '\0'
    unsigned char buffer[17];

    snprintf(buffer, sizeof(buffer), "R: %5u Ohms", res);
    /* Buffer now contains your character ASCII codes for LED Display
       - select PAGE (LED Display line) and set starting SEG (column)
       - for each c = ASCII code = Buffer[0], Buffer[1], ...,
           send 8 bytes in Characters[c][0-7] to LED Display
    */

    //...


    snprintf(buffer, sizeof(buffer), "F: %5u Hz", freq);
    /* Buffer now contains your character ASCII codes for LED Display
       - select PAGE (LED Display line) and set starting SEG (column)
       - for each c = ASCII code = Buffer[0], Buffer[1], ...,
           send 8 bytes in Characters[c][0-7] to LED Display
    */

    //...


	/* Wait for ~100 ms (for example) to get ~10 frames/sec refresh rate
       - You should use TIM3 to implement this delay (e.g., via polling)
    */

    //...

}

void oled_Write_Cmd(unsigned char cmd) {

    //... // make PB6 = CS# = 1
    //... // make PB7 = D/C# = 0
    //... // make PB6 = CS# = 0
    oled_Write(cmd);
    //... // make PB6 = CS# = 1

}

void oled_Write_Data(unsigned char data) {

    //... // make PB6 = CS# = 1
    //... // make PB7 = D/C# = 1
    //... // make PB6 = CS# = 0
    oled_Write(data);
    //... // make PB6 = CS# = 1

}

void oled_Write(unsigned char Value) {

    /* Wait until SPI1 is ready for writing (TXE = 1 in SPI1_SR) */

    //...

    /* Send one 8-bit character:
       - This function also sets BIDIOE = 1 in SPI1_CR1
    */
    HAL_SPI_Transmit(&SPI_Handle, &Value, 1, HAL_MAX_DELAY);


    /* Wait until transmission is complete (TXE = 1 in SPI1_SR) */

    //...

}

void oled_config(void) {

	// Don't forget to enable GPIOB clock in RCC
	// Don't forget to configure PB3/PB5 as AF0
	// Don't forget to enable SPI1 clock in RCC

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
    HAL_SPI_Init( &SPI_Handle );

    // Enable the SPI
    __HAL_SPI_ENABLE( &SPI_Handle );


    /* Reset LED Display (RES# = PB4):
       - make pin PB4 = 0, wait for a few ms
       - make pin PB4 = 1, wait for a few ms
    */
    //...


	// Send initialization commands to LED display
    for ( unsigned int i = 0; i < sizeof( oled_init_cmds ); i++ )
    {
        oled_Write_Cmd( oled_init_cmds[i] );
    }


    /* Fill LED Display data memory (GDDRAM) with zeros:
       - for each PAGE = 0, 1, ..., 7
           set starting SEG = 0
           call oled_Write_Data( 0x00 ) 128 times
    */

    //...

}

#pragma GCC diagnostic pop
