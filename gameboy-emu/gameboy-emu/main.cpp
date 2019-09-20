#pragma once

#include "cpu.h"
#include "ppu.h"
#include "input.h"
#include <iostream>

extern int countCycles;


int main()
{

	initialize();
	setupScreen();
	if (!loadRom("D:/Other/Gameboy/Game/Tetris (World).gb"))
		return -1;

	bool quit = false;


	while (1) {
		while (countCycles < MAX_CYCLES_PER_SECOND)
		{
			checkInput();
			int cycles = executeOpcode(fetchOpcode());
			countCycles += cycles;
			updateGraphics(cycles);
			checkInterruptRequests();
		}
		countCycles = 0;
		drawToScreen();


	}



	return 0;
}