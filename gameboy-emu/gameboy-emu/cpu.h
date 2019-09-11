#pragma once

#ifndef CPU_H
#define CPU_H

#define VERTICAL_BLANKING 1
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

#define Z_FLAG 7
#define N_FLAG 6
#define H_FLAG 5
#define C_FLAG 4


//#define REG_A 7
//#define REG_B 0
//#define REG_C 1
//#define REG_D 2
//#define REG_E 3
//#define REG_H 4
//#define REG_L 5
//#define FLAGS 6

/*
BC 00
DE 01
HL 10
AF 11
*/



void loadRom(const char *);
void initialize();
int fetchOpcode();
int executeOpcode(unsigned char);
unsigned char readRam(unsigned short address);
void writeRam(unsigned short  address, unsigned char data);
//void writeRam(unsigned short  address, unsigned short  data);
void pushToStack(unsigned short data);
unsigned short popFromStack();
void requestInterrupt(int interruptNumber);
void checkInterruptRequests();
void serviceInterrupt(int interruptNumber);
void startDmaTransfer(unsigned char data);
void setBit(unsigned char * value, int bitNumber);
void resetBit(unsigned char * value, int bitNumber);
bool isBitSet(unsigned char * value, int bitNumber);


int getRegPairNumber(unsigned short opcode);
unsigned short readRegPairValue(int pairNr);
void writeRegPairValue(int pairNr, unsigned short pairValue);


#endif




