

#pragma once


#ifndef CPU_H
#define CPU_H


unsigned char ram[65536];
unsigned short stackPointer = 0, programCounter = 0;
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

void loadRom(const char *);
void initialize();
int fetchOpcode();
void executeOpcode(unsigned char);
void updateFlagRegister();
unsigned char readRam(unsigned short address);
void writeRam(unsigned short  address, char data);
void writeRam(unsigned short  address, unsigned short  data);

#endif
