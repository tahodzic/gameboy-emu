#pragma once

#include "cpu.h"
#include "ppu.h"
#include <iostream>

extern int countCycles;

int main()
{

	initialize();
	setupScreen();
	loadRom("D:/Other/Gameboy/Game/Tetris (World).gb");
	while (1) {
		int i = -1;
		while (countCycles < MAX_CYCLES_PER_SECOND)
		{
			int cycles = executeOpcode(fetchOpcode());
			i++;
			countCycles += cycles;
			//updateFlagRegister();
			updateGraphics(cycles);
			checkInterruptRequests();
		}
		countCycles = 0;
		drawToScreen();


	}



	return 0;
}