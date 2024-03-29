#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "cpu.h"
#include "input.h"
#include <fstream>
#include <iostream>

#ifdef _DEBUG
int debugCount = 0;
#endif


int cyclesScanLine;
unsigned char ram[65536];
unsigned short stackPointer, programCounter;
int timerCounter, dividerCounter;

//Order is: B, C, D, E, H, L, F, A
//B: 000, C: 001, D: 010,
//E: 011, H: 100, L: 101, F: 110, A: 111
unsigned char regs[8];


/*Interrupts*/
/*
Bit 0: V-Blank Interupt
Bit 1: LCD Interupt
Bit 2: Timer Interupt
Bit 4: Joypad Interupt
*/
char imeFlag = 0x00, disableImeFlag = 0x00, enableImeFlag = 0x00, imeFlagCount = 0;
bool haltFlag = false, stopFlag = false;

const unsigned char REG_A = 7;
const unsigned char REG_B = 0;
const unsigned char REG_C = 1;
const unsigned char REG_D = 2;
const unsigned char REG_E = 3;
const unsigned char REG_H = 4;
const unsigned char REG_L = 5;
const unsigned char FLAGS = 6;

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




void initialize()
{
#ifdef _DEBUG
	freopen("output.txt", "w", stdout);

	//prevent automatic flush of cout after every "\n", speeds up extremely
	std::setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);
#endif

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

	timerCounter = 0;
	dividerCounter = 0;
	
	unsigned char typeOfCartridge;
	typeOfCartridge = readRam(0x147);
	switch (typeOfCartridge)
	{
		case 0:
		{
			mbc1 = false;
		}
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
		default: 
		{
			std::cout << "Unhandled memory controller: " << std::hex << typeOfCartridge;
			return;
		}
	}


}

void updateTimers(int cycles)
{
	updateDividerRegister(cycles);

	// the clock must be enabled to update the clock
	if (isTimerEnabled())
	{
		timerCounter -= cycles;

		// enough cpu clock cycles have happened to update the timer
		if (timerCounter <= 0)
		{
			// reset m_TimerTracer to the correct value
			setClockFrequency();

			// timer about to overflow
			if (readRam(TIMA) == 255)
			{
				writeRam(TIMA, readRam(TMA));
				requestInterrupt(2);
			}
			else
			{
				writeRam(TIMA, readRam(TIMA) + 1);
			}
		}
	}
}

void updateDividerRegister(int cycles)
{
	dividerCounter += cycles;
	while(dividerCounter >= 255)
	{
		dividerCounter = 0;
		ram[DIV]++;
	}
}


bool isTimerEnabled()
{
	unsigned char tac = readRam(TAC);
	return (isBitSet(&tac, 2));
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
	//std::cout << std::hex << address << "\n";
	if (address < 0x8000)
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

	else if (address >= 0xE000 && address < 0xFE00)
	{
		ram[address] = data;
		writeRam(address - 0x2000, data);
	}

	else if (address >= 0xFEA0 && address < 0xFF00)
	{

	}

	// FF44 shows which horizontal scanline is currently being draw. Writing here resets it
	else if (address == 0xFF44)
	{
		ram[0xFF44] = 0;
	}
	else if (address == LCDC_LY)
	{
		ram[LCDC_LY] = 0;
	}
	else if (address == 0xFF46)
	{
		startDmaTransfer(data);
	}

	/*Restricted area*/
	else if (address >= 0xFF4C && address < 0xFF80)
	{

	}

	//trap the divider register
	else if (address == DIV)
	{
		ram[DIV] = 0;
		dividerCounter = 0;
	}

	else if (address == TAC)
	{
		unsigned char currentFreq = getClockFrequency();
		ram[TAC] = data;
		unsigned char newFreq = getClockFrequency();

		if (currentFreq != newFreq)
		{
			setClockFrequency();
		}
	}

	else
	{
		ram[address] = data;
	}


}

unsigned char getClockFrequency()
{
	//the first 2 bits are the clock frequency
	return readRam(TAC) & 0x3;
}

void setClockFrequency()
{
	unsigned char freq = getClockFrequency();
	switch (freq)
	{
		//timerCounter is set to CLOCKSPEED/frequency
		case 0: timerCounter = 1024; break; // freq 4096
		case 1: timerCounter = 16; break;// freq 262144
		case 2: timerCounter = 64; break;// freq 65536
		case 3: timerCounter = 256; break;// freq 16382
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
	else if (address >= 0x2000 && address < 0x4000)
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
		currentRamBank = 0;
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
	writeRam(stackPointer, lowHalf);
	writeRam(stackPointer + 1, highHalf);
}

unsigned short popFromStack()
{
	unsigned short lowHalf = readRam(stackPointer);
	unsigned short highHalf = readRam(stackPointer+1);
	unsigned short poppedValue = (highHalf << 8) | lowHalf;

	//delete the popped value from stack
	//writeRam(stackPointer, 0x0);
	//writeRam(stackPointer+1, 0x0);

	//increment SP so that it points to the most upper stack value 
	stackPointer += 2;

	return poppedValue;
}

void requestInterrupt(int interruptNumber)
{
	unsigned char irRequestFlagStatus = readRam(IR_REQUEST_ADDRESS);
	setBit(&irRequestFlagStatus, interruptNumber);
	//irRequestFlagStatus = irRequestFlagStatus | interruptNumber;
	writeRam(IR_REQUEST_ADDRESS, irRequestFlagStatus);
}

void checkInterruptRequests()
{
	unsigned char irRequestFlagStatus = readRam(IR_REQUEST_ADDRESS);
	unsigned char irEnableFlagStatus = readRam(IR_ENABLE_ADDRESS);

	if (irRequestFlagStatus > 0)
	{
		if (isBitSet(&irRequestFlagStatus,VERTICAL_BLANKING))
		{
			if (isBitSet(&irEnableFlagStatus,VERTICAL_BLANKING))
			{
				//haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(VERTICAL_BLANKING);
			}
		}
		else if (isBitSet(&irRequestFlagStatus,LCDC))
		{
			if (isBitSet(&irEnableFlagStatus,LCDC))
			{
				//haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(LCDC);

			}
		}
		
		else if (isBitSet(&irRequestFlagStatus,TIMER_OVERFLOW))
		{
			if (isBitSet(&irEnableFlagStatus,TIMER_OVERFLOW))
			{
				//haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(TIMER_OVERFLOW);

			}
		}
		else if (isBitSet(&irRequestFlagStatus,SERIAL_IO_COMPLETION))
		{
			if (isBitSet(&irEnableFlagStatus,SERIAL_IO_COMPLETION))
			{
				//haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(SERIAL_IO_COMPLETION);

			}
		}
		else if (isBitSet(&irRequestFlagStatus,JOYPAD))
		{
			if (isBitSet(&irEnableFlagStatus,JOYPAD))
			{
				//haltFlag = false;
				if (imeFlag == 1)
					serviceInterrupt(JOYPAD);

			}
		}



	}
}

void serviceInterrupt(int interruptNumber)
{
	haltFlag = false;
	imeFlag = 0;
	unsigned char irRequestFlagStatus = readRam(IR_REQUEST_ADDRESS);

	//clear the interrupt bit in the status byte
	//resetBit(&irRequestFlagStatus,interruptNumber);
	irRequestFlagStatus = 0;
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
	if (haltFlag || stopFlag)
		return 0;

	unsigned char opcode = 0;

	opcode = readRam(programCounter);

	programCounter++;
	
	return opcode;
}

int executeOpcode(unsigned char opcode)
{

#ifdef _DEBUG

	//std::cout << "A:" << std::uppercase << std::hex << (regs[REG_A] > 0xF ? "" : "0") << +regs[REG_A];
	//std::cout << " F:" << (isBitSet(&regs[FLAGS], Z_FLAG) ? "Z" : "-") <<
	//	(isBitSet(&regs[FLAGS], N_FLAG) ? "N" : "-") <<
	//	(isBitSet(&regs[FLAGS], H_FLAG) ? "H" : "-") <<
	//	(isBitSet(&regs[FLAGS], C_FLAG) ? "C" : "-");
	//std::cout << " BC:" << std::nouppercase <<std::hex << (regs[REG_B] > 0xF ? "" : "0") << +regs[REG_B] << (regs[REG_C] > 0xF ? "" : "0") << +regs[REG_C];
	//std::cout << " DE:" << std::nouppercase <<std::hex << (regs[REG_D] > 0xF ? "" : "0") << +regs[REG_D] << (regs[REG_E] > 0xF ? "" : "0") << +regs[REG_E];
	//std::cout << " HL:" << std::nouppercase <<std::hex << (regs[REG_H] > 0xF ? "" : "0") << +regs[REG_H] << (regs[REG_L] > 0xF ? "" : "0") << +regs[REG_L];
	//std::cout << " SP:" << std::nouppercase << std::hex << +stackPointer;
	//std::cout << " PC:" << std::nouppercase << std::hex << (programCounter > 0xFFF ? "" : (programCounter > 0xFF ? "0" : (programCounter > 0xF ? "00" : "0"))) << +(programCounter-1);
	//std::cout << " op: " << std::uppercase << std::hex << (opcode > 0xF ? "" : "0") << +opcode << "\n";
#endif

	if (haltFlag || stopFlag)
		return 4;


	/*EI (0xFB) and DI (0xF3) are effective AFTER the next instruction,
	following if statement enables that*/
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

	/*Switch with all opcodes*/
	switch (opcode)
	{
		/*NOP*/
		case 0x00:
		{
			return 4;
		}

		//STOP CPU & LCD display until button pressed.
		case 0x10:
		{
			stopFlag = true;

			//halt LCD 
			resetBit(&ram[LCDC_CTRL], 7);

			//its a 2 byte command, hence increment pc
			programCounter++;
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


		/*LD r1, r2*/
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
			unsigned short srcAddress = nlowByte << 8 | nHighByte;

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
			unsigned char nHighByte = readRam(programCounter + 1);
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
			unsigned short regHL = regs[REG_H] << 8 | regs[REG_L];
			//unsigned short regHL = ((regs[REG_H] << 8) & 0xFF00) + (regs[REG_L] & 0xFF);

			regs[REG_A] = readRam(regHL);
			regHL--;
			regs[REG_H] = (regHL >> 8) & 0xFF;
			regs[REG_L] = regHL & 0xFF;
			return 8;
		}

		/*LD (HLD),A */
		/*LD (HL-),A */
		/*LDD (HL),A*/
		case 0x32:
		{
			unsigned short regHL = regs[REG_H] << 8 | regs[REG_L];

			//unsigned short regHL = ((regs[REG_H] << 8) & 0xFF00) + (regs[REG_L] & 0xFF);

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
			regs[REG_A] = readRam(0xFF00 + n);
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
			unsigned short regHL = regs[REG_H] << 8 | regs[REG_L];
			stackPointer = regHL;


			return 8;

		}

		/*LD HL,SP+n*/ case 0xF8: {
			signed char n = readRam(programCounter);
			unsigned short res = 0;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
 
			if (((((unsigned char)stackPointer) & 0xF) + (((unsigned char)n) & 0xF)) > 0xF)
				setBit(&regs[FLAGS], H_FLAG);

			if ((((unsigned char)(stackPointer)) + (((unsigned char)n))) > 0xFF)
				setBit(&regs[FLAGS], C_FLAG);
 
			res = stackPointer + (signed char)n;
			regs[REG_H] = res >> 8;
			regs[REG_L] = res & 0xFF;

			programCounter++;

			return 12;

		}


		/*LD (nn),SP*/ case 0x08: {
			unsigned short nn = readRam(programCounter + 1) << 8 | readRam(programCounter);

			writeRam(nn, stackPointer & 0xFF);
			writeRam(nn + 1, (stackPointer & 0xFF00) >> 8);

			programCounter += 2;

			return 20;


		}



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
			/*Lower nibble is ignored*/
			regs[FLAGS] = (popVal & 0x00F0);

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

			if (((regA & 0x0F) + (paramReg & 0x0F)) > 0x0F)
				setBit(&regs[FLAGS], H_FLAG);

			if ((int)(regA + paramReg) > 0xFF)
				setBit(&regs[FLAGS], C_FLAG);

			regA += paramReg;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			return 4;
		}


		/*ADD A,(HL)*/ 
		case 0x86:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(src);
			unsigned char &regA = regs[REG_A];

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);

			if (((regA & 0x0F) + (val & 0x0F)) > 0x0F)
				setBit(&regs[FLAGS], H_FLAG);

			if ((int)(regA + val) > 0xFF)
				setBit(&regs[FLAGS], C_FLAG);

			regA += val;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			return 8;
		}

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


		/*ADC A, r*/
		case 0x8F: case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D:
		{
			unsigned char &paramReg = regs[opcode & 0x7];
			unsigned char &regA = regs[REG_A];
			unsigned char carryFlag = isBitSet(&regs[FLAGS], C_FLAG) ? 1 : 0;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);

			if (((regA & 0x0F) + (paramReg & 0x0F) + carryFlag) > 0x0F)
				setBit(&regs[FLAGS], H_FLAG);

			if ((int)(regA + paramReg + carryFlag) > 0xFF)
				setBit(&regs[FLAGS], C_FLAG);

			regA += paramReg + carryFlag;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			return 4;
		}


		/*ADC A,(HL)*/ case 0x8E:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(src);
			unsigned char &regA = regs[REG_A];
			unsigned char carryFlag = isBitSet(&regs[FLAGS], C_FLAG) ? 1 : 0;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);

			if (((regA & 0x0F) + (val & 0x0F)+carryFlag) > 0x0F)
				setBit(&regs[FLAGS], H_FLAG);

			if ((int)(regA + val+carryFlag) > 0xFF)
				setBit(&regs[FLAGS], C_FLAG);

			regA += val + carryFlag;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			return 8;
		}
		/*ADC A,#*/ case 0xCE:
		{
			unsigned char n = readRam(programCounter);
			unsigned char &regA = regs[REG_A];
			unsigned char carryFlag = isBitSet(&regs[FLAGS], C_FLAG) ? 1 : 0;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);

			if (((regA & 0x0F) + (n & 0x0F)+carryFlag) > 0x0F)
				setBit(&regs[FLAGS], H_FLAG);

			if (((int)(regA + n + carryFlag)) > 0xFF)
				setBit(&regs[FLAGS], C_FLAG);

			regA += n + carryFlag;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			programCounter++;

			return 8;
		}

		/*SUB r*/
		case 0x97: case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95:
		{
			unsigned char &paramReg = regs[opcode & 0x7];
			unsigned char &regA = regs[REG_A];
			unsigned char res = regA - paramReg;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);


			/*if ((((regs[REG_A]) ^ paramReg ^ res) & 0x10) >> 4)
				setBit(&regs[FLAGS], H_FLAG);*/
			if ((regA & 0xF) < (paramReg & 0xF))
				setBit(&regs[FLAGS], H_FLAG);

			if (regA < paramReg)
				setBit(&regs[FLAGS], C_FLAG);

			regA -= paramReg;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			return 4;
		}
 
		/*SUB (HL)*/ case 0x96:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(src);
			unsigned char &regA = regs[REG_A];
			unsigned char res = regA - val;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);


			//if ((((regs[REG_A]) ^ val ^ res) & 0x10) >> 4)
			//	setBit(&regs[FLAGS], H_FLAG);
			if ((regA & 0xF) < (val & 0xF))
				setBit(&regs[FLAGS], H_FLAG);

			if (regs[REG_A] < val)
				setBit(&regs[FLAGS], C_FLAG);
			
			regA -= val;

			if(regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);


			return 8;
		}

		/*SUB #*/ case 0xD6:
		{
			unsigned char n = readRam(programCounter);
			std::cout.flush();
			unsigned char &regA = regs[REG_A];
			unsigned char res = regA - n;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);


			/*if ((((regs[REG_A]) ^ subtrahend ^ res) & 0x10) >> 4)
				setBit(&regs[FLAGS], H_FLAG);*/
			if ((regA & 0xF) < (n & 0xF))
				setBit(&regs[FLAGS], H_FLAG);

			if (regs[REG_A] < n)
				setBit(&regs[FLAGS], C_FLAG);

			regA -= n;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			programCounter++;

			return 8;
		}

		/*SBC A, r*/
		case 0x9F: case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D:
		{
			unsigned char &paramReg = regs[opcode & 0x7];
			unsigned char &regA = regs[REG_A];
			unsigned char carryFlag = isBitSet(&regs[FLAGS], C_FLAG) ? 1 : 0;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);

			if ((regA & 0xF) < ((paramReg & 0xF)+carryFlag))
				setBit(&regs[FLAGS], H_FLAG);

			if (regA < (paramReg+carryFlag))
				setBit(&regs[FLAGS], C_FLAG);

			regA -= (paramReg+carryFlag);

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);


			
			return 4;
		}
		/*SBC A,(HL)*/ case 0x9E:
		{
			unsigned short regHL = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(regHL);
			unsigned char &regA = regs[REG_A];
			unsigned char carryFlag = isBitSet(&regs[FLAGS], C_FLAG) ? 1 : 0;
 

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);

			if ((regA & 0xF) < ((val & 0xF)+carryFlag))
				setBit(&regs[FLAGS], H_FLAG);

			if (regA < (val+carryFlag))
				setBit(&regs[FLAGS], C_FLAG);

			regA -= (val + carryFlag);

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);


			return 8;
		}
		/*SBC A,#*/ case 0xDE:
		{
			unsigned char n = readRam(programCounter);
			unsigned char &regA = regs[REG_A];
			unsigned char carryFlag = isBitSet(&regs[FLAGS], C_FLAG) ? 1 : 0;
			unsigned char subtrahend = n + carryFlag;
			unsigned char res = regA - n;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);

			if ((regA & 0xF) < ((n & 0xF)+carryFlag))
				setBit(&regs[FLAGS], H_FLAG);

			//should be correct to test n and not subtrahend, but i dunno
			if (regA < (n+carryFlag))
				setBit(&regs[FLAGS], C_FLAG);

			regA -= subtrahend;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			programCounter++;

			return 8;

			
		}



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
	
		/*AND (HL)*/ case 0xA6:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(src);

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			setBit(&regs[FLAGS], H_FLAG);

			regs[REG_A] &= val;

			if (regs[REG_A] == 0)
				setBit(&regs[FLAGS], Z_FLAG);


			return 8;
		}

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


		/*OR (HL)*/ case 0xB6:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(src);
			unsigned char &regA = regs[REG_A];
			
			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);

			regA |= val;

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);


			return 8;
		}

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




		/*XOR (HL)*/ case 0xAE:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(src);

			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], Z_FLAG);

			regs[REG_A] ^= val;

			if (regs[REG_A] == 0x00)
				setBit(&regs[FLAGS], Z_FLAG);

			return 8;
		}

		/*XOR * */ case 0xEE:
		{
			unsigned char n = readRam(programCounter);

			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);
			resetBit(&regs[FLAGS], Z_FLAG);

			regs[REG_A] ^= n;

			if (regs[REG_A] == 0x00)
				setBit(&regs[FLAGS], Z_FLAG);

			programCounter++;

			return 8;
		}



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

			//if ((((paramReg) ^ (1) ^ res) & 0x10) >> 4)
			//	setBit(&regs[FLAGS], H_FLAG);
			if((regA&0xF) < (paramReg&0xF))
				setBit(&regs[FLAGS], H_FLAG);

			
			if (res == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			if (regA < paramReg)
				setBit(&regs[FLAGS], C_FLAG);

			return 4;
		}
		/*CP (HL)*/ case 0xBE:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(src);
			unsigned char &regA = regs[REG_A];
			unsigned char res = regA - val;

				resetBit(&regs[FLAGS], H_FLAG);
				resetBit(&regs[FLAGS], C_FLAG);

			if (res == 0)
			{
				setBit(&regs[FLAGS], Z_FLAG);
			}
			else
			{
				resetBit(&regs[FLAGS], Z_FLAG);
			}

			setBit(&regs[FLAGS], N_FLAG);

			if ((regA & 0xF) < (val & 0xF))
				setBit(&regs[FLAGS], H_FLAG);

			if (regs[REG_A] < val)
				setBit(&regs[FLAGS], C_FLAG);
			


			return 8;
		}
		/*CP #*/ case 0xFE:
		{
			unsigned char n = readRam(programCounter);
			unsigned char &regA = regs[REG_A];
			unsigned char res = regA - n;

				resetBit(&regs[FLAGS], H_FLAG);
				resetBit(&regs[FLAGS], C_FLAG);
			
				if (res == 0)
			{
				setBit(&regs[FLAGS],Z_FLAG);
			}
			else
			{
				resetBit(&regs[FLAGS],Z_FLAG);
			}
			
			setBit(&regs[FLAGS],N_FLAG);

			if ((regA & 0xF) < (n & 0xF))
				setBit(&regs[FLAGS], H_FLAG);

			if (regs[REG_A] < n)
				setBit(&regs[FLAGS], C_FLAG);
			 

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

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);

			if((paramReg & 0xF) == 0)
				setBit(&regs[FLAGS], H_FLAG); 

			if (res == 0x0)
				setBit(&regs[FLAGS], Z_FLAG);

			/*if ((((paramReg) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&regs[FLAGS], H_FLAG);*/

			paramReg--;

			return 4;
		}

		/*DEC (HL)*/ case 0x35:
		{
			unsigned short src = regs[REG_H] << 8 | regs[REG_L];
			unsigned char val = readRam(src);
			unsigned char res = val - 1;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], N_FLAG);

			if ((val & 0xF) == 0)
				setBit(&regs[FLAGS], H_FLAG);

			if (res == 0x0)
				setBit(&regs[FLAGS], Z_FLAG);

			val--;

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

			if (((regPairValue&0xFFF) + (dst&0xFFF))> 0xFFF)
				setBit(&regs[FLAGS], H_FLAG);

			if ((int)(regPairValue + dst) > 0xFFFF)
				setBit(&regs[FLAGS], C_FLAG);

			
			dst += regPairValue;

			regs[REG_H] = dst >> 8;
			regs[REG_L] = dst & 0xFF;

			return 8;
		}

		/*ADD HL,SP*/ case 0x39:
		{
			unsigned short regHL = regs[REG_H] << 8 | regs[REG_L];
			//unsigned short regHL = ((regs[REG_H] << 8) & 0xFF00) + (regs[REG_L] & 0xFF);

			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);

			if (((stackPointer & 0xFFF) + (regHL & 0xFFF)) > 0xFFF)
				setBit(&regs[FLAGS], H_FLAG);

			if ((int)(stackPointer + regHL) > 0xFFFF)
				setBit(&regs[FLAGS], C_FLAG);

			regHL += stackPointer;

			regs[REG_H] = regHL >> 8;
			regs[REG_L] = regHL & 0xFF;

			return 8;
		}


		/*ADD SP, #*/ case 0xE8:
		{
			signed char n = readRam(programCounter);
			
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);


			if ((  ( ((unsigned char)stackPointer) & 0xF) + (((unsigned char)n) & 0xF)) > 0xF)
				setBit(&regs[FLAGS], H_FLAG);

			if ((  ((unsigned char)(stackPointer)) + (((unsigned char)n))) > 0xFF)
				setBit(&regs[FLAGS], C_FLAG);

			stackPointer = (signed char)n + stackPointer;

			programCounter++;

			return 16;
		}

		/*INC ss*/
		case 0x03: case 0x13: case 0x23:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short regPairValue = readRegPairValue(pairNr);

			regPairValue++;
			writeRegPairValue(pairNr, regPairValue);

			return 8;
		}

		/*INC SP*/ case 0x33:
		{
			stackPointer += 1;
			return 8;
		}

		/*DEC ss*/
		case 0x0B: case 0x1B: case 0x2B:
		{
			int pairNr = getRegPairNumber(opcode);
			unsigned short regPairValue = readRegPairValue(pairNr);

			regPairValue--;
			writeRegPairValue(pairNr, regPairValue);

			return 8;
		}

		/*DEC SP*/ case 0x3B:
		{
			stackPointer -= 1;
			return 8;
		}



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

				/*SWAP (HL)*/
				case 0x36:
				{
					unsigned short src = regs[REG_H] << 8 | regs[REG_L];
					unsigned char val = readRam(src);
					unsigned char lowNibble = val & 0x0F;
					unsigned char highNibble = val & 0xF0;

					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], C_FLAG);
					resetBit(&regs[FLAGS], Z_FLAG);

					val = lowNibble << 4 | highNibble >> 4;

					if (val == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					writeRam(src, val);

					return 16;
				}

				/*RES b, r*/
				case 0x87: case 0x8F: case 0x97: case 0x9F: case 0xA7: case 0xAF: case 0xB7: case 0xBF: /*RES b, A*/
				case 0x80: case 0x88: case 0x90: case 0x98: case 0xA0: case 0xA8: case 0xB0: case 0xB8: /*RES b, B*/
				case 0x81: case 0x89: case 0x91: case 0x99: case 0xA1: case 0xA9: case 0xB1: case 0xB9: /*RES b, C*/
				case 0x82: case 0x8A: case 0x92: case 0x9A: case 0xA2: case 0xAA: case 0xB2: case 0xBA: /*RES b, D*/
				case 0x83: case 0x8B: case 0x93: case 0x9B: case 0xA3: case 0xAB: case 0xB3: case 0xBB: /*RES b, E*/
				case 0x84: case 0x8C: case 0x94: case 0x9C: case 0xA4: case 0xAC: case 0xB4: case 0xBC: /*RES b, H*/
				case 0x85: case 0x8D: case 0x95: case 0x9D: case 0xA5: case 0xAD: case 0xB5: case 0xBD: /*RES b, L*/
				{
					unsigned char bitNumber = (cbOpcode >> 3) & 0x07;
					unsigned char *paramReg = &regs[cbOpcode & 0x07];

					resetBit(paramReg, bitNumber);

					return 8;
				}

				/*RES b, (HL)*/
				case 0x86: case 0x8E: case 0x96: case 0x9E: case 0xA6: case 0xAE: case 0xB6: case 0xBE:
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

					if (paramReg == 0)
						setBit(&regs[FLAGS], Z_FLAG);


					return 8;
				}

				/*SLA (HL)*/
				case 0x26:
				{
					unsigned short src = regs[REG_H] << 8 | regs[REG_L];

					unsigned char val = readRam(src);

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], C_FLAG);

					if (val & 0x80)
						setBit(&regs[FLAGS], C_FLAG);

					val <<= 1;

					if (val == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					writeRam(src, val);

					return 16;
				}

				/*SRA r*/
				case 0x2F: case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D:
				{
					unsigned char &paramReg = regs[cbOpcode & 0x07];
					unsigned char tmp = paramReg;

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], C_FLAG);

					paramReg >>= 1;

					if (isBitSet(&tmp, 0))
						setBit(&regs[FLAGS], C_FLAG);

					if (isBitSet(&tmp, 7))
						setBit(&paramReg, 7);

					if (paramReg == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					return 8;
				}

				/*SRA (HL)*/
				case 0x2E:
				{
					unsigned short src = regs[REG_H] << 8 | regs[REG_L];
					unsigned char val = readRam(src);
					unsigned char tmp = val;

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], C_FLAG);

					val >>= 1;

					if (isBitSet(&tmp, 0))
						setBit(&regs[FLAGS], C_FLAG);

					if (isBitSet(&tmp, 7))
						setBit(&val, 7);

					if (val == 0)
						setBit(&regs[FLAGS], Z_FLAG);
					
					writeRam(src, val);


					return 16;
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

					if (paramReg == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					return 8;
				}

				/*SRL (HL)*/
				case 0x3E:
				{
					unsigned short src = regs[REG_H] << 8 | regs[REG_L];
					unsigned char val = readRam(src);
					unsigned char tmp = val;

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], C_FLAG);

					if (val & 0x1)
						setBit(&regs[FLAGS], C_FLAG);

					val >>= 1;

					if (val == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					writeRam(src, val);

					return 16;
				}
				/*BIT b, (HL)*/
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

					return 12;
				}

				/*BIT b, r*/
				case 0x47: case 0x4F: case 0x57: case 0x5F: case 0x67: case 0x6F: case 0x77: case 0x7F: /*BIT b, A*/
				case 0x40: case 0x48: case 0x50: case 0x58: case 0x60: case 0x68: case 0x70: case 0x78: /*BIT b, B*/
				case 0x41: case 0x49: case 0x51: case 0x59: case 0x61: case 0x69: case 0x71: case 0x79: /*BIT b, C*/
				case 0x42: case 0x4A: case 0x52: case 0x5A: case 0x62: case 0x6A: case 0x72: case 0x7A: /*BIT b, D*/
				case 0x43: case 0x4B: case 0x53: case 0x5B: case 0x63: case 0x6B: case 0x73: case 0x7B: /*BIT b, E*/
				case 0x44: case 0x4C: case 0x54: case 0x5C: case 0x64: case 0x6C: case 0x74: case 0x7C: /*BIT b, H*/
				case 0x45: case 0x4D: case 0x55: case 0x5D: case 0x65: case 0x6D: case 0x75: case 0x7D: /*BIT b, L*/
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

				/*SET b, r*/
				case 0xC7: case 0xCF: case 0xD7: case 0xDF: case 0xE7: case 0xEF: case 0xF7: case 0xFF: /*SET b, A*/
				case 0xC0: case 0xC8: case 0xD0: case 0xD8: case 0xE0: case 0xE8: case 0xF0: case 0xF8: /*SET b, B*/
				case 0xC1: case 0xC9: case 0xD1: case 0xD9: case 0xE1: case 0xE9: case 0xF1: case 0xF9: /*SET b, C*/
				case 0xC2: case 0xCA: case 0xD2: case 0xDA: case 0xE2: case 0xEA: case 0xF2: case 0xFA: /*SET b, D*/
				case 0xC3: case 0xCB: case 0xD3: case 0xDB: case 0xE3: case 0xEB: case 0xF3: case 0xFB: /*SET b, E*/
				case 0xC4: case 0xCC: case 0xD4: case 0xDC: case 0xE4: case 0xEC: case 0xF4: case 0xFC: /*SET b, H*/
				case 0xC5: case 0xCD: case 0xD5: case 0xDD: case 0xE5: case 0xED: case 0xF5: case 0xFD: /*SET b, L*/
				{
					unsigned char bitNumber = (cbOpcode >> 3) & 0x07;
					unsigned char *paramReg = &regs[cbOpcode & 0x07];

					setBit(paramReg, bitNumber);

					return 8;
				}

				/*SET b, (HL)*/
				case 0xC6: case 0xCE: case 0xD6: case 0xDE: case 0xE6: case 0xEE: case 0xF6: case 0xFE:
				{
					unsigned char bitNumber = (cbOpcode >> 3) & 0x07;
					unsigned short src = regs[REG_H] << 8 | regs[REG_L];
					unsigned char val = readRam(src);

					setBit(&val, bitNumber);
					writeRam(src, val);

					return 16;
				}



				/*RL r*/
				case 0x17: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
				{
					unsigned char &paramReg = regs[cbOpcode & 0x07];
					unsigned char tmp = paramReg;

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);

					paramReg <<= 1;

					//If C Flag is set, set bit 0 of paramReg as well
					if (isBitSet(&regs[FLAGS], C_FLAG))
					{
						setBit(&paramReg, 0);
					}

					//If the most left bit of tmp ist set, set the C Flag 
					if (isBitSet(&tmp, 7))
					{
						setBit(&regs[FLAGS], C_FLAG);
					}
					//If not, reset the C Flag
					else
					{
						resetBit(&regs[FLAGS], C_FLAG);

					}

					if (paramReg == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					return 8;
				}

				/*RL (HL)*/
				case 0x16:
				{
					unsigned short src = regs[REG_H] << 8 | regs[REG_L];
					unsigned char val = readRam(src);
					unsigned char tmp = val;

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);

					val <<= 1;

					//If C Flag is set, set bit 0 of paramReg as well
					if (isBitSet(&regs[FLAGS], C_FLAG))
					{
						setBit(&val, 0);
					}

					//If the most left bit of tmp ist set, set the C Flag 
					if (isBitSet(&tmp, 7))
					{
						setBit(&regs[FLAGS], C_FLAG);
					}
					//If not, reset the C Flag
					else
					{
						resetBit(&regs[FLAGS], C_FLAG);

					}

					if (val == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					writeRam(src, val);

					return 16;
				}
				/*RR r*/
				case 0x1F: case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D:
				{
					unsigned char &paramReg = regs[cbOpcode & 0x07];
					unsigned char tmp = paramReg;

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);

					paramReg >>= 1;

					//If C Flag is set, set bit 0 of paramReg as well
					if (isBitSet(&regs[FLAGS], C_FLAG))
					{
						setBit(&paramReg, 7);
					}

					//If the most left bit of tmp ist set, set the C Flag 
					if (isBitSet(&tmp, 0))
					{
						setBit(&regs[FLAGS], C_FLAG);
					}
					//If not, reset the C Flag
					else
					{
						resetBit(&regs[FLAGS], C_FLAG);

					}

					if (paramReg == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					return 8;
				}

				/*RR (HL)*/
				case 0x1E:
				{
					unsigned short src = regs[REG_H] << 8 | regs[REG_L];
					unsigned char val = readRam(src);
					unsigned char tmp = val;

					resetBit(&regs[FLAGS], Z_FLAG);
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);

					val >>= 1;

					//If C Flag is set, set bit 0 of paramReg as well
					if (isBitSet(&regs[FLAGS], C_FLAG))
					{
						setBit(&val, 7);
					}

					//If the most left bit of tmp ist set, set the C Flag 
					if (isBitSet(&tmp, 0))
					{
						setBit(&regs[FLAGS], C_FLAG);
					}
					//If not, reset the C Flag
					else
					{
						resetBit(&regs[FLAGS], C_FLAG);

					}

					if (val == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					writeRam(src, val);

					return 16;
				}

				/*RLC r*/
				case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x07:
				{
					unsigned char &paramReg = regs[cbOpcode & 0x07];
					unsigned char tmp = paramReg;
					
					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], Z_FLAG);


					paramReg <<= 1;

					if (isBitSet(&tmp, 7))
					{
						setBit(&paramReg, 0);
						setBit(&regs[FLAGS], C_FLAG);
					}
					else
						resetBit(&regs[FLAGS], C_FLAG);

					if(paramReg  == 0)
						setBit(&regs[FLAGS], Z_FLAG);


					return 8;
				}

				/*RLC (HL)*/
				case 0x06:
				{
					unsigned short regHL = regs[REG_H] << 8 | regs[REG_L];

					//unsigned short regHL = ((regs[REG_H] << 8) & 0xFF00) + (regs[REG_L] & 0xFF);
					unsigned char val = readRam(regHL);
					unsigned char tmp = val;

					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], Z_FLAG);


					val <<= 1;

					if (isBitSet(&tmp, 7))
					{
						setBit(&val, 0);
						setBit(&regs[FLAGS], C_FLAG);
					}
					else
						resetBit(&regs[FLAGS], C_FLAG);

					if (val == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					writeRam(regHL, val);

					return 16;
				}

				/*RRC r*/
				case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0F:
				{
					unsigned char &paramReg = regs[cbOpcode & 0x07];
					unsigned char tmp = paramReg;

					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], Z_FLAG);

					paramReg >>= 1;

					if (isBitSet(&tmp, 0))
					{
						setBit(&paramReg, 7);
						setBit(&regs[FLAGS], C_FLAG);
					}
					else
						resetBit(&regs[FLAGS], C_FLAG);

					if (paramReg == 0)
						setBit(&regs[FLAGS], Z_FLAG);


					return 8;
				}
				/*RRC (HL)*/
				case 0x0E:
				{
					unsigned short regHL = regs[REG_H] << 8 | regs[REG_L];

					//unsigned short regHL = ((regs[REG_H] << 8) & 0xFF00) + (regs[REG_L] & 0xFF);
					unsigned char val = readRam(regHL);
					unsigned char tmp = val;

					resetBit(&regs[FLAGS], N_FLAG);
					resetBit(&regs[FLAGS], H_FLAG);
					resetBit(&regs[FLAGS], Z_FLAG);

					val >>= 1;

					if (isBitSet(&tmp, 0))
					{
						setBit(&val, 7);
						setBit(&regs[FLAGS], C_FLAG);
					}
					else
						resetBit(&regs[FLAGS], C_FLAG);

					if (val == 0)
						setBit(&regs[FLAGS], Z_FLAG);

					writeRam(regHL, val);

					return 16;
				}
				default:
				{
					std::cout << "Opcode: 0xCB " << std::hex << +cbOpcode << " not yet implemented.";
					return 0;
				}
			}
		}






		/*DAA*/ case 0x27:
		{
			unsigned char &regA = regs[REG_A];
			signed char lowNibble = regA & 0x0F;

			resetBit(&regs[FLAGS], Z_FLAG);

			//	When this instruction is executed, the A register is BCD corrected using the contents of the flags.The exact process is the following : 
			//if the least significant four bits of A contain a non - BCD digit(i.e.it is greater than 9) or the H flag is set, then $06 is added to the register.
			//	Then the four most significant bits are checked.If this more significant digit also happens to be greater than 9 or the C flag is set, then $60 is added.

			//If N = 1, subtract the correction, else add it
			if (isBitSet(&regs[FLAGS], N_FLAG) == false)
			{
				if (regA > 0x99 || isBitSet(&regs[FLAGS], C_FLAG))
				{
					regA += 0x60;
					setBit(&regs[FLAGS], C_FLAG);
				}

				if (lowNibble > 9 || isBitSet(&regs[FLAGS], H_FLAG))
				{
					regA += 0x06;
				}
			}
			else
			{
				if (isBitSet(&regs[FLAGS], C_FLAG))
					regA -= 0x60;
				if (isBitSet(&regs[FLAGS], H_FLAG))
					regA -= 0x06;
			}

			if (regA == 0)
				setBit(&regs[FLAGS], Z_FLAG);

			resetBit(&regs[FLAGS], H_FLAG);

			return 4;
		}

		/*CPL*/ case 0x2F:
		{
			setBit(&regs[FLAGS],N_FLAG);
			setBit(&regs[FLAGS],H_FLAG);

			regs[REG_A] = ~regs[REG_A];

			return 4;
		}


		/*CCF*/ case 0x3F:
		{
			if(isBitSet(&regs[FLAGS], C_FLAG))
				resetBit(&regs[FLAGS], C_FLAG);
			else
				setBit(&regs[FLAGS], C_FLAG);

			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);

			return 4;
		}

		/*SCF*/ case 0x37:
		{
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			setBit(&regs[FLAGS], C_FLAG);
			return 4;
		}

		/*HALT*/ case 0x76:
		{
			haltFlag = true;
			
			return 4;
		}

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
			unsigned char tmp = regA;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);

			regA <<= 1;

			if (isBitSet(&tmp,7))
			{
				setBit(&regs[FLAGS], C_FLAG);
				setBit(&regA, 0);
			}

			//if(regA == 0)
			//	setBit(&regs[FLAGS], Z_FLAG);


			return 4;
		}


		/*RRCA*/ case 0x0F:
		{
			unsigned char &regA = regs[REG_A];
			unsigned char tmp = regA;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);
			resetBit(&regs[FLAGS], C_FLAG);

			regA >>= 1;
			
			if (isBitSet(&tmp, 0))
			{
				setBit(&regs[FLAGS], C_FLAG);
				setBit(&regA, 7);
			}
			
			return 4;
		}

		/*RLA*/ case 0x17:
		{
			unsigned char &regA = regs[REG_A];
			unsigned char tmp = regA;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);

			regA <<= 1;

			if (isBitSet(&regs[FLAGS], C_FLAG))
				setBit(&regA, 0);

			if (isBitSet(&tmp, 7))
				setBit(&regs[FLAGS], C_FLAG);
			else
				resetBit(&regs[FLAGS], C_FLAG);


			return 4;
		}

		/*RRA*/ case 0x1F:
		{
			unsigned char &regA = regs[REG_A];
			unsigned char tmp = regA;

			resetBit(&regs[FLAGS], Z_FLAG);
			resetBit(&regs[FLAGS], H_FLAG);
			resetBit(&regs[FLAGS], N_FLAG);

			regA >>= 1;

			if (isBitSet(&regs[FLAGS], C_FLAG))
				setBit(&regA, 7);

			if (isBitSet(&tmp, 0))
				setBit(&regs[FLAGS], C_FLAG);
			else
				resetBit(&regs[FLAGS], C_FLAG);


			return 4;
		}

		/*JP nn*/ case 0xC3:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));
			programCounter = newAddressToJumpTo;

			return 16;
		}

		/*JP NZ,nn*/ case 0xC2:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));

			if (!isBitSet(&regs[FLAGS], Z_FLAG))
			{
				programCounter = newAddressToJumpTo;
				return 16;
			}
			else
				programCounter += 2;

			return 12;
		}

		/*JP Z,nn*/ case 0xCA:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));

			if (isBitSet(&regs[FLAGS], Z_FLAG))
			{
				programCounter = newAddressToJumpTo;
				return 16;
			}
			else
				programCounter += 2;

			return 12;
		}

		/*JP NC,nn*/ case 0xD2:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));

			if (!isBitSet(&regs[FLAGS], C_FLAG))
			{
				programCounter = newAddressToJumpTo;
				return 16;
			}
			else
				programCounter += 2;

			return 12;
		}

		/*JP C,nn*/ case 0xDA:
		{
			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));

			if (isBitSet(&regs[FLAGS], C_FLAG))
			{
				programCounter = newAddressToJumpTo;
				return 16;
			}
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

			return 12;
		}

		/*JR NZ,**/ case 0x20:
		{
			if (isBitSet(&regs[FLAGS], Z_FLAG) == false)
			{
				programCounter += (signed char)readRam(programCounter);
				programCounter++;
				return 12;
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
				return 12;
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
				return 12;
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
				return 12;
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

			
			return 24;
		}

		/*CALL NZ,nn*/ case 0xC4:
		{		
			if (isBitSet(&regs[FLAGS], Z_FLAG) == false)
			{
				unsigned char nHighByte = readRam(programCounter);
				unsigned char nLowByte = readRam(programCounter + 1);
				pushToStack(programCounter + 2);
				//LSB first
				programCounter = nLowByte << 8 | nHighByte;
				return 24;
			}
			else
				programCounter += 2;

			return 12;
		}

		/*CALL Z,nn*/ case 0xCC:
		{
			if (isBitSet(&regs[FLAGS], Z_FLAG))
			{
				unsigned char nHighByte = readRam(programCounter);
				unsigned char nLowByte = readRam(programCounter + 1);
				pushToStack(programCounter + 2);
				//LSB first
				programCounter = nLowByte << 8 | nHighByte;
				return 24;
			}
			else
				programCounter += 2;

			return 12;
		}
		/*CALL NC,nn*/ case 0xD4:
		{
			if (isBitSet(&regs[FLAGS], C_FLAG) == false)
			{
				unsigned char nHighByte = readRam(programCounter);
				unsigned char nLowByte = readRam(programCounter + 1);
				pushToStack(programCounter + 2);
				//LSB first
				programCounter = nLowByte << 8 | nHighByte;
				return 24;
			}
			else
				programCounter += 2;

			return 12;
		}
		/*CALL C,nn*/ case 0xDC:
		{
			if (isBitSet(&regs[FLAGS], C_FLAG))
			{
				unsigned char nHighByte = readRam(programCounter);
				unsigned char nLowByte = readRam(programCounter + 1);
				pushToStack(programCounter + 2);
				//LSB first
				programCounter = nLowByte << 8 | nHighByte;
				return 24;
			}
			else
				programCounter += 2;

			return 12;
		}

		/*RST 00H, RST 08H
		  RST 10H, RST 18H
          RST 20H, RST 28H
          RST 30H, RST 38H*/
		case 0xC7: case 0xCF: case 0xD7: case 0xDF: case 0xE7: case 0xEF: case 0xF7: case 0xFF:
		{
			pushToStack(programCounter);
			programCounter = opcode - 0xC7;
			return 16;
		}

		/*RET -/-*/ case 0xC9:
		{

			programCounter = popFromStack(); 
			return 16;
		}

		/*RET NZ*/ case 0xC0:
		{
			if (isBitSet(&regs[FLAGS], Z_FLAG) == false)
			{
				programCounter = popFromStack();
				return 20;
			}
			
			return 8;
		}
		/*RET Z*/ case 0xC8:
		{

			if (isBitSet(&regs[FLAGS], Z_FLAG))
			{
				programCounter = popFromStack();
				return 20;
			}

			return 8;
		}
		/*RET NC */case 0xD0:
		{
			if (isBitSet(&regs[FLAGS], C_FLAG) == false)
			{
				programCounter = popFromStack();
				return 20;
			}

			return 8;
		}
		/*RET C*/ case 0xD8:
		{
			if (isBitSet(&regs[FLAGS], C_FLAG))
			{
				programCounter = popFromStack();
				return 20;
			}

			return 8;
		}

		/*RETI -/-*/ case 0xD9:
		{
			programCounter = popFromStack();
			imeFlag = 1;
			return 16;
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
		fread(cartridgeContent, 1, 0x200000, fp);
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

void resolveStop()
{
	if (stopFlag)
	{
		stopFlag = false;
		setBit(&ram[LCDC_CTRL], 7);
	}
}