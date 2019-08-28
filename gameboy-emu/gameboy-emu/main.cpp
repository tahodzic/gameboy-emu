#pragma once

#include "cpu.h"
#include "ppu.h"

extern int countCycles;

int main()
{
	initialize();
	loadRom("D:/Other/Gameboy/Game/Tetris (World).gb");
	while (1) {
		while (countCycles < MAX_CYCLES_PER_SECOND)
		{
			int cycles = executeOpcode(fetchOpcode());
			countCycles += cycles;
			updateFlagRegister();
			updateGraphics(cycles);
			checkInterruptRequests();
		}

		drawToScreen();

	}


	return 0;
}