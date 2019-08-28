#pragma once

#include "ppu.h"
#include "cpu.h"
#include "SDL.h"

extern unsigned char ram[65536];

SDL_Window *mainWindow = NULL;
SDL_Renderer *renderer = NULL;

void setupScreen()
{
	SDL_Init(SDL_INIT_VIDEO);

	//Window
	mainWindow = SDL_CreateWindow("Donai Yanen GameBoy Emulator",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		640, 320,
		SDL_WINDOW_SHOWN
	);

	//Renderer
	renderer = SDL_CreateRenderer(mainWindow, -1, 0);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer); // fill the scene with black
	SDL_RenderPresent(renderer); // copy to screen

}
void drawToScreen()
{
	bool empty = true;
}


