

#pragma once


#ifndef CPU_H
#define CPU_H

void loadRom(const char *);
void initialize();
int fetchOpcode();
void executeOpcode(unsigned char);
void updateFlagRegister();
char readRam(short int address);
void writeRam(short int address, char data);
void writeRam(short int address, short int data);

unsigned char ram[65536];
short int stackPointer = 0, programCounter = 0;
int MAXCYCLESPERSECOND = 69905;

struct reg
{
	char A = 0x00;
	char B = 0x00;
	char C = 0x00;
	char D = 0x00;
	char E = 0x00;
	char F = 0x00;
	char H = 0x00;
	char L = 0x00;
} registers;

struct flag
{
	char Z = 0x00;
	char N = 0x00;
	char H = 0x00;
	char C = 0x00;
} flags;

char imeFlag;

#endif
