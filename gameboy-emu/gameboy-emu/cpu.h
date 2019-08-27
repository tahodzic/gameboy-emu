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
#define MAX_CYCLES_PER_SECOND 69905
#define CYCLES_PER_SCAN_LINE 456
#define LCDC_CTRL 0xFF40
#define LCDC_STAT 0xFF41
#define LCDC_LY 0xFF44

unsigned char ram[65536];
unsigned short stackPointer, programCounter;
int nofCycles;
int cyclesScanLine;
/*Interrupts*/
/*
Bit 0: V-Blank Interupt
Bit 1: LCD Interupt
Bit 2: Timer Interupt
Bit 4: Joypad Interupt
*/
char imeFlag;
unsigned char screenData[160][144][3]

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

struct flag
{
	char Z;
	char N;
	char H;
	char C;
} flags;


void loadRom(const char *);
void initialize();
int fetchOpcode();
int executeOpcode(unsigned char);
void updateFlagRegister();
unsigned char readRam(unsigned short address);
void writeRam(unsigned short  address, char data);
void writeRam(unsigned short  address, unsigned short  data);
void pushToStack(unsigned short data);
unsigned short popFromStack();
void requestInterrupt(int interruptNumber);
void checkInterruptRequests();
void serviceInterrupt(int interruptNumber);
void updateGraphics(int cycles);
void drawScanLine();
void setLcdStatus();
bool isLcdEnabled();
void startDmaTransfer(char data);
void renderSprites();
void renderTiles();

#endif




