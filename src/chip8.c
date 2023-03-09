#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "chip8.h"

const char chip8_default_character_set[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

void chip8_init(struct chip8 *chip8) {
  memset(chip8, 0, sizeof(struct chip8));
  memcpy(&chip8->memory.memory, chip8_default_character_set,
         sizeof(chip8_default_character_set));
};

void chip8_load(struct chip8* chip8, const char* buf, size_t size) {
	assert(size + CHIP8_PROGRAM_LOAD_ADDR <= CHIP8_MEMORY_SIZE);
	memcpy(&chip8->memory.memory[CHIP8_PROGRAM_LOAD_ADDR], buf, size);
	chip8->registers.PC = CHIP8_PROGRAM_LOAD_ADDR;
};

static void chip8_exec_extended_eight(struct chip8* chip8, unsigned short opcode) {
	unsigned char x = (opcode >> 8) & 0x000F;
	unsigned char y = (opcode >> 4) & 0x000F;
	unsigned char final_four_bits = opcode & 0x000F;
	unsigned short tmp = 0;

	switch(final_four_bits) {
		// LD Vx, Vy (8xy0): Set Vx = Vy.
		case 0x00:
			chip8->registers.V[x] = chip8->registers.V[y];
			break;
		// OR Vx, Vy (8xy1): Set Vx = Vx | Vy.
		case 0x01:
			chip8->registers.V[x] |= chip8->registers.V[y];
			break;
		// AND Vx, Vy (8xy2): Set Vx = Vx & Vy.
		case 0x02:
			chip8->registers.V[x] &= chip8->registers.V[y];
			break;
		// XOR Vx, Vy (8xy3): Set Vx = Vx ^ Vy.
		case 0x03:
			chip8->registers.V[x] ^= chip8->registers.V[y];
			break;
		// ADD Vx, Vy (8xy4): Set Vx = Vx + Vy, set VF = carry
		case 0x04:
			tmp = chip8->registers.V[x] + chip8->registers.V[y];
			chip8->registers.V[0x0F] = tmp > 0xFF;
			chip8->registers.V[x] = tmp;
			break;
		// SUB Vx, Vy (8xy5): Set Vx = Vx - Vy, set VF = NOT borrow
		case 0x05:
			chip8->registers.V[0x0F] = chip8->registers.V[x] > chip8->registers.V[y];
			chip8->registers.V[x] -= chip8->registers.V[y];
			break;
		// SHR Vx {, Vy} (8xy6): Set Vx = Vx >> 1
		case 0x06:
			chip8->registers.V[0x0F] = chip8->registers.V[x] & 0x01;
			chip8->registers.V[x] /= 2;
			break;
		// SUBN Vx, Vy (8xy7): Set Vx = Vy - Vx, set VF = NOT borrow
		case 0x07:
			chip8->registers.V[0x0F] = chip8->registers.V[y] > chip8->registers.V[x];
			chip8->registers.V[x] = chip8->registers.V[y] - chip8->registers.V[x];
			break;
		// SHL Vx {, Vy} (8xyE): Set Vx = Vx << 1
		case 0x0E:
			chip8->registers.V[0x0F] = chip8->registers.V[x] & 0b10000000;
			chip8->registers.V[x] *= 2;
			break;
	}
};

static char chip8_wait_for_key_press(struct chip8* chip8) {
	SDL_Event event;
	while(SDL_WaitEvent(&event)) {
		if (event.type != SDL_KEYDOWN)
			continue;

		char c = event.key.keysym.sym;
		char chip8_key = chip8_keyboard_map(&chip8->keyboard, c);

		if (chip8_key != -1) {
			return chip8_key;
		}

		return -1;
	}
}

static void chip8_exec_extended_f(struct chip8* chip8, unsigned short opcode) {
	unsigned char x  = (opcode >> 8) & 0x000F;
	
	switch(opcode & 0x00FF)
	{
		// LD Vx, DT (Fx07): Set Vx to delay timer value.
		case 0x07:
			chip8->registers.V[x] = chip8->registers.dt;
			break;
		// LD Vx, K (Fx0A): Set Vx = pressed key
		case 0x0A:
			{
				char pressed_key = chip8_wait_for_key_press(chip8);
				chip8->registers.V[x] = pressed_key;
			}
			break;
		// LD DT, Vx (Fx15): Set DT = Vx.
		case 0x15:
			chip8->registers.dt = chip8->registers.V[x];
			break;
		// LD ST, Vx (Fx18): Set ST = Vx.
		case 0x18:
			chip8->registers.st = chip8->registers.V[x];
			break;
		// ADD I, Vx (Fx1E): Set I = I + Vx
		case 0x1E:
			chip8->registers.I += chip8->registers.V[x];
			break;
		// LD F, Vx (Fx29): Set I = location of sprite for digit Vx.
		case 0x29:
			chip8->registers.I = chip8->registers.V[x] * CHIP8_DEFAULT_SPRITE_HEIGHT;
			break;
		// LD B, Vx (Fx33):
		case 0x33:
			{
				unsigned char hundreds = chip8->registers.V[x] / 100;
				unsigned char tens = chip8->registers.V[x] / 10 % 10;
				unsigned char ones = chip8->registers.V[x] % 10;

				chip8_memory_set(&chip8->memory, chip8->registers.I, hundreds);
				chip8_memory_set(&chip8->memory, chip8->registers.I + 1, tens);
				chip8_memory_set(&chip8->memory, chip8->registers.I + 2, ones);
			}
			break;
		// LD [I], Vx (Fx55): Store registers V0-Vx in memory starting at location I
		case 0x55:
			for (int i = 0; i <= x; i++) {
				chip8_memory_set(&chip8->memory, chip8->registers.I+i, chip8->registers.V[i]);
			}
			break;
		// LD Vx, [I] (Fx65): Read memory into V0-Vx starting at location I
		case 0x65:
			for (int i = 0; i <= x; i++) {
				chip8->registers.V[i] = chip8_memory_get(&chip8->memory, chip8->registers.I+i);
			}
			break;
	}
}

static void chip8_exec_extended(struct chip8* chip8, unsigned short opcode) {
	unsigned short nnn = opcode & 0x0fff;
	unsigned char x = (opcode >> 8) & 0x000F;
	unsigned char y = (opcode >> 4) & 0x000F;
	unsigned char n = opcode & 0x000F;
	unsigned char kk = opcode & 0x00FF;

	switch(opcode & 0xF000)
	{
		// JP addr: 1nnn Jump to location nnn
		case 0x1000:
			chip8->registers.PC = nnn;
			break;
		// CALL addr: Call subroutine at location nnn
		case 0x2000:
			chip8_stack_push(chip8, chip8->registers.PC);
			chip8->registers.PC = nnn;
			break;
		// SE Vx, byte (3xkk): Skip next instruction if Vx == kk
		case 0x3000:
			if (chip8->registers.V[x] == kk) {
				chip8->registers.PC += 2;
			}
			break;
		// SNE Vx, byte (4xkk): Skip next instruction if Vx != kk
		case 0x4000:
			if (chip8->registers.V[x] != kk) {
				chip8->registers.PC += 2;
			}
			break;
		// SE Vx, Vy (4xy0): Skip next instruction if Vx == Vy
		case 0x5000:
			if (chip8->registers.V[x] == chip8->registers.V[y]) {
				chip8->registers.PC += 2;
			}
			break;
		// LD Vx, byte (6xkk): Load kk into Vx
		case 0x6000:
			chip8->registers.V[x] = kk;
			break;
		// ADD Vx, byte (7xkk): Add kk to Vx
		case 0x7000:
			chip8->registers.V[x] += kk;
			break;
		// execute 0x8000 series opcodes
		case 0x8000:
			chip8_exec_extended_eight(chip8, opcode);
			break;
		// SNE Vx, Vy (9xy0): Skip next instrcution if Vx != Vy
		case 0x9000:
			if (chip8->registers.V[x] != chip8->registers.V[y]) {
				chip8->registers.PC += 2;
			}
			break;
		// LD I, addr (Annn): Set I = nnn.
		case 0xA000:
			chip8->registers.I = nnn;
			break;
		// JP V0, addr (Bnnn): Jump to location nnn + V0.
		case 0xB000:
			chip8->registers.PC = chip8->registers.V[0x00] + nnn;
			break;
		// RND Vx, byte (Cxkk): Set Vx = random byte & kk
		case 0xC000:
			srand(clock());
			chip8->registers.V[x] = (rand() % 255) & kk;
			break;
		// DRW Vx, Vy, nibble (Dxyn): Display n-byte sprite starting at memory location
		// I at (Vx, Vy), set VF = collision.
		case 0xD000:
			{
				const char* sprite = (const char*) &chip8->memory.memory[chip8->registers.I];
				chip8->registers.V[0x0F] = chip8_screen_draw_sprite(
					&chip8->screen,
					chip8->registers.V[x],
					chip8->registers.V[y],
					sprite,
					n
				);
			}
			break;
		case 0xE000:
			{
				switch (opcode & 0x00FF)
				{
					// SKP Vx, (Ex9E): Skip the next instruction if the key with
					// value Vx is pressed.
					case 0x9E:
						if (chip8_keyboard_is_down(&chip8->keyboard, chip8->registers.V[x])) {
							chip8->registers.PC += 2;
						}
						break;
					// SKNP Vx, (ExA1): Skip the next instruction if the key with
					// value Vx is not pressed.
					case 0xA1:
						if (!chip8_keyboard_is_down(&chip8->keyboard, chip8->registers.V[x])) {
							chip8->registers.PC += 2;
						}
						break;
				}
			}
			break;
		case 0xF000:
			chip8_exec_extended_f(chip8, opcode);
			break;
	}
};

void chip8_exec(struct chip8* chip8, unsigned short opcode) {
	switch(opcode) {
		// CLS: Clear the display
		case 0x00E0:
			chip8_screen_clear(&chip8->screen);
			break;
		// RET: Return from subroutine
		case 0x00EE:
			chip8->registers.PC = chip8_stack_pop(chip8);
			break;
		default:
			chip8_exec_extended(chip8, opcode);
	}
};

