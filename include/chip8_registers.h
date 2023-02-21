#ifndef CHIP8_REGISTERS_H
#define CHIP8_REGISTERS_H

#include "config.h"

struct chip8_registers
{
	unsigned char V[CHIP8_TOTAL_DATA_REGISTERS];
	unsigned short I;
	unsigned char  dt;  // delay timer
	unsigned char  st;  // sound timer
	unsigned short PC;  // program counter
	unsigned char  SP;  // stack pointer
};

#endif
