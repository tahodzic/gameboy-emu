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
	loadRom("D:/Other/Gameboy/Game/Tetris (World).gb");
	bool quit = false;


	while (1) {
		int i = -1;
		while (countCycles < MAX_CYCLES_PER_SECOND)
		{
			checkInput();
			int cycles = executeOpcode(fetchOpcode());
			i++;
			countCycles += cycles;
			updateGraphics(cycles);
			checkInterruptRequests();
		}
		countCycles = 0;
		drawToScreen();


	}



	return 0;
}