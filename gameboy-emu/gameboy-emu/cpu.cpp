#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "cpu.h"
#include "input.h"
#include <fstream>
#include <iostream>


int cyclesScanLine;
unsigned short stackPointer, programCounter;
/*Interrupts*/
/*
Bit 0: V-Blank Interupt
Bit 1: LCD Interupt
Bit 2: Timer Interupt
Bit 4: Joypad Interupt
*/
char imeFlag = 0x00, disableImeFlag = 0x00, enableImeFlag = 0x00, imeFlagCount = 0;
bool haltFlag = false;

const unsigned char REG_A = 7;
const unsigned char REG_B = 0;
const unsigned char REG_C = 1;
const unsigned char REG_D = 2;
const unsigned char REG_E = 3;
const unsigned char REG_H = 4;
const unsigned char REG_L = 5;
const unsigned char FLAGS = 6;

//Order is: B, C, D, E, H, L, F, A
//B: 000, C: 001, D: 010,
//E: 011, H: 100, L: 101, F: 110, A: 111
unsigned char regs[8];

unsigned char ram[65536];
int countCycles;

/*Memory Bank Controller MBC*/
bool mbc1 = false;
bool mbc2 = false;
bool enableRam = false;
bool romBanking = false;
unsigned char cartridgeContent[0x200000];
unsigned char currentReadOnlyMemoryBank;
unsigned char ramBanks[0x8000];
unsigned char currentRamBank;

/*Debug purposes*/
int debugCount = 0;
/*****************/


void initialize()
{
	freopen("output.txt", "w", stdout);

	//prevent automatic flush of cout after every "\n", speeds up extremely
	std::setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);

	countCycles = 0;
	cyclesScanLine = CYCLES_PER_SCAN_LINE;
	programCounter = 0x100;
	stackPointer = 0xFFFE; // from $FFFE - $FF80, grows downward

	//standard values for registers and some ram locations according to pandocs
	regs[REG_A] = 0x01;
	regs[REG_B] = 0x00;
	regs[REG_C] = 0x13;
	regs[REG_D] = 0x00;
	regs[REG_E] = 0xD8;
	regs[REG_H] = 0x01;
	regs[REG_L] = 0x4D;
	regs[FLAGS] = 0xB0;

	ram[0xFF00] = 0xFF;
	ram[0xFF05] = 0x00;
	ram[0xFF06] = 0x00;
	ram[0xFF07] = 0x00;
	ram[0xFF10] = 0x80;
	ram[0xFF11] = 0xBF;
	ram[0xFF12] = 0xF3;
	ram[0xFF14] = 0xBF;
	ram[0xFF16] = 0x3F;
	ram[0xFF17] = 0x00;
	ram[0xFF19] = 0xBF;
	ram[0xFF1A] = 0x7F;
	ram[0xFF1B] = 0xFF;
	ram[0xFF1C] = 0x9F;
	ram[0xFF1E] = 0xBF;
	ram[0xFF20] = 0xFF;
	ram[0xFF21] = 0x00;
	ram[0xFF22] = 0x00;
	ram[0xFF23] = 0xBF;
	ram[0xFF24] = 0x77;
	ram[0xFF25] = 0xF3;
	ram[0xFF26] = 0xF1;
	ram[0xFF40] = 0x91;
	ram[0xFF42] = 0x00;
	ram[0xFF43] = 0x00;
	ram[0xFF45] = 0x00;
	ram[0xFF47] = 0xFC;
	ram[0xFF48] = 0xFF;
	ram[0xFF49] = 0xFF;
	ram[0xFF4A] = 0x00;
	ram[0xFF4B] = 0x00;
	ram[0xFFFF] = 0x00;

	/*MBC*/
	currentReadOnlyMemoryBank = 1;
	currentRamBank = 0;
	memset(&ramBanks, 0, sizeof(ramBanks));

	unsigned char typeOfCartridge;
	typeOfCartridge = readRam(0x147);
	switch (typeOfCartridge)
	{
		case 1: case 2: case 3:
		{
			mbc1 = true;
			break;
		}
		case 5: case 6:
		{
			mbc2 = true;
			break;
		}
	}
}

unsigned char readRam(unsigned short address)
{
	//are we reading from the ready only memory ROM?
	if ((address >= 0x4000) && (address <= 0x7FFF))
	{
		unsigned short newAddress = address - 0x4000;
		return cartridgeContent[newAddress + (currentReadOnlyMemoryBank * 0x4000)];

	}
	//are we reading from random access memory RAM?
	else if ((address >= 0xA000) && (address <= 0xBFFF))
	{
		unsigned short newAddress = address - 0xA000;
		return ramBanks[newAddress + (currentRamBank * 0x2000)];
	}

	if (address == 0xFF00)
	{
		return getJoypadState();
	}

	return ram[address];
}

/*8 Bit writes*/
void writeRam(unsigned short address, unsigned char data)
{
	/*0x0000 - 0x7FFF is read only*/
	if (address > 0x7FFF)
	{
		if (address >= 0xE000 && address < 0xFE00)
		{
			ram[address] = data;
			writeRam(address - 0x2000, data);
		}

		else if (address >= 0xFEA0 && address < 0xFF00)
		{

		}


		else if (address >= 0xFF4C && address < 0xFF80)
		{

		}

		else if (address == 0xFF46)
		{
			startDmaTransfer(data);
		}
		else
		{
			ram[address] = data;
		}
	}
	else if (address < 0x8000)
	{
		handleBanking(address, data);

	}
	else if ((address >= 0xA000) && (address < 0xC000))
	{
		if (enableRam)
		{
			unsigned short newAddress = address - 0xA000;
			ramBanks[newAddress + (currentReadOnlyMemoryBank * 0x2000)] = data;
		}
	}

}

void handleBanking(unsigned short address, unsigned char data)
{
	if (address < 0x2000)
	{
		if (mbc1 || mbc2)
		{
			enableRamBank(address, data);
		}
	}
	else if (address >= 0x200 && address < 0x4000)
	{
		if (mbc1 || mbc2)
		{
			changeLowRomBank(data);
		}
	}
	else if (address >= 0x4000 && address < 0x6000)
	{
		if (mbc1)
		{
			if (romBanking)
			{
				changeHighRomBank(data);
			}
			else
			{
				changeRamBank(data);
			}
		}
	}
	else if (address >= 0x6000 && address < 0x8000)
	{
		if (mbc1)
		{
			changeModeRomRam(data);
		}
	}
}
void changeRamBank(unsigned char data)
{
	currentRamBank = data & 0x3;
}

void changeLowRomBank(unsigned char data)
{
	if (mbc2)
	{
		currentReadOnlyMemoryBank = data & 0xF;
		if (currentReadOnlyMemoryBank == 0)
			currentReadOnlyMemoryBank++;

		return;
	}

	unsigned char lower5 = data & 31;
	//turn the lower 5 bits off
	currentReadOnlyMemoryBank &= 224;
	currentReadOnlyMemoryBank |= lower5;
	if (currentReadOnlyMemoryBank == 0)
		currentReadOnlyMemoryBank++;

}

void changeModeRomRam(unsigned char data)
{
	unsigned char newData = data & 0x1;
	romBanking = (newData == 0) ? true : false;
	if (romBanking)
		currentReadOnlyMemoryBank = 0;
}

void changeHighRomBank(unsigned char data)
{
	// turn off the upper 3 bits of the current rom
	currentReadOnlyMemoryBank &= 31;

	// turn off the lower 5 bits of the data
	data &= 224;
	currentReadOnlyMemoryBank |= data;
	if (currentReadOnlyMemoryBank == 0)
		currentReadOnlyMemoryBank++;
}

void enableRamBank(unsigned short address, unsigned char data)
{
	if (mbc2)
	{
		if (address & 0x10)
			return;
	}
	unsigned char testData = data & 0xF;
	if (testData == 0xA)
		enableRam = true;
	else if (testData == 0x0)
		enableRam = false;
}

/*16 bit writes*/
//TODO
/*oid writeRam(unsigned short address, unsigned short data)
{

}*/

void startDmaTransfer(unsigned char data)
{
	unsigned short address = data << 8;
	for (int i = 0; i < 0xA0; i++)
	{
		writeRam(0xFE00 + i, readRam(address + i));
	}
}

void pushToStack(unsigned short data)
{
	stackPointer -= 2;
	unsigned char lowHalf = data & 0x00FF;
	unsigned char highHalf = (data & 0xFF00) >> 8;
	writeRam(stackPointer, highHalf);
	writeRam(stackPointer + 1, lowHalf);
}

unsigned short popFromStack()
{
	unsigned short highHalf = readRam(stackPointer);
	unsigned short lowHalf = readRam(stackPointer+1);
	unsigned short poppedValue = (highHalf << 8) | lowHalf;

	//delete the popped value from stack
	writeRam(stackPointer, 0x0);
	writeRam(stackPointer+1, 0x0);

	//increment SP so that it points to the most upper stack value 
	stackPointer += 2;

	return poppedValue;
}

void requestInterrupt(int interruptNumber)
{
	unsigned char irRequestFlagStatus = readRam(IR_REQUEST_ADDRESS);
	irRequestFlagStatus = irRequestFlagStatus | interruptNumber;
	writeRam(IR_REQUEST_ADDRESS, irRequestFlagStatus);
}

void checkInterruptRequests()
{
	char irRequestFlagStatus = readRam(IR_REQUEST_ADDRESS);
	char irEnableFlagStatus = readRam(IR_ENABLE_ADDRESS);

	if (irRequestFlagStatus > 0)
	{
		if (irRequestFlagStatus & VERTICAL_BLANKING == VERTICAL_BLANKING)
		{
			if (irEnableFlagStatus & VERTICAL_BLANKING == VERTICAL_BLANKING)
			{
				haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(VERTICAL_BLANKING);
			}
		}
		else if (irRequestFlagStatus & LCDC == LCDC)
		{
			if (irEnableFlagStatus & LCDC == LCDC)
			{
				haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(LCDC);

			}
		}
		else if (irRequestFlagStatus & TIMER_OVERFLOW == TIMER_OVERFLOW)
		{
			if (irEnableFlagStatus & TIMER_OVERFLOW == TIMER_OVERFLOW)
			{
				haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(TIMER_OVERFLOW);

			}
		}
		else if (irRequestFlagStatus & SERIAL_IO_COMPLETION == SERIAL_IO_COMPLETION)
		{
			if (irEnableFlagStatus & SERIAL_IO_COMPLETION == SERIAL_IO_COMPLETION)
			{
				haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(SERIAL_IO_COMPLETION);

			}
		}
		else if (irRequestFlagStatus & JOYPAD == JOYPAD)
		{
			if (irEnableFlagStatus & JOYPAD == JOYPAD)
			{
				haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(JOYPAD);

			}
		}



	}
}

void serviceInterrupt(int interruptNumber)
{
	imeFlag = 0;
	unsigned char irRequestFlagStatus = readRam(IR_REQUEST_ADDRESS);

	//clear the interrupt bit in the status byte
	irRequestFlagStatus &= ~interruptNumber;
	writeRam(IR_REQUEST_ADDRESS, irRequestFlagStatus);
	pushToStack(programCounter);

	switch (interruptNumber)
	{
		case VERTICAL_BLANKING: programCounter = 0x40; break;
		case LCDC: programCounter = 0x48; break;
		case TIMER_OVERFLOW: programCounter = 0x50; break;
		case SERIAL_IO_COMPLETION: programCounter = 0x58; break;
		case JOYPAD: programCounter = 0x60; break;
	}
}

int fetchOpcode()
{
	unsigned char opcode = 0;

	opcode = readRam(programCounter);
	programCounter++;
	//std::cout << "Opcode: " << std::hex << (opcode < 0x10 ? "0x0" : "0x") << (int)opcode << std::endl;
	return opcode;
}

int executeOpcode(unsigned char opcode)
{
	//debugCount++;
	//if (debugCount > 1'000'000) 
	//{
	//std::cout << "Opcode: " << std::uppercase << std::hex << (opcode < 0x10 ? "0x0" : "0x") << (int)opcode << "\n";
	//std::cout << "af: 0x" << std::uppercase << std::hex << +regs[REG_A] << +regs[FLAGS] << "\n";
	//std::cout << "bc: 0x" << std::uppercase << std::hex << +regs[REG_B] << +regs[REG_C] << "\n";
	//std::cout << "de: 0x" << std::uppercase << std::hex << +regs[REG_D] << +regs[REG_E] << "\n";
	//std::cout << "hl: 0x" << std::uppercase << std::hex << +regs[REG_H] << +regs[REG_L] << "\n";
	//std::cout << "sp: 0x" << std::uppercase << std::hex << +stackPointer << "\n";
	//std::cout << "pc: 0x" << std::uppercase << std::hex << +programCounter << "\n\n";
	//}

	if (haltFlag)
		return 4;

	if (imeFlagCount > 0)
	{
		imeFlagCount--;
		if (enableImeFlag == 1 )
		{
			imeFlag = 0x01;
			enableImeFlag = 0x00;
		}
		else if (disableImeFlag == 1)
		{
			imeFlag = 0x00;
			disableImeFlag = 0x00;
		}
	}

	switch (opcode)
	{
		case 0x00:
		{
			return 4;
		}

		/*LD r, n*/
		case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: case 0x3E:
		{
			unsigned char &dst = regs[(opcode >> 3) & 0x7];
			dst = readRam(programCounter);
			programCounter++;

			return 8;
		}


		///*LD r1, r2*/
		case 0x7F: case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: /*LD A, reg*/
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47: /*LD B, reg*/
		case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F: /*LD C, reg*/
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57: /*LD D, reg*/
		case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F: /*LD E, reg*/
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67: /*LD H, reg*/
		case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F: /*LD L, reg*/
		{
			unsigned char &dst = regs[(opcode >> 3) & 0x7];
			unsigned char src = regs[opcode & 0x7];
			dst = src;
			return 4;
		}

		/*LD r, (HL)*/
		case 0x7E: case 0x46: case 0x4E: case 0x56:  case 0x5E: case 0x66: case 0x6E:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char &dst = regs[(opcode >> 3) & 0x7];

	
			dst = readRam(src);

			return 8;
		}

		/*LD (HL), r*/
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
		{
			unsigned short dst = regs[REG_H] << 8 | regs[REG_L];
			unsigned char src = regs[opcode & 0x7];

			writeRam(dst, src);

			return 8;
		}

		/*LD (HL),n*/ case 0x36:
		{
			unsigned char n = readRam(programCounter);
			unsigned short dst = regs[REG_H] << 8 | regs[REG_L];

			writeRam(dst, n);
			programCounter++;

			return 12;
		}

		/*LD A,(BC)
		  LD A,(DE)*/
		case 0x0A: case 0x1A:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short src = readRegPairValue(pairNr);

			regs[REG_A] = readRam(src);

			return 8;
		}

		/*LD A,(nn)*/ case 0xFA:
		{
			unsigned char nHighByte = readRam(programCounter);
			unsigned char nlowByte = readRam(programCounter + 1);
			//LSB byte first
			unsigned short srcAddress =  nlowByte << 8 | nHighByte;

			regs[REG_A] = readRam(srcAddress);

			programCounter += 2;

			return 16;
		}

		/*LD (BC),A*/
		/*LD (DE),A*/
		case 0x02: case 0x12:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short dstAddress = readRegPairValue(pairNr);
			unsigned char &src = regs[REG_A];

			writeRam(dstAddress, src);

			return 8;
		}

		/*LD (nn),A*/ case 0xEA:
		{
			unsigned char nlowByte = readRam(programCounter);
			unsigned char nHighByte = readRam(programCounter+1);
			unsigned short nn = nHighByte << 8 | nlowByte;

			writeRam(nn, regs[REG_A]);

			programCounter += 2;

			return 16;
		}


		/*LD A, (C)*/ case 0xF2:
		{
			unsigned short srcAddress = 0xFF00 + regs[REG_C];
			regs[REG_A] = readRam(srcAddress);
			return 8;
		}

		/*LD (C), A*/ case 0xE2:
		{
			unsigned short dstAddress = 0xFF00 + regs[REG_C];
			writeRam(dstAddress, regs[REG_A]);
			return 8;
		}

		/*LD A,(HLD)*/
		/*LD A,(HL-)*/
		/*LDD A,(HL)*/ 
		case 0x3A:
		{
			unsigned short regHL = ((regs[REG_H] << 8) & 0xFF00) + (regs[REG_L] & 0xFF);

			regs[REG_A] = readRam(regHL);
			regHL--;
			regs[REG_H] = (regHL >> 8) & 0xFF;
			regs[REG_L] = regHL & 0xFF;
			std::cout.flush();
			return 8;
		}

		/*LD (HLD),A */
		/*LD (HL-),A */
		/*LDD (HL),A*/ 
		case 0x32:
		{
			unsigned short regHL = ((regs[REG_H] << 8) & 0xFF00) + (regs[REG_L] & 0xFF);

			writeRam(regHL, regs[REG_A]);
			regHL--;
			regs[REG_H] = (regHL >> 8) & 0xFF;
			regs[REG_L] = regHL & 0xFF;

			return 8;
		}

		/*LD A,(HLI) */
		/*LD A,(HL+) */
		/*LDI A,(HL)*/
		case 0x2A:
		{		
			
			unsigned short srcAddress = regs[REG_H] << 8 | regs[REG_L];
			unsigned char value = readRam(srcAddress);
			regs[REG_A] = value;
			srcAddress++;
			regs[REG_H] = (srcAddress & 0xFF00) >> 8;
			regs[REG_L] = srcAddress & 0x00FF;
			return 8;
		}

		/*LD (HLI),A */
		/*LD (HL+),A */
		/*LDI (HL),A*/
		case 0x22:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short dst = readRegPairValue(pairNr);

			writeRam(dst, regs[REG_A]);
			dst++;
			writeRegPairValue(pairNr, dst);

			return 8;
		}

		/*LDH (n),A*/
		/*LD ($FF00+n),A*/ case 0xE0:
		{
			//int n = ram[programCounter] & 0xFF;
			unsigned char n = readRam(programCounter);
			writeRam(0xFF00 + n, regs[REG_A]);
			programCounter++;
			return 12;
		}

		/*LDH A,(n)*/
		/*LD A,($FF00+n)*/ case 0xF0:
		{
			//int n = ram[programCounter] & 0xFF;
			unsigned char n = readRam(programCounter);
			//registers.A = ram[0xFF00 + n] & 0xFF;
			regs[REG_A] = readRam(0xFF00+n);
			///registers.A = 0x3E;
			programCounter++;
			return 12;
		}

		///*LD n,nn*/
		case 0x01: case 0x11: case 0x21:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short nn = readRam(programCounter + 1) << 8 | readRam(programCounter);

			writeRegPairValue(pairNr, nn);
			programCounter += 2;

			return 12;
		}

		/*LD SP,nn*/ case 0x31: 
		{
			unsigned char nlowByte = readRam(programCounter);
			unsigned char nHighByte = readRam(programCounter + 1);
			unsigned short nn = nHighByte << 8 | nlowByte;

			stackPointer = nn;
			programCounter += 2;

			return 12;
		}


		/*LD SP, HL*/ case 0xF9: {
			stackPointer = regs[REG_H] << 8;
			stackPointer |= regs[REG_L];

			return 8;

		}

		/*LD HL,SP+n*/ case 0xF8: {
			signed char n = readRam(programCounter);
			unsigned short res = 0;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);

			if(((int)(stackPointer&0xFF)+ (int)n) > 0xFF)
				setBit(&regs[FLAGS], C_FLAG);

			if((stackPointer & 0xF + n & 0xF) > 0xF)
				setBit(&regs[FLAGS], H_FLAG);

			res = stackPointer + n;
			regs[REG_H] = res >> 8;
			regs[REG_L] = res & 0xFF;

			return 12;

		}


		///*LD (nn),SP*/ case 0x08: {
		//	herewasabreak;

		//}



		/*PUSH AF*/ case 0xF5: {
			unsigned short pushVal = regs[REG_A] << 8 | regs[FLAGS];
			pushToStack(pushVal);

			return 16;
		}

		/*PUSH qq*/
		case 0xC5: case 0xD5: case 0xE5:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short regPairValue = readRegPairValue(pairNr);

			pushToStack(regPairValue);

			return 16;
		}
			   
		/*POP AF*/ case 0xF1: 
		{
			unsigned short popVal = popFromStack();
			regs[REG_A] = (popVal & 0xFF00) >> 8;
			regs[FLAGS] = (popVal & 0x00FF);

			return 12;

		}

		/*POP qq*/
		case 0xC1: case 0xD1: case 0xE1:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short popVal = popFromStack();

			writeRegPairValue(pairNr, popVal);

			return 12;
		}

		/*ADD A, r*/ case 0x87: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:
		{
			unsigned char &paramReg = regs[opcode & 0x7];
			unsigned char &regA = regs[REG_A]; 

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			
			if ((int)(regA + paramReg) > 255)
				setBit(&regs[FLAGS], C_FLAG);

			if (((regA & 0x0F) + (paramReg & 0x0F)) > 0x0F)
				setBit(&regs[FLAGS], H_FLAG);

			regA += paramReg;
			
			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			return 4;
		}
		

		///*ADD A,(HL)*/ case 0x86:

		/*ADD A,#*/ case 0xC6:
		{
			unsigned char n = readRam(programCounter);
			unsigned char &regA = regs[REG_A];

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);

			if ((int)(regA + n) > 255)
				setBit(&regs[FLAGS], C_FLAG);

			if (((regA & 0x0F) + (n & 0x0F)) > 0x0F)
				setBit(&regs[FLAGS], H_FLAG);

			regA += n;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			programCounter++;

			return 8;
		}



		///*ADC A,A*/ case 0x8F:
		//{
		//	herewasabreak;
		//}
		///*ADC A,B*/ case 0x88:
		//{
		//	herewasabreak;
		//}
		///*ADC A,C*/ case 0x89:
		//{
		//	herewasabreak;
		//}
		///*ADC A,D*/ case 0x8A:
		//{
		//	herewasabreak;
		//}
		///*ADC A,E*/ case 0x8B:
		//{
		//	herewasabreak;
		//}
		///*ADC A,H*/ case 0x8C:
		//{
		//	herewasabreak;
		//}
		///*ADC A,L*/ case 0x8D:
		//{
		//	herewasabreak;
		//}
		///*ADC A,(HL)*/ case 0x8E:
		//{
		//	herewasabreak;
		//}
		///*ADC A,#*/ case 0xCE:
		//{
		//	herewasabreak;
		//}


		///*SUB A*/ case 0x97:
		//{
		//	herewasabreak;
		//}
		///*SUB B*/ case 0x90:
		//{
		//	herewasabreak;
		//}
		///*SUB C*/ case 0x91:
		//{
		//	herewasabreak;
		//}
		///*SUB D*/ case 0x92:
		//{
		//	herewasabreak;
		//}
		///*SUB E*/ case 0x93:
		//{
		//	herewasabreak;
		//}
		///*SUB H*/ case 0x94:
		//{
		//	herewasabreak;
		//}
		///*SUB L*/ case 0x95:
		//{
		//	herewasabreak;
		//}
		///*SUB (HL)*/ case 0x96:
		//{
		//	herewasabreak;
		//}
		///*SUB #*/ case 0xD6:
		//{
		//	herewasabreak;
		//}


		///*SBC A,A*/ case 0x9F:
		//{
		//	herewasabreak;
		//}
		///*SBC A,B*/ case 0x98:
		//{
		//	herewasabreak;
		//}
		///*SBC A,C*/ case 0x99:
		//{
		//	herewasabreak;
		//}
		///*SBC A,D*/ case 0x9A:
		//{
		//	herewasabreak;
		//}
		///*SBC A,E*/ case 0x9B:
		//{
		//	herewasabreak;
		//}
		///*SBC A,H*/ case 0x9C:
		//{
		//	herewasabreak;
		//}
		///*SBC A,L*/ case 0x9D:
		//{
		//	herewasabreak;
		//}
		///*SBC A,(HL)*/ case 0x9E:
		//{
		//	herewasabreak;
		//}
		///*SBC A,#*/ case 0xDE:
		//{
		//	herewasabreak;
		//}



		/*AND r*/
		case 0xA7: case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5:
		{
			unsigned char &paramReg = regs[opcode & 0x7];

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			setBit(&regs[FLAGS], H_FLAG);
			
			regs[REG_A] &= paramReg;

			if (regs[REG_A] == 0)
				setBit(&regs[FLAGS], Z_FLAG);
			
			return 4;
		}
	
		///*AND (HL)*/ case 0xA6:
		//{
		//	herewasabreak;
		//}
		/*AND #*/ case 0xE6:
		{
			unsigned char n = readRam(programCounter);

			resetBit(&regs[FLAGS],Z_FLAG);
			resetBit(&regs[FLAGS],N_FLAG);
			resetBit(&regs[FLAGS],C_FLAG);
			setBit(&regs[FLAGS],H_FLAG);

			regs[REG_A] &= n;

			if (regs[REG_A] == 0)
				setBit(&regs[FLAGS],Z_FLAG);

			programCounter++;

			return 8;
		}


		/*OR r*/
		case 0xB7: case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5:
		{
			unsigned char &paramReg = regs[opcode & 0x7];

			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], Z_FLAG);

			regs[REG_A] |= paramReg;

			if (regs[REG_A] == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			return 4;
		}


		///*OR (HL)*/ case 0xB6:
		//{
		//	herewasabreak;
		//}
		/*OR #*/ case 0xF6:
		{
			unsigned char n = readRam(programCounter);
			unsigned char &regA = regs[REG_A];

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);

			regA |= n;

			if(regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			programCounter++;

			return 8;
		}


		/*XOR r*/
		case 0xAF: case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD:
		{
			unsigned char &paramReg = regs[opcode & 0x7];

			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], Z_FLAG);

			regs[REG_A] ^= paramReg;

			if (regs[REG_A] == 0x00)
				setBit(&regs[FLAGS], Z_FLAG);

			return 4;
		}




		///*XOR (HL)*/ case 0xAE:
		//{
		//	herewasabreak;
		//}

		///*XOR * */ case 0xEE:
		//{
		//	herewasabreak;
		//}



		/*CP r*/
		case 0xBF: case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD:
		{
			unsigned char &paramReg = regs[opcode & 0x7];
			unsigned char &regA = regs[REG_A];
			unsigned char res = regA - paramReg;
			unsigned char tmp = paramReg;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			setBit(&regs[FLAGS], N_FLAG);

			if ((((paramReg) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&regs[FLAGS], H_FLAG);


			if (res == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			if (regA < paramReg)
				setBit(&regs[FLAGS], C_FLAG);

			return 4;
		}
		///*CP (HL)*/ case 0xBE:
		//{
		//	herewasabreak;
		//}
		/*CP #*/ case 0xFE:
		{
			//unsigned char res = registers.A - ram[programCounter];
			unsigned char n = readRam(programCounter);
			unsigned char res = regs[REG_A] - n;

			if (res == 0)
			{
				setBit(&regs[FLAGS],Z_FLAG);
			}
			else
			{
				resetBit(&regs[FLAGS],Z_FLAG);
			}
			
			setBit(&regs[FLAGS],N_FLAG);

			if ((((regs[REG_A]) ^ n ^ res) & 0x10) >> 4)
				setBit(&regs[FLAGS], H_FLAG);
			else
				resetBit(&regs[FLAGS], H_FLAG);

			if (regs[REG_A] < n)
				setBit(&regs[FLAGS], C_FLAG);
			else
				resetBit(&regs[FLAGS], C_FLAG);

			programCounter++;

			return 8;
		}

		/*INC r*/
		case 0x3C: case 0x04: case 0x0C: case 0x14: case 0x1C: case 0x24: case 0x2C:
		{
			unsigned char &paramReg = regs[(opcode >> 3) & 0x7];

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);

			if ((paramReg & 0x0F) == 0x0F)
				setBit(&regs[FLAGS], H_FLAG);

			paramReg++;

			if (paramReg == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			return 4;
		}
		
		/*INC (HL)*/ case 0x34:
		{
			unsigned short address = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(address);

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);

			if ((val & 0x0F) == 0x0F)
				setBit(&regs[FLAGS], H_FLAG);

			val++;
			
			if (val == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			writeRam(address, val);

			return 12;
		}

		/*DEC r*/
		case 0x3D: case 0x05: case 0x0D: case 0x15: case 0x1D: case 0x25: case 0x2D:
		{
			unsigned char &paramReg = regs[(opcode >> 3) & 0x7];
			unsigned char res = paramReg - 1;
			unsigned char tmp = paramReg;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);

			if (tmp - 1 == 0x0)
				setBit(&regs[FLAGS], Z_FLAG);

			if ((((paramReg) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&regs[FLAGS], H_FLAG);

			tmp--;
			paramReg = tmp;

			return 4;
		}

		/*DEC (HL)*/ case 0x35:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(src);
			unsigned char res = val - 1;
			unsigned char tmp = val;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);

			if (tmp - 1 == 0x0)
				setBit(&regs[FLAGS], Z_FLAG);

			if ((((val) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&regs[FLAGS], H_FLAG);

			tmp--;
			val = tmp;

			writeRam(src, val);
			
			return 12;
		}


		/*ADD HL, ss */ case 0x09: case 0x19: case 0x29:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short regPairValue = readRegPairValue(pairNr);
			unsigned short dst = regs[REG_H] << 8 | regs[REG_L];

			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);

			if ((int)(regPairValue + dst) > 0xFFF)
				setBit(&regs[FLAGS], H_FLAG);

			if ((int)(regPairValue + dst) > 0xFFFF)
				setBit(&regs[FLAGS], C_FLAG);

			
			dst += regPairValue;

			regs[REG_H] = dst >> 8;
			regs[REG_L] = dst & 0xFF;

			return 8;
		}

		///*ADD HL,SP*/ case 0x39:
		//{
		//	herewasabreak;
		//}


		///*ADD SP, #*/ case 0xE8:
		//{
		//	herewasabreak;
		//}

		/*INC ss*/
		case 0x03: case 0x13: case 0x23:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short regPairValue = readRegPairValue(pairNr);

			regPairValue++;
			writeRegPairValue(pairNr, regPairValue);

			return 8;
		}

		///*INC SP*/ case 0x33:
		//{
		//	herewasabreak;
		//}

		/*DEC ss*/
		case 0x0B: case 0x1B: case 0x2B:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short regPairValue = readRegPairValue(pairNr);

			regPairValue--;
			writeRegPairValue(pairNr, regPairValue);

			return 8;
		}

		///*DEC SP*/ case 0x3B:
		//{
		//	herewasabreak;
		//}



		/*CB Prefix*/
		case 0xCB:
		{
			unsigned char cbOpcode = fetchOpcode();
			switch (cbOpcode)
			{
				/*SWAP r*/
				case 0x37: case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: 
				{
					unsigned char &paramReg = regs[cbOpcode & 0x7];
					unsigned char lowNibble = paramReg & 0x0F;
					unsigned char highNibble = paramReg & 0xF0;

					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], C_FLAG);
					resetBit(&regs[FLAGS], Z_FLAG);

					paramReg = lowNibble << 4 | highNibble >> 4;
					
					if (paramReg == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					return 8;
				}

				/*RES b, r*/
				case 0x87: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:
				{
					unsigned char bitNumber = (cbOpcode >> 3) & 0x07;
					unsigned char *paramReg = &regs[cbOpcode & 0x07];

					resetBit(paramReg, bitNumber);

					return 8;
				}
				
				/*RES b, (HL)*/
				case 0x86:
				{
					unsigned char bitNumber = (cbOpcode >> 3) & 0x07;
					unsigned short dst = regs[REG_H] << 8 | regs[REG_L];
					unsigned char val = readRam(dst);

					resetBit(&val, bitNumber);					
					writeRam(dst, val);

					return 16;
				}

				/*SLA r*/
				case 0x27: case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:
				{
					unsigned char &paramReg = regs[cbOpcode & 0x7];

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], C_FLAG);

					if (paramReg & 0x80)
						setBit(&regs[FLAGS], C_FLAG);

					paramReg <<= 1;

					if(paramReg == 0)
						setBit(&regs[FLAGS], Z_FLAG);


					return 8;
				}

				/*BIT b, (HL)*/
				/*These cases and the next switch ARE NOT TO BE SWITCHED IN ORDER*/
				case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: case 0x76: case 0x7E:
				{
					unsigned short dst = regs[REG_H] << 8 | regs[REG_L];
					unsigned char bitNumber = (cbOpcode >> 3) & 0x07;
					unsigned char val = readRam(dst);

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					setBit(&regs[FLAGS], H_FLAG);

					//if the targetet bit is NOT set, then SET the Z flag, else it was already REset above
					if (!isBitSet(&val, bitNumber))
						setBit(&regs[FLAGS], Z_FLAG);

					return 8;
				}

				/*SRL r*/
				case 0x3F: case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D:
				{
					unsigned char &paramReg = regs[cbOpcode & 0x7];

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], C_FLAG);

					if (paramReg & 0x1)
						setBit(&regs[FLAGS], C_FLAG);

					paramReg >>= 1;

					if(paramReg == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					return 8;
				}

				default:
				{
					/*BIT b, r*/
					switch (cbOpcode & 0x40)
					{
						case 0x40:
						{
							unsigned char bitNumber = (cbOpcode >> 3) & 0x07;
							unsigned char *paramReg = &regs[cbOpcode & 0x07];

							resetBit(&regs[FLAGS], Z_FLAG);
							resetBit(&regs[FLAGS], N_FLAG);
							setBit(&regs[FLAGS], H_FLAG);

							//if the targetet bit is NOT set, then SET the Z flag, else it was already reset above
							if (!isBitSet(paramReg, bitNumber))
								setBit(&regs[FLAGS], Z_FLAG);

							return 8;
						}
					}
					std::cout << "0xCB Opcode: " << std::hex << +cbOpcode << " not yet implemented.";
					return 0;
				}
			}
		}


		///*SWAP (HL)*/ case 0xCB36:
		//{
		//	herewasabreak;
		//}



		///*DAA*/ case 0x27:
		//{
		//	herewasabreak;
		//}

		/*CPL*/ case 0x2F:
		{
			setBit(&regs[FLAGS],N_FLAG);
			setBit(&regs[FLAGS],H_FLAG);

			regs[REG_A] = ~regs[REG_A];

			return 4;
		}


		///*CCF*/ case 0x3F:
		//{
		//	herewasabreak;
		//}

		///*SCF*/ case 0x37:
		//{
		//	herewasabreak;
		//}

		/*HALT*/ case 0x76:
		{
			haltFlag = true;
			
			return 4;
		}

		///*STOP*/ case 0x1000:
		//{


		//	herewasabreak;
		//}


		/*DI*/ case 0xF3:
		{
			disableImeFlag = 0x01;
			imeFlagCount = 1;

			return 4;
		}



		/*EI*/ case 0xFB:
		{
			enableImeFlag = 0x01;
			imeFlagCount = 1;

			return 4;
		}

		/*RLCA*/ case 0x07:
		{
			unsigned char &regA = regs[REG_A];
			unsigned tmp = regA;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);

			regA <<= 1;

			if (tmp & 0x80)
			{
				setBit(&regs[FLAGS], C_FLAG);
				setBit(&regA, 1);
			}

			if(regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);


			return 4;
		}

		///*RLA*/ case 0x17:
		//{
		//	herewasabreak;
		//}

		///*RRCA*/ case 0x0F:
		//{
		//	herewasabreak;
		//}

		///*RRA*/ case 0x1F:
		//{
		//	herewasabreak;
		//}

		///*RLC A*/ case 0xCB07:
		//{
		//	herewasabreak;
		//}
		///*RLC B*/ case 0xCB00:
		//{
		//	herewasabreak;
		//}
		///*RLC C*/ case 0xCB01:
		//{
		//	herewasabreak;
		//}
		///*RLC D*/ case 0xCB02:
		//{
		//	herewasabreak;
		//}
		///*RLC E*/ case 0xCB03:
		//{
		//	herewasabreak;
		//}
		///*RLC H*/ case 0xCB04:
		//{
		//	herewasabreak;
		//}
		///*RLC L*/ case 0xCB05:
		//{
		//	herewasabreak;
		//}
		///*RLC (HL)*/ case 0xCB06:
		//{
		//	herewasabreak;
		//}


		///*RL A*/ case 0xCB17:
		//{
		//	herewasabreak;
		//}
		///*RL B*/ case 0xCB10:
		//{
		//	herewasabreak;
		//}
		///*RL C*/ case 0xCB11:
		//{
		//	herewasabreak;
		//}
		///*RL D*/ case 0xCB12:
		//{
		//	herewasabreak;
		//}
		///*RL E*/ case 0xCB13:
		//{
		//	herewasabreak;
		//}
		///*RL H*/ case 0xCB14:
		//{
		//	herewasabreak;
		//}
		///*RL L*/ case 0xCB15:
		//{
		//	herewasabreak;
		//}
		///*RL (HL)*/ case 0xCB16:
		//{
		//	herewasabreak;
		//}



		///*RRC A*/ case 0xCB0F:
		//{
		//	herewasabreak;
		//}
		///*RRC B*/ case 0xCB08:
		//{
		//	herewasabreak;
		//}
		///*RRC C*/ case 0xCB09:
		//{
		//	herewasabreak;
		//}
		///*RRC D*/ case 0xCB0A:
		//{
		//	herewasabreak;
		//}
		///*RRC E*/ case 0xCB0B:
		//{
		//	herewasabreak;
		//}
		///*RRC H*/ case 0xCB0C:
		//{
		//	herewasabreak;
		//}
		///*RRC L*/ case 0xCB0D:
		//{
		//	herewasabreak;
		//}
		///*RRC (HL)*/ case 0xCB0E:
		//{
		//	herewasabreak;
		//}

		///*RR A */case 0xCB1F:
		//{
		//	herewasabreak;
		//}
		///*RR B */case 0xCB18:
		//{
		//	herewasabreak;
		//}
		///*RR C */case 0xCB19:
		//{
		//	herewasabreak;
		//}
		///*RR D */case 0xCB1A:
		//{
		//	herewasabreak;
		//}
		///*RR E */case 0xCB1B:
		//{
		//	herewasabreak;
		//}
		///*RR H */case 0xCB1C:
		//{
		//	herewasabreak;
		//}
		///*RR L */case 0xCB1D:
		//{
		//	herewasabreak;
		//}
		///*RR (HL)*/case 0xCB1E:
		//{
		//	herewasabreak;
		//}



		///*SRA A*/ case 0xCB2F:
		//{
		//	herewasabreak;
		//}
		///*SRA B*/ case 0xCB28:
		//{
		//	herewasabreak;
		//}
		///*SRA C*/ case 0xCB29:
		//{
		//	herewasabreak;
		//}
		///*SRA D*/ case 0xCB2A:
		//{
		//	herewasabreak;
		//}
		///*SRA E*/ case 0xCB2B:
		//{
		//	herewasabreak;
		//}
		///*SRA H*/ case 0xCB2C:
		//{
		//	herewasabreak;
		//}
		///*SRA L*/ case 0xCB2D:
		//{
		//	herewasabreak;
		//}
		///*SRA (HL)*/ case 0xCB2E:
		//{
		//	herewasabreak;
		//}


		///*SRL (HL)*/ case 0xCB3E:
		//{
		//	herewasabreak;
		//}





		///*SET b,A*/ case 0xCBC7:
		//{
		//	herewasabreak;
		//}
		///*SET b,B*/ case 0xCBC0:
		//{
		//	herewasabreak;
		//}
		///*SET b,C*/ case 0xCBC1:
		//{
		//	herewasabreak;
		//}
		///*SET b,D*/ case 0xCBC2:
		//{
		//	herewasabreak;
		//}
		///*SET b,E*/ case 0xCBC3:
		//{
		//	herewasabreak;
		//}
		///*SET b,H*/ case 0xCBC4:
		//{
		//	herewasabreak;
		//}
		///*SET b,L*/ case 0xCBC5:
		//{
		//	herewasabreak;
		//}
		///*SET b,(HL)*/ case 0xCBC6:
		//{
		//	herewasabreak;
		//}


		/*JP nn*/ case 0xC3:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));
			programCounter = newAddressToJumpTo;

			return 12;
		}

		/*JP NZ,nn*/ case 0xC2:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));

			if (!isBitSet(&regs[FLAGS], Z_FLAG))
				programCounter = newAddressToJumpTo;
			else
				programCounter += 2;

			return 12;
		}

		/*JP Z,nn*/ case 0xCA:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));

			if (isBitSet(&regs[FLAGS], Z_FLAG))
				programCounter = newAddressToJumpTo;
			else
				programCounter += 2;

			return 12;
		}

		/*JP NC,nn*/ case 0xD2:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));

			if (!isBitSet(&regs[FLAGS], C_FLAG))
				programCounter = newAddressToJumpTo;
			else
				programCounter += 2;

			return 12;
		}

		/*JP C,nn*/ case 0xDA:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));

			if (isBitSet(&regs[FLAGS], C_FLAG))
				programCounter = newAddressToJumpTo;
			else
				programCounter += 2;

			return 12;
		}

		/*JP (HL)*/ case 0xE9:
		{
			programCounter = regs[REG_H] << 8 | regs[REG_L];

			return 4;
		}

		/*JR n*/ case 0x18:
		{
			unsigned char n = readRam(programCounter);

			programCounter += (signed char)n;
			programCounter++;

			return 8;
		}

		/*JR NZ,**/ case 0x20:
		{
			if (isBitSet(&regs[FLAGS], Z_FLAG) == false)
			{
				programCounter += (signed char)readRam(programCounter);
				programCounter++;
			}
			else programCounter++;

			return 8;
		}
		/*JR Z,* */case 0x28:
		{
			if (isBitSet(&regs[FLAGS], Z_FLAG))
			{
				programCounter += (signed char)readRam(programCounter);
				programCounter++;
			}
			else programCounter++;

			return 8; 
		}
		/*JR NC,**/ case 0x30:
		{
			if (isBitSet(&regs[FLAGS], C_FLAG) == false)
			{
				programCounter += (signed char)readRam(programCounter);
				programCounter++;
			}
			else programCounter++;

			return 8;

		}
		/*JR C,* */case 0x38:
		{
			if (isBitSet(&regs[FLAGS], C_FLAG))
			{
				programCounter += (signed char)readRam(programCounter);
				programCounter++;
			}
			else programCounter++;

			return 8;
		}

		/*call nn*/ case 0xCD:
		{
			//push address of NEXT instruction to stack. since it's a 16 bit parameter (nn) we do +2 instead of +1
			pushToStack(programCounter+2);
			unsigned char nHighByte = readRam(programCounter);
			unsigned char nLowByte = readRam(programCounter + 1);

			//LSB first
			programCounter = nLowByte << 8 | nHighByte;

			
			return 12;
		}

		/*CALL NZ,nn*/ case 0xC4:
		{
			unsigned short nn = readRam(programCounter) << 8 | readRam(programCounter + 1);
			if (isBitSet(&regs[FLAGS], Z_FLAG))
				programCounter = nn;
			else
				programCounter += 2;

			return 12;
		}

		///*CALL Z,nn*/ case 0xCC:
		//{
		//	herewasabreak;
		//}
		///*CALL NC,nn*/ case 0xD4:
		//{
		//	herewasabreak;
		//}
		///*CALL C,nn*/ case 0xDC:
		//{
		//	herewasabreak;
		//}

		///*RST 00H*/ case 0xC7:
		//{
		//	herewasabreak;
		//}
		///*RST 08H*/ case 0xCF:
		//{
		//	herewasabreak;
		//}
		///*RST 10H*/ case 0xD7:
		//{
		//	herewasabreak;
		//}
		///*RST 18H*/ case 0xDF:
		//{
		//	herewasabreak;
		//}
		///*RST 20H*/ case 0xE7:
		//{
		//	herewasabreak;
		//}
		/*RST 28H*/ case 0xEF:
		{
			pushToStack(programCounter);
			programCounter = 0x28;
			return 32;
		}
		///*RST 30H*/ case 0xF7:
		//{
		//	herewasabreak;
		//}
		/*RST 38H*/ case 0xFF:
		{
			//push PRESENT address onto stack, hence we need to pass programCounter - 1
			pushToStack(programCounter - 1);
			programCounter = 0x38;
			return 32;
		}

		/*RET -/-*/ case 0xC9:
		{
			programCounter = popFromStack();
			return 8;
		}

		/*RET NZ*/ case 0xC0:
		{
			if (isBitSet(&regs[FLAGS], Z_FLAG) == false)
				programCounter = popFromStack();
			
			return 8;
		}
		/*RET Z*/ case 0xC8:
		{

			if (isBitSet(&regs[FLAGS], Z_FLAG) == true)
				programCounter = popFromStack();

			return 8;
		}
		///*RET NC */case 0xD0:
		//{
		//	herewasabreak;
		//}
		///*RET C*/ case 0xD8:
		//{
		//	herewasabreak;
		//}

		/*RETI -/-*/ case 0xD9:
		{
			programCounter = popFromStack();
			imeFlag = 1;
			return 8;
		}


		default:
		{
			std::cout << "Following opcode needs to be implemented: " << std::hex << +opcode << std::endl;
			std::cout.flush();
		};
	}

}

//All opcodes that address register pairs have the register pair number on bit 4 and bit 5. 
int getRegPairNumber(unsigned short opcode)
{
	int pairNr = (opcode >> 4) & 0x03;
	return pairNr;
}

//Only works with the register pairs BC, DE, HL. AF doesn't work since index-wise its order is reversed
//BC: 00 -> B: 00, C:01
//DE: 01 -> D: 02, C:03 etc.
//With doubling the regPairNumber you get the first part of the pair, 
//with incrementing the doulbed regPairNumber you get the second part of the pair
unsigned short readRegPairValue(int pairNr)
{
	unsigned short pairValue = regs[pairNr * 2] << 8 | regs[pairNr * 2 + 1];

	return pairValue;
}

void writeRegPairValue(int pairNr, unsigned short pairValue)
{
	regs[pairNr * 2] = (pairValue & 0xFF00) >> 8;
	regs[pairNr * 2 + 1] = pairValue & 0x00FF;
}






bool loadCartridge(const char *romName)
{
	std::FILE* fp = fopen(romName, "rb");
	if (!fp)
	{
		std::cout << "ROM could not be loaded." << std::endl;
		return false;
	}
	else
	{
		fread(cartridgeContent, 1, 0x10000, fp);
		memcpy(ram, cartridgeContent, sizeof(char)*0x8000);
		return true;
	}


}

void setBit(unsigned char * value, int bitNumber)
{
	*value |= (1 << bitNumber);
}

void resetBit(unsigned char * value, int bitNumber)
{
	*value &= ~(1 << bitNumber);
}

bool isBitSet(unsigned char * value, int bitNumber)
{
	return !!((*value & (1 << bitNumber)));
}