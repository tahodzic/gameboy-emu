#pragma once

#include "ppu.h"
#include "cpu.h"
#include "SDL.h"
#include <iostream>

/*Extern indicates that screenData was defined in another file
and that we want to use it here*/
extern 	unsigned char screenData[160][144][3];

SDL_Window *mainWindow = NULL;
SDL_Renderer *renderer = NULL;

void setupScreen()
{
	SDL_Init(SDL_INIT_VIDEO);

	//Window
	mainWindow = SDL_CreateWindow("Donai Yanen GameBoy Emulator",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		160, 144,
		SDL_WINDOW_SHOWN
	);

	//Renderer
	renderer = SDL_CreateRenderer(mainWindow, -1, 0);
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderClear(renderer); // fill the scene with white
	SDL_RenderPresent(renderer); // copy to screen

}
void drawToScreen()
{
	/*Create the texture in which we blit screenData.
	SDL_PIXELFORMAT_ARGB8888 = Alpha Red Green Blue, the 8s indicate depth (in bit)*/
	SDL_Texture* texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		160,
		144);

	/*This pointer needs to be only declared, not defined.
	Later with SDL_LockTexture, this pointer will be assigned the starting address of the first pixel of the texture.*/
	Uint32* pixels;
	/*Pitch = row * size of pixel in bytes  (row = number of pixels per row)    */
	int pitch = 4 * 160;
	/*Format will be filled with SDL_QueryTexture (or just hard copy [though not recommended] the format from SDL_CreateTexture above*/
	Uint32 format = 0;

	if (SDL_QueryTexture(texture, &format, NULL, NULL, NULL))
		std::cout << "SDL_QueryTexture failed. Error: " << SDL_GetError();

	if (SDL_LockTexture(texture, NULL,(void**) &pixels, &pitch))
		std::cout << "SDL_LockTexture failed. Error: " << SDL_GetError();

	Uint32 red = 0, green = 0, blue = 0, alpha = 0;

	for (int x = 0; x < 160; x++)
	{
		for (int y = 0; y < 144; y++)
		{
			if (screenData[x][y][0])
			{
				red = screenData[x][y][0];
				green = screenData[x][y][1];
				blue = screenData[x][y][2];
				int a = y * 160 + x;
				pixels[y * 160 + x] = (alpha << 3 * 8) | (red << 2 * 8) | (green << 8) | blue;
			}
		}
	}

	SDL_UnlockTexture(texture);

	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}