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




void loadRom(const char *);
void initialize();
int fetchOpcode();
int executeOpcode(unsigned char);
void updateFlagRegister();
unsigned char readRam(unsigned short address);
void writeRam(unsigned short  address, unsigned char data);
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
void startDmaTransfer(unsigned char data);
void renderSprites();
void renderTiles();

#endif




