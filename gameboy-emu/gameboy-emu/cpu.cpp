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
unsigned char screenData[160][144][3];

const unsigned char REG_A = 7;
const unsigned char REG_B = 0;
const unsigned char REG_C = 1;
const unsigned char REG_D = 2;
const unsigned char REG_E = 3;
const unsigned char REG_H = 4;
const unsigned char REG_L = 5;
const unsigned char FLAGS = 6;

struct reg
{
	unsigned char A;
	unsigned char B;
	unsigned char C;
	unsigned char D;
	unsigned char E;
	unsigned char F;
	unsigned char H;
	unsigned char L;
} registers;


//Order is: B, C, D, E, H, L, F, A
//B: 000, C: 001, D: 010,
//E: 011, H: 100, L: 101, F: 110, A: 111
unsigned char regs[8];

unsigned char ram[65536];
int countCycles;

void initialize()
{
	freopen("output.txt", "w", stdout);

	//prevent automatic flush of cout after every "\n", speeds up extremely
	std::setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);

	countCycles = 0;
	cyclesScanLine = CYCLES_PER_SCAN_LINE;
	programCounter = 0x100;
	stackPointer = 0xFFFE; // from $FFFE - $FF80, grows downward

	registers.A = 0x01;
	registers.F = 0xB0;
	registers.B = 0x00;
	registers.C = 0x13;
	registers.D = 0x00;
	registers.E = 0xD8;
	registers.H = 0x01;
	registers.L = 0x4D;

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

}

unsigned char readRam(unsigned short address)
{
	 if (address == 0xFF00)
	 {
		 return getJoypadState();
	 }
	 
	return ram[address] & 0xFF;
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
		if (imeFlag == 1)
		{
			if (irRequestFlagStatus & VERTICAL_BLANKING == VERTICAL_BLANKING)
			{
				if(irEnableFlagStatus & VERTICAL_BLANKING == VERTICAL_BLANKING)
					serviceInterrupt(VERTICAL_BLANKING);
			}
			else if (irRequestFlagStatus & LCDC == LCDC)
			{
				if (irEnableFlagStatus & LCDC == LCDC)
					serviceInterrupt(LCDC);
			}
			else if (irRequestFlagStatus & TIMER_OVERFLOW == TIMER_OVERFLOW)
			{
				if (irEnableFlagStatus & TIMER_OVERFLOW == TIMER_OVERFLOW)
					serviceInterrupt(TIMER_OVERFLOW);
			}
			else if (irRequestFlagStatus & SERIAL_IO_COMPLETION == SERIAL_IO_COMPLETION)
			{
				if(irEnableFlagStatus & SERIAL_IO_COMPLETION == SERIAL_IO_COMPLETION)
					serviceInterrupt(SERIAL_IO_COMPLETION);
			}
			else if (irRequestFlagStatus & JOYPAD == JOYPAD)
			{
				if(irEnableFlagStatus & JOYPAD == JOYPAD)
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
	std::cout << "Opcode: " << std::uppercase << std::hex << (opcode < 0x10 ? "0x0" : "0x") << (int)opcode << "\n";
	std::cout << "af: 0x" << std::uppercase << std::hex << +registers.A << +registers.F << "\n";
	std::cout << "bc: 0x" << std::uppercase << std::hex << +registers.B << +registers.C << "\n";
	std::cout << "de: 0x" << std::uppercase << std::hex << +registers.D << +registers.E << "\n";
	std::cout << "hl: 0x" << std::uppercase << std::hex << +registers.H << +registers.L << "\n";
	std::cout << "sp: 0x" << std::uppercase << std::hex << +stackPointer << "\n";
	std::cout << "pc: 0x" << std::uppercase << std::hex << +programCounter << "\n\n";

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


		///*LD A,(HL)*/ case 0x7E:
		//{
		//	herewasabreak;
		//}


		///*LD B,(HL)*/ case 0x46:
		//{
		//	herewasabreak;
		//}


		///*LD C,(HL)*/ case 0x4E:
		//{
		//	herewasabreak;
		//}

		
		///*LD D,(HL)*/ case 0x56:
		//{
		//	herewasabreak;
		//}		

		///*LD E,(HL)*/ case 0x5E:
		//{
		//	herewasabreak;
		//}

		///*LD H,(HL)*/ case 0x66:

		///*LD L,(HL)*/ case 0x6E:
		//{
		//	herewasabreak;
		//}


		///*LD (HL),B*/ case 0x70:
		//{
		//	herewasabreak;
		//}
		///*LD (HL),C*/ case 0x71:
		//{
		//	herewasabreak;
		//}
		///*LD (HL),D*/ case 0x72:
		//{
		//	herewasabreak;
		//}
		///*LD (HL),E*/ case 0x73:
		//{
		//	herewasabreak;
		//}
		///*LD (HL),H*/ case 0x74:
		//{
		//	herewasabreak;
		//}
		///*LD (HL),L*/ case 0x75:
		//{
		//	herewasabreak;
		//}
		/*LD (HL),n*/ case 0x36:
		{
			unsigned char n = readRam(programCounter);
			unsigned short dstAddress = registers.H << 8 | registers.L;
			writeRam(dstAddress, n);
			programCounter++;
			return 12;
		}

		///*LD A,(BC)*/ case 0x0A:
		//{
		//	herewasabreak;
		//}
		///*LD A,(DE)*/ case 0x1A:
		//{
		//	herewasabreak;
		//}

		/*LD A,(nn)*/ case 0xFA:
		{
			unsigned char nHighByte = readRam(programCounter);
			unsigned char nlowByte = readRam(programCounter + 1);
			//LSB byte first
			unsigned short srcAddress =  nlowByte << 8 | nHighByte;

			registers.A = readRam(srcAddress);

			programCounter += 2;

			return 16;
		}






	

		///*LD (BC),A*/ case 0x02:
		//{
		//	herewasabreak;
		//}
		///*LD (DE),A*/ case 0x12:
		//{
		//	herewasabreak;
		//}
		///*LD (HL),A*/ case 0x77:
		//{
		//	herewasabreak;
		//}
		/*LD (nn),A*/ case 0xEA:
		{
			unsigned char nlowByte = readRam(programCounter);
			unsigned char nHighByte = readRam(programCounter+1);
			unsigned short nn = nHighByte << 8 | nlowByte;

			writeRam(nn, registers.A);

			programCounter += 2;

			return 16;
		}


		///*LD A, (C)*/ case 0xF2:
		//{
		//	herewasabreak;
		//}

		/*LD (C), A*/ case 0xE2:
		{
			unsigned short dstAddress = 0xFF00 + registers.C;
			writeRam(dstAddress, registers.A);
			return 8;
		}

		///*LD A,(HLD)*/
		///*LD A,(HL-)*/
		///*LDD A,(HL)*/ case 0x3A:
		//{
		//	herewasabreak;
		//}

		/*LD (HLD),A */
		/*LD (HL-),A */
		/*LDD (HL),A*/ case 0x32:
		{
			//int regHL = ((registers.H << 8) & 0xFF00) + (registers.L & 0xFF);
			short int regHL = ((registers.H << 8) & 0xFF00) + (registers.L & 0xFF);
			writeRam(regHL, registers.A);
			//ram[regHL]= registers.A;
			regHL--;
			registers.H = (regHL >> 8) & 0xFF;
			registers.L = regHL & 0xFF;
			return 8;
		}

		/*LD A,(HLI) */
		/*LD A,(HL+) */
		/*LDI A,(HL)*/
		case 0x2A:
		{		
			
			unsigned short srcAddress = registers.H << 8 | registers.L;
			unsigned char value = readRam(srcAddress);
			registers.A = value;
			srcAddress++;
			registers.H = (srcAddress & 0xFF00) >> 8;
			registers.L = srcAddress & 0x00FF;
			return 8;
		}
//
//		/*LD (HLI),A */
///*LD (HL+),A */
///*LDI (HL),A*/
//		case 0x22:
//		{
//			herewasabreak;
//		}
//
		/*LDH (n),A*/
		/*LD ($FF00+n),A*/ case 0xE0:
		{
			//int n = ram[programCounter] & 0xFF;
			unsigned char n = readRam(programCounter);
			writeRam(0xFF00 + n, registers.A);
			//ram[0xFF00 + n] = registers.A;
			programCounter++;
			return 12;
		}

		/*LDH A,(n)*/
		/*LD A,($FF00+n)*/ case 0xF0:
		{
			//int n = ram[programCounter] & 0xFF;
			unsigned char n = readRam(programCounter);
			//registers.A = ram[0xFF00 + n] & 0xFF;
			registers.A = readRam(0xFF00+n);
			///registers.A = 0x3E;
			programCounter++;
			return 12;
		}

		///*LD n,nn*/
		/*LD BC,nn*/ case 0x01: 
		{
			registers.C = readRam(programCounter);
			registers.B = readRam(++programCounter);
			programCounter++;
			return 12;
		}
		/*LD DE,nn*/ case 0x11: 
		{
			registers.D = readRam(programCounter);
			registers.E = readRam(++programCounter);
			std::cout.flush();
			programCounter++;
			return 12;
		}
		/*LD HL,nn*/ case 0x21: 
		{
			//registers.L = ram[programCounter];
			//registers.H = ram[++programCounter];
			registers.L = readRam(programCounter);
			registers.H = readRam(++programCounter);
			programCounter++;
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


		///*LD SP, HL*/ case 0xF9: {
		//	herewasabreak;

		//}

		///*LDHL SP, n*/ case 0xF8: {
		//	herewasabreak;

		//}


		///*LD (nn),SP*/ case 0x08: {
		//	herewasabreak;

		//}


		/*PUSH AF*/ case 0xF5: {
			unsigned short pushVal = registers.A << 8 | registers.F;
			pushToStack(pushVal);
			return 16;

		}


		/*PUSH BC*/ case 0xC5: {
			unsigned short pushVal = registers.B << 8 | registers.C;
			pushToStack(pushVal);
			return 16;

		}


		/*PUSH DE*/ case 0xD5: {
			unsigned short pushVal = registers.D << 8 | registers.E;
			pushToStack(pushVal);
			return 16;

		}


		/*PUSH HL*/ case 0xE5: {
			unsigned short pushVal = registers.H << 8 | registers.L;
			pushToStack(pushVal);
			return 16;

		}



		/*POP AF*/ case 0xF1: 
		{
			unsigned short popVal = popFromStack();
			registers.A = (popVal & 0xFF00) >> 8;
			registers.F = (popVal & 0x00FF);

			return 12;

		}


		/*POP BC*/ case 0xC1: 
		{
			unsigned short popVal = popFromStack();
			registers.B = (popVal & 0xFF00) >> 8;
			registers.C = (popVal & 0x00FF);

			return 12;

		}


		/*POP DE*/ case 0xD1: 
		{
			unsigned short popVal = popFromStack();
			registers.D = (popVal & 0xFF00) >> 8;
			registers.E = (popVal & 0x00FF);

			return 12;

		}


		/*POP HL*/ case 0xE1: 
		{
			unsigned short popVal = popFromStack();
			registers.H = (popVal & 0xFF00) >> 8;
			registers.L = (popVal & 0x00FF);

			return 12;

		}


		/*ADD A,A*/ case 0x87:
		{
			resetBit(&registers.F, Z_FLAG);
			resetBit(&registers.F, C_FLAG);
			resetBit(&registers.F, H_FLAG);
			
			if ((int)(registers.A + registers.A) > 255)
				setBit(&registers.F, C_FLAG);

			if (((registers.A & 0x0F) + (registers.A & 0x0F)) > 0x0F)
				setBit(&registers.F, H_FLAG);

			registers.A += registers.A;
			
			if (registers.A == 0)
				setBit(&registers.F, Z_FLAG);



			resetBit(&registers.F, N_FLAG);

			return 4;
		}
		///*ADD A,B*/ case 0x80:
		//{
		//	herewasabreak;
		//}
		///*ADD A,C*/ case 0x81:
		//{
		//	herewasabreak;
		//}
		///*ADD A,D*/ case 0x82:
		//{
		//	herewasabreak;
		//}
		///*ADD A,E*/ case 0x83:
		//{
		//	herewasabreak;
		//}
		///*ADD A,H*/ case 0x84:
		//{
		//	herewasabreak;
		//}
		///*ADD A,L*/ case 0x85:
		//{
		//	herewasabreak;
		//}
		///*ADD A,(HL)*/ case 0x86:
		//{
		//	herewasabreak;
		//}
		///*ADD A,#*/ case 0xC6:
		//{
		//	herewasabreak;
		//}



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



		/*AND A*/ case 0xA7:
		{	
			resetBit(&registers.F, Z_FLAG);
			registers.A &= registers.A;
			if (registers.A == 0)
				setBit(&registers.F, Z_FLAG);
			resetBit(&registers.F, N_FLAG);
			setBit(&registers.F, H_FLAG);
			resetBit(&registers.F, C_FLAG);


			return 4;
		}
		///*AND B*/ case 0xA0:
		//{
		//	herewasabreak;
		//}
		/*AND C*/ case 0xA1:
		{
			resetBit(&registers.F, Z_FLAG);
			registers.A &= registers.C;
			if (registers.A == 0)
				setBit(&registers.F, Z_FLAG);
			resetBit(&registers.F, N_FLAG);
			setBit(&registers.F, H_FLAG);
			resetBit(&registers.F, C_FLAG);

			return 4;
		}
		///*AND D*/ case 0xA2:
		//{
		//	herewasabreak;
		//}
		///*AND E*/ case 0xA3:
		//{
		//	herewasabreak;
		//}
		///*AND H*/ case 0xA4:
		//{
		//	herewasabreak;
		//}
		///*AND L*/ case 0xA5:
		//{
		//	herewasabreak;
		//}
		///*AND (HL)*/ case 0xA6:
		//{
		//	herewasabreak;
		//}
		/*AND #*/ case 0xE6:
		{
			unsigned char n = readRam(programCounter);
			registers.A &= n;
			programCounter++;
			resetBit(&registers.F,Z_FLAG);
			if (registers.A == 0)
				setBit(&registers.F,Z_FLAG);
			resetBit(&registers.F,N_FLAG);
			setBit(&registers.F,H_FLAG);
			resetBit(&registers.F,C_FLAG);


			return 8;
		}



		///*OR A*/ case 0xB7:
		//{
		//	herewasabreak;
		//}
		/*OR B*/ case 0xB0:
		{
			registers.A |= registers.B;
			resetBit(&registers.F, Z_FLAG);
			if (registers.A == 0)
				setBit(&registers.F, Z_FLAG);

			resetBit(&registers.F, N_FLAG);
			resetBit(&registers.F, H_FLAG);
			resetBit(&registers.F, C_FLAG);

			return 4;
		}
		/*OR C*/ case 0xB1:
		{
			registers.A |= registers.C;
			resetBit(&registers.F,Z_FLAG);
			if (registers.A == 0)
				setBit(&registers.F,Z_FLAG);

			resetBit(&registers.F,N_FLAG);
			resetBit(&registers.F,H_FLAG);
			resetBit(&registers.F,C_FLAG);

			return 4;
		}
		///*OR D*/ case 0xB2:
		//{
		//	herewasabreak;
		//}
		///*OR E*/ case 0xB3:
		//{
		//	herewasabreak;
		//}
		///*OR H*/ case 0xB4:
		//{
		//	herewasabreak;
		//}
		///*OR L*/ case 0xB5:
		//{
		//	herewasabreak;
		//}
		///*OR (HL)*/ case 0xB6:
		//{
		//	herewasabreak;
		//}
		///*OR #*/ case 0xF6:
		//{
		//	herewasabreak;
		//}



		/*XOR A*/ case 0xAF:
		{
			registers.A ^= registers.A;
			if (registers.A == 0x00)
				setBit(&registers.F, Z_FLAG);

			resetBit(&registers.F, N_FLAG);
			resetBit(&registers.F, C_FLAG);
			resetBit(&registers.F, H_FLAG);

			return 4;
		}
		///*XOR B*/ case 0xA8:
		//{
		//	herewasabreak;
		//}
		/*XOR C*/ case 0xA9:
		{
			registers.A ^= registers.C;
			if (registers.A == 0x00)
				setBit(&registers.F, Z_FLAG);

			resetBit(&registers.F, N_FLAG);
			resetBit(&registers.F, C_FLAG);
			resetBit(&registers.F, H_FLAG);

			return 4;
		}
		///*XOR D*/ case 0xAA:
		//{
		//	herewasabreak;
		//}
		///*XOR E*/ case 0xAB:
		//{
		//	herewasabreak;
		//}
		///*XOR H*/ case 0xAC:
		//{
		//	herewasabreak;
		//}
		///*XOR L */case 0xAD:
		//{
		//	herewasabreak;
		//}
		///*XOR (HL)*/ case 0xAE:
		//{
		//	herewasabreak;
		//}
		///*XOR * */ case 0xEE:
		//{
		//	herewasabreak;
		//}



		///*CP A*/ case 0xBF:
		//{
		//	herewasabreak;
		//}
		///*CP B*/ case 0xB8:
		//{
		//	herewasabreak;
		//}
		///*CP C*/ case 0xB9:
		//{
		//	herewasabreak;
		//}
		///*CP D*/ case 0xBA:
		//{
		//	herewasabreak;
		//}
		///*CP E*/ case 0xBB:
		//{
		//	herewasabreak;
		//}
		///*CP H*/ case 0xBC:
		//{
		//	herewasabreak;
		//}
		///*CP L*/ case 0xBD:
		//{
		//	herewasabreak;
		//}
		///*CP (HL)*/ case 0xBE:
		//{
		//	herewasabreak;
		//}
		/*CP #*/ case 0xFE:
		{
			//unsigned char res = registers.A - ram[programCounter];
			unsigned char n = readRam(programCounter);
			unsigned char res = registers.A - n;
			if (res == 0)
			{
				setBit(&registers.F,Z_FLAG);
			}
			else
			{
				resetBit(&registers.F,Z_FLAG);
			}
			
			setBit(&registers.F,N_FLAG);
			if ((((registers.A) ^ n ^ res) & 0x10) >> 4)
				setBit(&registers.F, H_FLAG);
			else
				resetBit(&registers.F, H_FLAG);

			if (registers.A < n)
				setBit(&registers.F, C_FLAG);
			else
				resetBit(&registers.F, C_FLAG);

			programCounter++;

			return 8;
		}


		/*INC A*/ case 0x3C:
		{
			resetBit(&registers.F,Z_FLAG);
			resetBit(&registers.F,N_FLAG);

			if ((registers.A & 0x0F) == 0x0F)
				resetBit(&registers.F,H_FLAG);

			registers.A++;

			if (registers.A == 0)
				setBit(&registers.F,Z_FLAG);

			
			
			return 4;


		}
		///*INC B*/ case 0x04:
		//{
		//	herewasabreak;
		//}
		/*INC C*/ case 0x0C:
		{
			resetBit(&registers.F,Z_FLAG);
			if (registers.C == 0x0F)
				resetBit(&registers.F,H_FLAG);
			else if (registers.C == 0)
				setBit(&registers.F,Z_FLAG);
			registers.C++;

			resetBit(&registers.F,N_FLAG);


			return 4;
		}
		///*INC D*/ case 0x14:
		//{
		//	herewasabreak;
		//}
		///*INC E*/ case 0x1C:
		//{
		//	herewasabreak;
		//}
		///*INC H*/ case 0x24:
		//{
		//	herewasabreak;
		//}
		///*INC L*/ case 0x2C:
		//{
		//	herewasabreak;
		//}
		/*INC (HL)*/ case 0x34:
		{
			resetBit(&registers.F,Z_FLAG);
			unsigned short address = registers.H << 8 | registers.L;
			unsigned char val = readRam(address);
			val++;
			if (val == 0)
				setBit(&registers.F,Z_FLAG);
			resetBit(&registers.F,N_FLAG);
			if ((val & 0x0F) == 0x0F)
				setBit(&registers.F,H_FLAG);
			else
				resetBit(&registers.F,H_FLAG);
			writeRam(address, val);
			return 12;
		}



		/*DEC A*/ case 0x3D:
		{
			unsigned char res = registers.A - 1;
			unsigned char tmp = registers.A;


			if (tmp - 1 == 0x0)
				setBit(&registers.F,Z_FLAG);
			else resetBit(&registers.F,Z_FLAG);

			if ((((registers.A) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&registers.F, H_FLAG);
			else resetBit(&registers.F, H_FLAG);
			tmp--;
			registers.A = tmp;
			setBit(&registers.F,N_FLAG);

			return 4;
		}
		/*DEC B*/ case 0x05:
		{
			unsigned char res = registers.B - 1;
			unsigned char tmp = registers.B;


			if (tmp - 1 == 0x0)
				setBit(&registers.F,Z_FLAG);
			else resetBit(&registers.F,Z_FLAG);
			
			if((((registers.B) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&registers.F, H_FLAG);
			else resetBit(&registers.F, H_FLAG);

			tmp--;
			registers.B = tmp;

			setBit(&registers.F,N_FLAG);



			return 4;
		}
		/*DEC C*/ case 0x0D:
		{
			unsigned char res = registers.C - 1;
			unsigned char tmp = registers.C;


			if (tmp - 1 == 0x0)
				setBit(&registers.F,Z_FLAG);
			else resetBit(&registers.F,Z_FLAG);

			if((((registers.C) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&registers.F, H_FLAG);
			else resetBit(&registers.F, H_FLAG);

			tmp--;
			registers.C = tmp;
			setBit(&registers.F,N_FLAG);

			return 4;
		}
		/*DEC D*/ case 0x15:
		{
			unsigned char res = registers.D - 1;
			unsigned char tmp = registers.D;


			if (tmp - 1 == 0x0)
				setBit(&registers.F,Z_FLAG);
			else resetBit(&registers.F,Z_FLAG);

			if( (((registers.D) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&registers.F, H_FLAG);
			else resetBit(&registers.F, H_FLAG);
			tmp--;
			registers.D = tmp;
			setBit(&registers.F,N_FLAG);

			
			return 4;
		}
		/*DEC E*/ case 0x1D:
		{
			unsigned char res = registers.E - 1;
			unsigned char tmp = registers.E;


			if (tmp - 1 == 0x0)
				setBit(&registers.F,Z_FLAG);
			else resetBit(&registers.F,Z_FLAG);

			if((((registers.E) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&registers.F, H_FLAG);
			else resetBit(&registers.F, H_FLAG);
			tmp--;
			registers.E = tmp;
			setBit(&registers.F,N_FLAG);

			
			return 4;
		}
		/*DEC H*/ case 0x25:
		{
			unsigned char res = registers.H - 1;
			unsigned char tmp = registers.H;


			if (tmp - 1 == 0x0)
				setBit(&registers.F,Z_FLAG);
			else resetBit(&registers.F,Z_FLAG);

			if((((registers.H) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&registers.F, H_FLAG);
			else resetBit(&registers.F, H_FLAG);

			tmp--;
			registers.H = tmp;
			setBit(&registers.F,N_FLAG);

			
			return 4;
		}
		/*DEC L*/ case 0x2D:
		{
			unsigned char res = registers.H - 1;
			unsigned char tmp = registers.H;


			if (tmp - 1 == 0x0)
				setBit(&registers.F,Z_FLAG);
			else resetBit(&registers.F,Z_FLAG);

			if( (((registers.H) ^ (1) ^ res) & 0x10) >> 4)
				setBit(&registers.F, H_FLAG);
			else resetBit(&registers.F, H_FLAG);
			tmp--;
			registers.H = tmp;
			setBit(&registers.F,N_FLAG);

			
			return 4;
		}
		///*DEC (HL)*/ case 0x35:
		//{
		//	//unsigned char res = registers.C - 1;
		//	//unsigned char tmp = registers.C;



		//	//if (tmp - 1 == 0x0)
		//	//	setBit(&registers.F,Z_FLAG);
		//	//else resetBit(&registers.F,Z_FLAG);

		//	//flags.H = (((registers.C) ^ (1) ^ res) & 0x10) >> 4;
		//	//tmp--;
		//	//registers.C = tmp;
		//	//setBit(&registers.F,N_FLAG);

		//	
		//	herewasabreak;
		//}



		///*ADD HL,BC*/ case 0x09:
		//{
		//	herewasabreak;
		//}
		/*ADD HL,DE*/ case 0x19:
		{
			unsigned short de = registers.D << 8 | registers.E;
			unsigned short hl = registers.H << 8 | registers.L;
			unsigned short result = 0x0000;

			resetBit(&registers.F, N_FLAG);
			resetBit(&registers.F, H_FLAG);
			resetBit(&registers.F, C_FLAG);

			if ((int)(de + hl) > 0xFFF)
				setBit(&registers.F, H_FLAG);

			if ((int)(de + hl) > 0xFFFF)
				setBit(&registers.F, C_FLAG);

			
			result = de + hl;

			registers.H = (result & 0xFF00) >> 8;
			registers.L = result & 0x00FF;

			return 8;
		}
		///*ADD HL,HL*/ case 0x29:
		//{
		//	herewasabreak;
		//}
		///*ADD HL,SP*/ case 0x39:
		//{
		//	herewasabreak;
		//}


		///*ADD SP, #*/ case 0xE8:
		//{
		//	herewasabreak;
		//}



		///*INC BC*/ case 0x03:
		//{
		//	herewasabreak;
		//}
		///*INC DE*/ case 0x13:
		//{
		//	herewasabreak;
		//}
		///*INC HL*/ case 0x23:
		//{
		//	herewasabreak;
		//}
		///*INC SP*/ case 0x33:
		//{
		//	herewasabreak;
		//}



		/*DEC BC*/ case 0x0B:
		{
			unsigned short nn = registers.B << 8 | registers.C;
			nn--;
			registers.B = (nn & 0xFF00 )>> 8;
			registers.C = nn & 0x00FF;
			return 8;
		}
		///*DEC DE*/ case 0x1B:
		//{
		//	herewasabreak;
		//}
		///*DEC HL*/ case 0x2B:
		//{
		//	herewasabreak;
		//}
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
				/*SWAP A*/
				case 0x37:
				{
					unsigned char lowNibble = registers.A & 0x0F;
					unsigned char highNibble = registers.A & 0xF0;

					registers.A = lowNibble << 4 | highNibble >> 4;

					resetBit(&registers.F, Z_FLAG);
					if (registers.A == 0)
						setBit(&registers.F, Z_FLAG);

					resetBit(&registers.F, N_FLAG);
					resetBit(&registers.F, H_FLAG);
					resetBit(&registers.F, C_FLAG);

					return 8;
				}
			}
		}
		///*SWAP A*/ case 0xCB37:
		//{
		//	herewasabreak;
		//}
		///*SWAP B*/ case 0xCB30:
		//{
		//	herewasabreak;
		//}
		///*SWAP C*/ case 0xCB31:
		//{
		//	herewasabreak;
		//}
		///*SWAP D*/ case 0xCB32:
		//{
		//	herewasabreak;
		//}
		///*SWAP E*/ case 0xCB33:
		//{
		//	herewasabreak;
		//}
		///*SWAP H*/ case 0xCB34:
		//{
		//	herewasabreak;
		//}
		///*SWAP L*/ case 0xCB35:
		//{
		//	herewasabreak;
		//}
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
			registers.A = ~registers.A;
			setBit(&registers.F,N_FLAG);
			setBit(&registers.F,H_FLAG);
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

		///*HALT*/ case 0x76:
		//{
		//	herewasabreak;
		//}

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

		///*RLCA*/ case 0x07:
		//{
		//	herewasabreak;
		//}

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

		///*SLA A*/ case 0xCB27:
		//{
		//	herewasabreak;
		//}
		///*SLA B*/ case 0xCB20:
		//{
		//	herewasabreak;
		//}
		///*SLA C*/ case 0xCB21:
		//{
		//	herewasabreak;
		//}
		///*SLA D*/ case 0xCB22:
		//{
		//	herewasabreak;
		//}
		///*SLA E*/ case 0xCB23:
		//{
		//	herewasabreak;
		//}
		///*SLA H*/ case 0xCB24:
		//{
		//	herewasabreak;
		//}
		///*SLA L*/ case 0xCB25:
		//{
		//	herewasabreak;
		//}
		///*SLA (HL)*/ case 0xCB26:
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

		///*SRL A*/ case 0xCB3F:
		//{
		//	herewasabreak;
		//}
		///*SRL B*/ case 0xCB38:
		//{
		//	herewasabreak;
		//}
		///*SRL C*/ case 0xCB39:
		//{
		//	herewasabreak;
		//}
		///*SRL D*/ case 0xCB3A:
		//{
		//	herewasabreak;
		//}
		///*SRL E*/ case 0xCB3B:
		//{
		//	herewasabreak;
		//}
		///*SRL H*/ case 0xCB3C:
		//{
		//	herewasabreak;
		//}
		///*SRL L*/ case 0xCB3D:
		//{
		//	herewasabreak;
		//}
		///*SRL (HL)*/ case 0xCB3E:
		//{
		//	herewasabreak;
		//}


		///*BIT b,A*/ case 0xCB47:
		//{
		//	herewasabreak;
		//}
		///*BIT b,B*/ case 0xCB40:
		//{
		//	herewasabreak;
		//}
		///*BIT b,C*/ case 0xCB41:
		//{
		//	herewasabreak;
		//}
		///*BIT b,D*/ case 0xCB42:
		//{
		//	herewasabreak;
		//}
		///*BIT b,E*/ case 0xCB43:
		//{
		//	herewasabreak;
		//}
		///*BIT b,H*/ case 0xCB44:
		//{
		//	herewasabreak;
		//}
		///*BIT b,L*/ case 0xCB45:
		//{
		//	herewasabreak;
		//}
		///*BIT b,(HL)*/ case 0xCB46:
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

		///*RES b,A*/ case 0xCB87:
		//{
		//	herewasabreak;
		//}
		///*RES b,B*/ case 0xCB80:
		//{
		//	herewasabreak;
		//}
		///*RES b,C*/ case 0xCB81:
		//{
		//	herewasabreak;
		//}
		///*RES b,D*/ case 0xCB82:
		//{
		//	herewasabreak;
		//}
		///*RES b,E*/ case 0xCB83:
		//{
		//	herewasabreak;
		//}
		///*RES b,H*/ case 0xCB84:
		//{
		//	herewasabreak;
		//}
		///*RES b,L*/ case 0xCB85:
		//{
		//	herewasabreak;
		//}
		///*RES b,(HL)*/ case 0xCB86:
		//{
		//	herewasabreak;
		//}


		/*JP nn*/ case 0xC3:
		{
			//int newAddressToJumpTo = ((ram[programCounter + 1] << 8) + (ram[programCounter] & 0xFF));

			int newAddressToJumpTo = ((readRam(programCounter + 1) << 8) + readRam(programCounter));
			programCounter = newAddressToJumpTo;
			return 12;
		}

		///*JP NZ,nn*/ case 0xC2:
		//{
		//	herewasabreak;
		//}
		///*JP Z,nn*/ case 0xCA:
		//{
		//	herewasabreak;
		//}
		///*JP NC,nn*/ case 0xD2:
		//{
		//	herewasabreak;
		//}
		///*JP C,nn*/ case 0xDA:
		//{
		//	herewasabreak;
		//}

		///*JP (HL)*/ case 0xE9:
		//{
		//	herewasabreak;
		//}
		///*JR n*/ case 0x18:
		//{
		//	herewasabreak;
		//}

		/*JR NZ,**/ case 0x20:
		{
			if (isBitSet(&registers.F, Z_FLAG) == false)
			{
				//programCounter += (signed)ram[programCounter];
				programCounter += (signed char)readRam(programCounter);
				programCounter++;
			}
			else programCounter++;
			return 8;
		}
		/*JR Z,* */case 0x28:
		{
			if (isBitSet(&registers.F, Z_FLAG) == true)
			{
				//programCounter += (signed)ram[programCounter];
				programCounter += (signed char)readRam(programCounter);
				programCounter++;
			}
			else programCounter++;

			return 8; 
		}
		///*JR NC,**/ case 0x30:
		//{
		//	herewasabreak;
		//}
		///*JR C,* */case 0x38:
		//{
		//	herewasabreak;
		//}

		/*call nn*/ case 0xCD:
		{
			//push address of NEXT instruction to stack. since it's a 16 bit parameter (nn) we do +2 instead of +1
			pushToStack(programCounter+2);
			unsigned char nHighByte = readRam(programCounter);
			unsigned char nLowByte = readRam(programCounter + 1);

			//LSB first
			programCounter = nLowByte << 8 | nHighByte;
			//Increment, because the subroutine is placed after the nn
			//programCounter++;

			
			return 12;
		}

		///*CALL NZ,nn*/ case 0xC4:
		//{
		//	herewasabreak;
		//}
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
			pushToStack(programCounter - 1);
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
			if (isBitSet(&registers.F, Z_FLAG) == false)
				programCounter = popFromStack();
			
			return 8;
		}
		/*RET Z*/ case 0xC8:
		{

			if (isBitSet(&registers.F, Z_FLAG) == true)
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
		};
	}

}




void loadRom(const char *romName)
{
	std::FILE* fp = fopen(romName, "rb");
	if (!fp)
	{
		std::cout << "ROM could not be loaded." << std::endl;
	}
	else
	{
		fread(ram, 1, 0x8000, fp);
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