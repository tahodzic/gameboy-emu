#pragma once

#include "ppu.h"
#include "cpu.h"
#include "SDL.h"
#include <iostream>

unsigned char screenData[GB_SCREEN_X][GB_SCREEN_Y][3];
extern int cyclesScanLine;

SDL_Window *mainWindow = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture* texture = NULL;

void setupScreen()
{
	SDL_Init(SDL_INIT_VIDEO);

	//Window
	mainWindow = SDL_CreateWindow("Donai Yanen GameBoy Emulator",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		GB_SCREEN_X, GB_SCREEN_Y,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	//Renderer
	renderer = SDL_CreateRenderer(mainWindow, -1, 0);
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderClear(renderer); // fill the scene with white
	SDL_RenderPresent(renderer); // copy to screen

	/*Create the texture in which we blit screenData.
	SDL_PIXELFORMAT_ARGB8888 = Alpha Red Green Blue, the 8s indicate depth (in bit)*/
	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		GB_SCREEN_X, 
		GB_SCREEN_Y);

	/*Format will be filled with SDL_QueryTexture (or just hard copy [though not recommended] the format from SDL_CreateTexture above*/
	Uint32 format = 0;

	if (SDL_QueryTexture(texture, &format, NULL, NULL, NULL))
	{
		std::cout << "SDL_QueryTexture failed. Error: " << SDL_GetError();
		return;
	}
}

void drawToScreen()
{

	/*This pointer needs to be only declared, not defined.
	Later with SDL_LockTexture, this pointer will be assigned the starting address of the first pixel of the texture.*/
	Uint32* pixels = nullptr;
	/*Pitch = row * size of pixel in bytes  (row = number of pixels per row)    */
	int pitch = 4 * GB_SCREEN_X;

	if (SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch))
		std::cout << "SDL_LockTexture failed. Error: " << SDL_GetError();

	Uint32 red = 0, green = 0, blue = 0, alpha = 0xFF;

	for (int x = 0; x < GB_SCREEN_X; x++)
	{
		for (int y = 0; y < GB_SCREEN_Y; y++)
		{

			red = screenData[x][y][0];
			green = screenData[x][y][1];
			blue = screenData[x][y][2];
			int a = y * GB_SCREEN_X + x;
			pixels[y * GB_SCREEN_X + x] = (alpha << 3 * 8) | (red << 2 * 8) | (green << 8) | blue;

		}
	}

	SDL_UnlockTexture(texture);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}


void updateGraphics(int cycles)
{
	setLcdStatus();

	if (isLcdEnabled())
	{
		cyclesScanLine -= cycles;

		/*when cyclesScanLine is 0, then one entire scanLine has been drawn.*/
		if (cyclesScanLine <= 0)
		{
			unsigned char currentLine = readRam(LCDC_LY);
			currentLine++;
			writeRam(LCDC_LY, currentLine);

			cyclesScanLine = CYCLES_PER_SCAN_LINE;

			if (currentLine < GB_SCREEN_Y)
				drawScanLine();

			else if (currentLine == GB_SCREEN_Y)
				requestInterrupt(VERTICAL_BLANKING);

			else if (currentLine > 153)
				writeRam(LCDC_LY, (unsigned short)0);
		}
	}


}

void drawScanLine()
{
	unsigned char control = readRam(LCDC_CTRL);
	if (isBitSet(&control, 0))
		renderTiles();
	if (isBitSet(&control, 1))
		renderSprites();
}

void renderSprites()
{
	unsigned char lcdcControl = readRam(LCDC_CTRL);
	bool use8x16 = false;

	if (isBitSet(&lcdcControl, 2))
		use8x16 = true;

	for (int sprite = 0; sprite < 40; sprite++)
	{
		unsigned char index = sprite * 4;
		unsigned char yPos = readRam(0xFE00 + index) - 16;
		unsigned char xPos = readRam(0xFE00 + index + 1) - 8;
		unsigned char spriteLocation = readRam(0xFE00 + index + 2);
		unsigned char attributes = readRam(0xFE00 + index + 3);

		bool yFlip = isBitSet(&attributes, 6);
		bool xFlip = isBitSet(&attributes, 5);

		int ly = readRam(LCDC_LY);

		int ySize = 8;
		if (use8x16)
			ySize = 16;

		if ((ly >= yPos) && (ly < (yPos + ySize)))
		{
			int line = ly - yPos;

			//read the line in backwards
			if (yFlip)
			{
				line -= ySize;
				line *= -1;
			}

			line *= 2;
			unsigned short dataAddress = (0x8000 + (spriteLocation * 16)) + line;
			unsigned char data1 = readRam(dataAddress);
			unsigned char data2 = readRam(dataAddress + 1);

			// its easier to read in from right to left as pixel 0 is
			// bit 7 in the colour data, pixel 1 is bit 6 etc...
			for (int tilePixel = 7; tilePixel >= 0; tilePixel--)
			{
				int colourBit = tilePixel;
				// read the sprite in backwards for the x axis
				if (xFlip)
				{
					colourBit -= 7;
					colourBit *= -1;
				}

				// combine data 2 and data 1 to get the colour id for this pixel
				// in the tile
				int dotData = isBitSet(&data2, colourBit) ? 1 : 0;
				dotData <<= 1;
				dotData |= (isBitSet(&data1, colourBit) ? 1 : 0);


				unsigned short paletteAddress = isBitSet(&attributes, 4) ? 0xFF49 : 0xFF48;
				int col = 0;
				unsigned char palette = readRam(paletteAddress);

				////lets get the color from the palette at 0xFF47
				col = (palette & (1 << (2 * dotData + 1))) ? 1 : 0;
				col <<= 1;
				col |= (palette & (1 << (2 * dotData))) ? 1 : 0;

				if (col == 0)
					continue;



				int red = 0;
				int green = 0;
				int blue = 0;

				// setup the RGB values
				switch (col)
				{
					case 0: red = 255;  green = 255; blue = 255;   break;
					case 1: red = 0xCC; green = 0xCC; blue = 0xCC; break;
					case 2: red = 0x77; green = 0x77; blue = 0x77; break;
				}

				int xPix = 0 - tilePixel;
				xPix += 7;
				int pixel = xPos + xPix;


				// safety check to make sure what im about
				// to set is int the 160x144 bounds
				if ((ly < 0) || (ly > 143) || (pixel < 0) || (pixel > 159))
				{
					continue;
				}

				screenData[pixel][ly][0] = red;
				screenData[pixel][ly][1] = green;
				screenData[pixel][ly][2] = blue;


			}
		}

	}
}
void renderTiles()
{
	unsigned short tileData = 0, bgMemory = 0;
	bool unsign = true;


	unsigned char scrollY = readRam(0xFF42);
	unsigned char scrollX = readRam(0xFF43);
	unsigned char windowY = readRam(0xFF4A);
	unsigned char windowX = readRam(0xFF4B) - 7;


	bool useWindow = false;
	unsigned char lcdcControl = readRam(LCDC_CTRL);



	//window enabled?
	if (isBitSet(&lcdcControl, 5))
	{
		if (windowY <= readRam(LCDC_LY))
			useWindow = true;

	}

	if (isBitSet(&lcdcControl, 4))
	{
		tileData = 0x8000;
	}
	else
	{
		tileData = 0x8800;
		unsign = false;
	}

	if (!useWindow)
	{
		if (isBitSet(&lcdcControl, 3))
			bgMemory = 0x9C00;
		else
			bgMemory = 0x9800;

	}
	else
	{
		if (isBitSet(&lcdcControl, 6))
			bgMemory = 0x9C00;
		else
			bgMemory = 0x9800;
	}

	unsigned char yPos = 0;

	if (useWindow)
		yPos = readRam(LCDC_LY) - windowY;
	else
		yPos = scrollY + readRam(LCDC_LY);

	// which of the 8 vertical pixels of the current
	// tile is the scanline on?
	unsigned short tileRow = (((unsigned char)(yPos / 8)) * 32);



	for (int pixel = 0; pixel < GB_SCREEN_X; pixel++)
	{
		unsigned char xPos = pixel + scrollX;

		if (useWindow)
		{
			if (pixel >= windowX)
				xPos = pixel - windowX;
		}
		unsigned short tileCol = (xPos / 8);
		short tileNum;

		unsigned short tileAddress = bgMemory + tileRow + tileCol;

		if (unsign)
			tileNum = (unsigned char)readRam(tileAddress);
		else
			tileNum = (signed char)readRam(tileAddress);

		unsigned short tileLocation = tileData;

		if (unsign)
			tileLocation += (tileNum * 16);
		else
			tileLocation += ((tileNum + 128) * 16);


		unsigned char line = yPos % 8;
		line *= 2; // each vertical line takes up two bytes of memory
		unsigned char data1 = readRam(tileLocation + line);
		unsigned char data2 = readRam(tileLocation + line + 1);


		// pixel 0 in the tile is bit 7 of data 1 and data 2
		// Pixel 1 in the tile is bit 6 of data 1 and data 2 etc.
		// hence the order needs to be turnt around
		// xPos goes from left to right (from low number to high number)
		// bits, however, go from right to left (from low number to high number)
		// so if you want the bit at xPos 0 you need bit number 7
		int colourBit = xPos % 8;
		colourBit = 7 - colourBit;

		// combine data 2 and data 1 to get the colour id for this pixel
		// in the tile
		int pixelColorId = isBitSet(&data2, colourBit) ? 1 : 0;
		pixelColorId <<= 1;
		pixelColorId |= (isBitSet(&data1, colourBit) ? 1 : 0);

		////lets get the color from the palette at 0xFF47
		unsigned char palette = readRam(0xFF47);
		int col = 0;
		col = (palette & (1 << (2 * pixelColorId + 1))) ? 1 : 0;
		col <<= 1;
		col |= (palette & (1 << (2 * pixelColorId))) ? 1 : 0;

		int red = 0;
		int green = 0;
		int blue = 0;

		// setup the RGB values
		switch (col)
		{
			case 0: red = 255;  green = 255; blue = 255; break;
			case 1: red = 0xCC; green = 0xCC; blue = 0xCC; break;
			case 2: red = 0x77; green = 0x77; blue = 0x77; break;
		}

		int ly = readRam(LCDC_LY);

		// safety check to make sure what im about
		// to set is int the 160x144 bounds
		if ((ly < 0) || (ly > 143) || (pixel < 0) || (pixel > 159))
		{
			continue;
		}

		screenData[pixel][ly][0] = red;
		screenData[pixel][ly][1] = green;
		screenData[pixel][ly][2] = blue;
	}
}


void setLcdStatus()
{
	unsigned char status = readRam(LCDC_STAT);

	if (!isLcdEnabled())
	{
		cyclesScanLine = CYCLES_PER_SCAN_LINE;
		writeRam(LCDC_LY, (unsigned char)0);
		
		//set mode to 01 (vblank mode)
		status &= 0xFC;
		setBit(&status, 0);

		writeRam(LCDC_STAT, status);

		return;
	}

	unsigned char currentLine = readRam(LCDC_LY);
	unsigned char currentMode = status & 0x03;

	unsigned char modeToSet = 0;
	bool requestInt = 0;

	//Mode 1: over 144, so go into vblank mode
	if (currentLine > GB_SCREEN_Y)
	{
		modeToSet = 0x01;
		setBit(&status, 0);
		resetBit(&status, 1);
		requestInt = isBitSet(&status, 4);
	}
	else
	{
		int mode2bounds = 456 - 80;
		int mode3bounds = mode2bounds - 172;

		//Mode 2 (Searching OAM-RAM Mode)
		if (cyclesScanLine >= mode2bounds)
		{
			modeToSet = 0x02;
			setBit(&status, 1);
			resetBit(&status, 0);
			requestInt = isBitSet(&status, 5);
		}
		//Mode 3 (Transfer Data to LCD Driver mode)
		else if (cyclesScanLine >= mode3bounds)
		{
			modeToSet = 0x03;
			setBit(&status, 0);
			setBit(&status, 1);
		}
		//Mode 0 (H-Blank mode)
		else
		{
			modeToSet = 0x00;
			resetBit(&status, 0);
			resetBit(&status, 1);
			requestInt = isBitSet(&status, 3);

		}

	}

	//entered a new mode so request interrupt
	if (requestInt && (modeToSet != currentMode))
		requestInterrupt(0x01);

	//check coincidence flag
	if (currentLine == readRam(0xFF45))
	{
		setBit(&status, 2);
		if (isBitSet(&status, 6))
			requestInterrupt(0x01);
	}
	else
	{
		//make sure the coincidence flag (bit 2) is 0
		resetBit(&status, 2);
	}

	writeRam(LCDC_STAT, status);
}

bool isLcdEnabled()
{
	unsigned char lcdcCtrl = readRam(LCDC_CTRL);

	return isBitSet(&lcdcCtrl, 0x7);
}

