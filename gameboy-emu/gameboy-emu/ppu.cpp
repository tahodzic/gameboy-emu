#pragma once

#include "ppu.h"
#include "cpu.h"
#include "SDL.h"

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
	//SDL_Point points[23040] = { 0 };
	//bool empty = true;

	////rect array to be displayed on screen
	//SDL_Rect rect[23040];
	//for (int x = 0; x < 160; x++)
	//{
	//	for (int y = 0; y < 144; y++)
	//	{
	//		if (screenData[x][y][0])
	//		{
	//			points[y * 160 + x].x = x;
	//			points[y * 160 + x].y = y;
	//		}
	//	}
	//}
	//SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF); // the rect color (black, background)
	//SDL_RenderClear(renderer);
	//SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // the rect color (solid white)
	//SDL_RenderDrawPoints(renderer, points, 23040);
	////SDL_RenderFillRects(renderer, rect, 2048);
	//SDL_RenderPresent(renderer);

	
	//SDL_RenderCopy(renderer, texture, NULL, &texture_rect);
	/*SDL_RenderCopy is responsible for making the gameloop understand that there's
	something that wants to be rendered, inside the parentheses are (the renderer's name,
	the name of the texture, the area it wants to crop but you can leave this as NULL
	if you don't want the texture to be cropped, and the texture's rect)*/






	SDL_Texture* texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		160,
		144);

	Uint32 pixels[23040] = { 0 };
	pixels[0] = 0;
	int pitch = 4 * 160;
	Uint32 format = 0;

	int test = SDL_QueryTexture(texture, &format, NULL, NULL, NULL);

	//int test2 = SDL_LockTexture(texture, NULL,(void**) &pixels, &pitch);
	//Uint32 red = 0, green = 0, blue = 0;

	////for (int x = 0; x < 160; x++)
	////{
	////	for (int y = 0; y < 144; y++)
	////	{
	////		if (screenData[x][y][0])
	////		{
	////			red = screenData[x][y][0];
	////			green = screenData[x][y][1];
	////			blue = screenData[x][y][2];
	////			int a = y * 160 + x;
	////			pixels[y * 160 + x] = (red << 3*8) | (green << 2*8) | (blue << 8);
	////		}
	////	}
	////}
	//pixels[0] = 0xCCAABBDD;
	//pixels[1] = 0xCCAABBDD;
	//pixels[2] = 0xCCAABBDD;
	//pixels[3] = 0xCCAABBDD;
	//pixels[4] = 0xCCAABBDD;

	//SDL_UnlockTexture(texture);

	Uint32 red = 0, green = 0, blue = 0;

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
				pixels[y * 160 + x] = (red << 2*8) | (green << 8) | blue;
			}
		}
	}

	SDL_UpdateTexture(texture, NULL, pixels, pitch);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);





}
	//for (int i = 0; i < 2048; ++i) {
	//	if (screenData == 1) {

	//		rect[i].x = ((i * 10) % 640);
	//		rect[i].y = 10 * i / 64;
	//		rect[i].h = 10;
	//		rect[i].w = 10;
	//	}
	//	if (chip8::gfx[i] == 0) {
	//		rect[i].x = 0;
	//		rect[i].y = 0;
	//		rect[i].h = 0;
	//		rect[i].w = 0;
	//	}
	//}


