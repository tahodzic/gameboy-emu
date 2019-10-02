#pragma once

#include "cpu.h"
#include "ppu.h"
#include "input.h"
#include <iostream>
#include <Windows.h>
extern int countCycles;


int main()
{

	//if (!loadCartridge("D:/Other/Gameboy/Game/Tetris (World).gb"))
	if (!loadCartridge("D:/Other/Gameboy/Game/Mario_Yoshi.gb"))
		//if (!loadCartridge("D:/Other/Gameboy/cpu_instrs.gb"))
		return -1;
	initialize();
	setupScreen();

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
			//Sleep(100);
		countCycles = 0;
		drawToScreen();


	}



	return 0;
}