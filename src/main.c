#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#include "chip8.h"

const char keyboard_map[CHIP8_TOTAL_KEYS] = {
    SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,
    SDLK_8, SDLK_9, SDLK_a, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f,
};

int main(int argc, char **argv) {
  struct chip8 chip8;
  chip8_init(&chip8);

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
        char vkey = chip8_keyboard_map(keyboard_map, key);
        if (vkey != -1) {
          printf("chip8> key 0x%X is down\n", vkey);
          chip8_keyboard_down(&chip8.keyboard, vkey);
        }
      } break;
      case SDL_KEYUP: {
        char key = event.key.keysym.sym;
        int vkey = chip8_keyboard_map(keyboard_map, key);
        if (vkey != -1) {
          printf("chip8> key 0x%X is up\n", vkey);
          chip8_keyboard_up(&chip8.keyboard, vkey);
        }
      } break;
      }
    }
    if (run) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      SDL_RenderClear(renderer);

      SDL_SetRenderDrawColor(renderer, 0, 177, 64, 0);
      SDL_Rect r;
      r.x = 0;
      r.y = 0;
      r.w = 40;
      r.h = 40;
      SDL_RenderFillRect(renderer, &r);
      SDL_RenderPresent(renderer);
    }
  }
  printf("chip8> emulation ended\n");
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
