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
	if (!loadCartridge("D:/Other/Gameboy/Game/Legend of Zelda, The - Link's Awakening (Germany).gb"))
	//if (!loadCartridge("D:/Other/Gameboy/blarggs/gb-test-roms-master/cpu_instrs/cpu_instrs.gb"))
	//if (!loadCartridge("D:/Other/Gameboy/blarggs/gb-test-roms-master/oam_bug/oam_bug.gb"))
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
			updateTimers(cycles);
			updateGraphics(cycles);
			checkInterruptRequests();
		}
		countCycles = 0;
		drawToScreen();


	}



	return 0;
}