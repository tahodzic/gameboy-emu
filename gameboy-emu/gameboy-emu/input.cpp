#include "input.h"
#include "cpu.h"
#include "SDL.h"
#include <iostream>

unsigned char joypadState = 0xFF;
extern unsigned char ram[65536];

void handleInput(SDL_Event event)
{
	if (event.type == SDL_KEYDOWN)
	{
		int key = -1;
		switch (event.key.keysym.sym)
		{
			case SDLK_a: key = 4; break;
			case SDLK_s: key = 5; break;
			case SDLK_RETURN: key = 7; 
	std::cout << "keypressed" << std::endl;
				
				break;
			case SDLK_SPACE: key = 6; break;
			case SDLK_RIGHT: key = 0; break;
			case SDLK_LEFT: key = 1; break;
			case SDLK_UP: key = 2; break;
			case SDLK_DOWN: key = 3; break;
		}
		if (key != -1)
		{
			onKeyPressed(key);
		}
	}
	//If a key was released
	else if (event.type == SDL_KEYUP)
	{
		int key = -1;
		switch (event.key.keysym.sym)
		{
			case SDLK_a: key = 4; break;
			case SDLK_s: key = 5; break;
			case SDLK_RETURN: key = 7; break;
			case SDLK_SPACE: key = 6; break;
			case SDLK_RIGHT: key = 0; break;
			case SDLK_LEFT: key = 1; break;
			case SDLK_UP: key = 2; break;
			case SDLK_DOWN: key = 3; break;
		}
		if (key != -1)
		{
			onKeyReleased(key);
		}
	}
}

void checkInput()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		handleInput(event);
	}
}

void onKeyReleased(int key)
{
	//0 = pressed, 1 = released, hence setting the bit means it's released
	setBit(&joypadState, key);
}

void onKeyPressed(int key)
{
	resolveStop();
	// this function CANNOT call ReadMemory(0xFF00) it must access it directly from m_Rom[0xFF00]
// because ReadMemory traps this address
	bool previouslyUnset = false;

	if (isBitSet(&joypadState, key) == false)
		previouslyUnset = true;

	resetBit(&joypadState, key);

	// button pressed
	bool button = true;

	if (key > 3)
		button = true;
	else // directional button pressed
		button = false;

	unsigned char keyReq = ram[0xFF00];
	bool reqInterrupt = false;

	// player pressed button and programmer interested in button
	if (button && !isBitSet(&keyReq, 5))
	{
		reqInterrupt = true;
	}
	// player pressed directional and programmer interested
	else if (!button && !isBitSet(&keyReq, 4))
	{
		reqInterrupt = true;
	}

	if (reqInterrupt && !previouslyUnset)
	{
		requestInterrupt(JOYPAD);
	}
}

unsigned char getJoypadState()
{
	unsigned char res = ram[0xFF00];
	// flip all the bits
	res ^= 0xFF;

	// are we interested in the standard buttons?
	if (!isBitSet(&res, 4))
	{
		unsigned char topJoypad = joypadState >> 4;
		topJoypad |= 0xF0; // turn the top 4 bits on
		res &= topJoypad; // show what buttons are pressed
	}
	else if (!isBitSet(&res, 5))//directional buttons
	{
		unsigned char bottomJoypad = joypadState & 0xF;
		bottomJoypad |= 0xF0;
		res &= bottomJoypad;
	}
	return res;
}