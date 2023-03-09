#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "chip8.h"

const char keyboard_map[CHIP8_TOTAL_KEYS] = {
    SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,
    SDLK_8, SDLK_9, SDLK_a, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f,
};

void draw_boobies(struct chip8 *chip8, int x, int y) {
  chip8_screen_draw_sprite(&chip8->screen, x, y, &chip8->memory.memory[0x28],
                           5);
  chip8_screen_draw_sprite(&chip8->screen, x + 5, y,
                           &chip8->memory.memory[0x00], 5);
  chip8_screen_draw_sprite(&chip8->screen, x + 10, y,
                           &chip8->memory.memory[0x00], 5);
  chip8_screen_draw_sprite(&chip8->screen, x + 15, y,
                           &chip8->memory.memory[0x28], 5);
  chip8_screen_draw_sprite(&chip8->screen, x + 20, y,
                           &chip8->memory.memory[0x05], 5);
  chip8_screen_draw_sprite(&chip8->screen, x + 25, y,
                           &chip8->memory.memory[0x0F], 5);
  chip8_screen_draw_sprite(&chip8->screen, x + 30, y,
                           &chip8->memory.memory[0x19], 5);
};

void reverse(char *str, int len) {
  int i = 0, j = len - 1, temp;
  while (i < j) {
    temp = str[i];
    str[i] = str[j];
    str[j] = temp;
    i++;
    j--;
  }
};

int intToStr(int x, char str[], int d) {
  int i = 0;
  while (x) {
    str[i++] = (x % 10) + '0';
    x /= 10;
  }

  while (i < d) {
    str[i++] = '0';
  }

  reverse(str, i);
  str[i] = '\0';
  return i;
};

void ftoa(float n, char *res, int afterpoint) {
  int ipart = (int)n;
  float fpart = n - (float)ipart;
  int i = intToStr(ipart, res, 0);
  if (afterpoint != 0) {
    res[i] = '.';
    fpart = fpart * pow(10.0, (double)afterpoint);
    intToStr((int)fpart, res + i + 1, afterpoint);
  }
};

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("You must provide a file to load\n");
    return -1;
  }

  const char* filename = argv[1];
  printf("The filename to load is %s\n", filename);

  FILE* f = fopen(filename, "rb");
  if (!f)
  {
	  printf("Failed to open the file\n");
	  fclose(f);
	  return -1;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char buf[size];
  int res = fread(buf, size, 1, f);
  if (res != 1)
  {
	  printf("Failed to read from file.\n");
	  fclose(f);
	  return -1;
  }

  struct chip8 chip8;
  chip8_init(&chip8);
  chip8_load(&chip8, buf, size);
  chip8_keyboard_set_map(&chip8.keyboard, keyboard_map);

  // set up data for system checks
  chip8_memory_set(&chip8.memory, 0x400, 'Z');
  for (int i = 0; i <= 0xF; i++) {
    chip8.registers.V[i] = 'A' + i;
  }

  // run system checks
  printf("chip8> checking systems...\n");
  printf("  # chip8_mem check: %c\n", chip8_memory_get(&chip8.memory, 0x400));
  printf("  # chip8_register check...\n");
  for (int i = 0; i <= 0xF; i++) {
    printf("    - Register V%X: %c\n", i, chip8.registers.V[i]);
  }
  printf("  # chip8_register check success!\n");
  printf("  # chip8_stack check...\n");
  // push to return addresses
  printf("    - Push 1: 0xFF\n");
  chip8_stack_push(&chip8, 0xFF);
  printf("    - Push 2: 0xAA\n");
  chip8_stack_push(&chip8, 0xAA);
  // pop addresses from stack
  printf("    - Pop 1: 0x%X\n", chip8_stack_pop(&chip8));
  printf("    - Pop 2: 0x%X\n", chip8_stack_pop(&chip8));
  printf("  # chip8_stack check success!\n");
  // check keyboard
  printf("  # chip8_keyboard check...\n");
  printf("    - Pushing 0x2, 0x6, 0xD down...\n");
  chip8_keyboard_down(&chip8.keyboard, 0x2);
  chip8_keyboard_down(&chip8.keyboard, 0x6);
  chip8_keyboard_down(&chip8.keyboard, 0xD);
  int count = 0;
  for (int i = 0; i < CHIP8_TOTAL_KEYS; i++) {
    if (chip8_keyboard_is_down(&chip8.keyboard, i)) {
      printf("    - Key 0x%X is down\n", i);
      count++;
    }
  }
  if (count != 3) {
    printf("  # chip8_keyboard check unsuccessful.\n");
    return 1;
  }
  printf("    - Releasing 0xD\n");
  chip8_keyboard_up(&chip8.keyboard, 0xD);
  count = 0;
  for (int i = 0; i < CHIP8_TOTAL_KEYS; i++) {
    if (chip8_keyboard_is_down(&chip8.keyboard, i)) {
      printf("    - Key 0x%X is down\n", i);
      count++;
    }
  }
  if (count != 2) {
    printf("  # chip8_keyboard check unsuccessful.\n");
  }
  printf("  # chip8_keyboard check successful!\n");

  printf("chip8> system check complete.\n");
  printf("chip8> emulator starting...\n");

  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Window *window = SDL_CreateWindow(
      EMULATOR_WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      CHIP8_WIDTH * CHIP8_WINDOW_MULTIPLIER,
      CHIP8_HEIGHT * CHIP8_WINDOW_MULTIPLIER, SDL_WINDOW_SHOWN);

  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_TEXTUREACCESS_TARGET);
  SDL_bool run = SDL_TRUE;

  while (run) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        run = SDL_FALSE;
        break;
      case SDL_KEYDOWN: {
        char key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE) {
          printf("chip8> quitting...\n");
          run = SDL_FALSE;
          break;
        }
        char vkey = chip8_keyboard_map(&chip8.keyboard, key);
        if (vkey != -1) {
          chip8_keyboard_down(&chip8.keyboard, vkey);
        }
      } break;
      case SDL_KEYUP: {
        char key = event.key.keysym.sym;
        int vkey = chip8_keyboard_map(&chip8.keyboard, key);
        if (vkey != -1) {
          chip8_keyboard_up(&chip8.keyboard, vkey);
        }
      } break;
      }
    }
    if (run) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      SDL_RenderClear(renderer);
      SDL_SetRenderDrawColor(renderer, 0, 177, 64, 0);

      for (int x = 0; x < CHIP8_WIDTH; x++) {
        for (int y = 0; y < CHIP8_HEIGHT; y++) {
          if (chip8_screen_is_set(&chip8.screen, x, y)) {
            SDL_Rect r;
            r.x = x * CHIP8_WINDOW_MULTIPLIER;
            r.y = y * CHIP8_WINDOW_MULTIPLIER;
            r.w = CHIP8_WINDOW_MULTIPLIER;
            r.h = CHIP8_WINDOW_MULTIPLIER;
            SDL_RenderFillRect(renderer, &r);
          }
        }
      }

      SDL_RenderPresent(renderer);

      if (chip8.registers.dt > 0) {
        usleep(5000);
        chip8.registers.dt -= 1;
      }

      if (chip8.registers.st > 0) {
        char play_time[20];
        float time = 0.016667 * (float)chip8.registers.st;
        ftoa(time, play_time, 5);
        char cmd[256];
        sprintf(cmd, "play -q -n synth %s sin %d > /dev/null 2>&1", play_time,
                440);
        system(cmd);
        chip8.registers.st = 0;
      }

	  unsigned short opcode = chip8_memory_get_short(&chip8.memory, chip8.registers.PC);
	  chip8.registers.PC += 2;
	  chip8_exec(&chip8, opcode);
    }
  }

  printf("chip8> emulation ended\n");
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
