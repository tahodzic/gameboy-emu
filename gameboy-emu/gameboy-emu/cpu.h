

#pragma once


#ifndef CPU_H
#define CPU_H

void loadRom(const char *);
void initialize();
short int fetchOpcode();
void executeOpcode(short int);

char ram[65536];
short int stackPointer = 0, programCounter = 0;

struct reg
{
	char A;
	char B;
	char C;
	char D;
	char E;
	char F;
	char H;
	char L;
} registers;

#endif