

#pragma once


#ifndef CPU_H
#define CPU_H

#define VERTICAL_BLANKING 0
#define LCDC 2
#define TIMER_OVERFLOW 4
#define SERIAL_IO_COMPLETION 8
#define JOYPAD 16
#define IR_REQUEST_ADDRESS 0xFF0F
#define IR_ENABLE_ADDRESS 0xFFFF

unsigned char ram[65536];
unsigned short stackPointer = 0, programCounter = 0;
int MAXCYCLESPERSECOND = 69905;

/*Interrupts*/
/*
Bit 0: V-Blank Interupt
Bit 1: LCD Interupt
Bit 2: Timer Interupt
Bit 4: Joypad Interupt
*/
char imeFlag;


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


void loadRom(const char *);
void initialize();
int fetchOpcode();
void executeOpcode(unsigned char);
void updateFlagRegister();
unsigned char readRam(unsigned short address);
void writeRam(unsigned short  address, char data);
void writeRam(unsigned short  address, unsigned short  data);
void pushToStack(unsigned short data);
unsigned short popFromStack();
void requestInterrupt(int interruptNumber);
void checkInterruptRequests();
void serviceInterrupt(int interruptNumber);

#endif
