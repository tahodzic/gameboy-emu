#include "cpu.h"
#include "opcodes.h"
#include <fstream>
#include <iostream>

int main()
{
	initialize();
	loadRom("D:/Other/Gameboy/Game/Tetris (World).gb");
	
	for(int i = 0; i < 20; i++)
	{
		executeOpcode(fetchOpcode());
	}

	registers.A = 0;
	
	return 0;
}

void initialize()
{
	programCounter = 0x100;
	stackPointer = 0xE000; //stack: from $C000 - $DFFF of ram, grows downward
}

void loadRom(const char *romName) 
{
	 std::ifstream rom (romName, std::ifstream::binary);

	 rom.seekg(0, rom.end);
	 int length = rom.tellg();
	 rom.seekg(0, rom.beg);

	 if (rom)
		 rom.read(ram, length);
	 else
		 std::cout << "Couldn't open following rom: " << romName;

	 if (rom)
		 std::cout << "Rom opened successfully." << std::endl;
	 else
		 std::cout << "Error: only " << rom.gcount() << " could be read" << std::endl;

	 //Close rom, since it's now loaded into ram
	 rom.close();
}

short int fetchOpcode()
{
	short int opcode = 0;

	//for (int i = 0; i < opcodes.at(ram[programCounter]); i++)
	//{
	//	opcode += ram[programCounter + i];
	//}
	
	opcode = ram[programCounter] & 0x00FF;
	programCounter++;
	std::cout << "Opcode: " << std::hex << opcode << std::endl;

	return opcode;
}

void executeOpcode(short int opcode)
{
	switch (opcode)
	{
		case 0x00:
		{
			break;
		}
		case 0xC3:
		{
			short int newAddressToJumpTo = (ram[programCounter + 1] << 8) + ram[programCounter];
			programCounter = newAddressToJumpTo;
			break;
		}

		/*LD nn, n*/
		/*LD B, n*/
		case 0x06:
		{

		}
		/*LD C, n*/
		case 0x0E:
		{
		
		}
		/*LD D, n*/
		case 0x16:
		{
		
		}
		/*LD E, n*/
		case 0x1E:
		{

		}
		/*LD H, n*/
		case 0x26:
		{

		}
		/*LD L, n*/
		case 0x2E:
		{
		
		}


		/*LD r1, r2*/
		/*LD A,A*/ case 0x7F:
		{
			break;
		}
		/*LD A,B*/ case 0x78:
		{
			break;
		}
		/*LD A,C*/ case 0x79:
		{
			break;
		}
		/*LD A,D*/ case 0x7A:
		{
			break;
		}
		/*LD A,E*/ case 0x7B:
		{
			break;
		}
		/*LD A,H*/ case 0x7C:
		{
			break;
		}
		/*LD A,L*/ case 0x7D:
		{
			break;
		}
		/*LD A,(HL)*/ case 0x7E:
		{
			break;
		}
		/*LD B,B*/ case 0x40:
		{
			break;
		}
		/*LD B,C*/ case 0x41:
		{
			break;
		}
		/*LD B,D*/ case 0x42:
		{
			break;
		}
		/*LD B,E*/ case 0x43:
		{
			break;
		}
		/*LD B,H*/ case 0x44:
		{
			break;
		}
		/*LD B,L*/ case 0x45:
		{
			break;
		}
		/*LD B,(HL)*/ case 0x46:
		{
			break;
		}
		/*LD C,B*/ case 0x48:
		{
			break;
		}
		/*LD C,C*/ case 0x49:
		{
			break;
		}
		/*LD C,D*/ case 0x4A:
		{
			break;
		}
		/*LD C,E*/ case 0x4B:
		{
			break;
		}
		/*LD C,H*/ case 0x4C:
		{
			break;
		}
		/*LD C,L*/ case 0x4D:
		{
			break;
		}
		/*LD C,(HL)*/ case 0x4E:
		{
			break;
		}
		/*LD D,B*/ case 0x50:
		{
			break;
		}
		/*LD D,C*/ case 0x51:
		{
			break;
		}

		{
			break;
		}
		/*LD D,D*/ case 0x52:
		{
			break;
		}
		/*LD D,E*/ case 0x53:
		{
			break;
		}
		/*LD D,H*/ case 0x54:
		{
			break;
		}
		/*LD D,L*/ case 0x55:
		{
			break;
		}
		/*LD D,(HL)*/ case 0x56:
		{
			break;
		}
		/*LD E,B*/ case 0x58:
		{
			break;
		}
		/*LD E,C*/ case 0x59:
		{
			break;
		}
		/*LD E,D*/ case 0x5A:
		{
			break;
		}
		/*LD E,E*/ case 0x5B:
		{
			break;
		}
		/*LD E,H*/ case 0x5C:
		{
			break;
		}
		/*LD E,L*/ case 0x5D:
		{
			break;
		}
		/*LD E,(HL)*/ case 0x5E:
		{
			break;
		}
		/*LD H,B*/ case 0x60:
		{
			break;
		}
		/*LD H,C*/ case 0x61:
		{
			break;
		}
		/*LD H,D*/ case 0x62:
		{
			break;
		}
		/*LD H,E*/ case 0x63:
		{
			break;
		}
		/*LD H,H*/ case 0x64:
		{
			break;
		}
		/*LD H,L*/ case 0x65:
		{
			break;
		}
		/*LD H,(HL)*/ case 0x66:
		{
			break;
		}
		/*LD L,B*/ case 0x68:
		{
			break;
		}
		/*LD L,C*/ case 0x69:
		{
			break;
		}
		/*LD L,D*/ case 0x6A:
		{
			break;
		}
		/*LD L,E*/ case 0x6B:
		{
			break;
		}
		/*LD L,H*/ case 0x6C:
		{
			break;
		}
		/*LD L,L*/ case 0x6D:
		{
			break;
		}
		/*LD L,(HL)*/ case 0x6E:
		{
			break;
		}
		/*LD (HL),B*/ case 0x70:
		{
			break;
		}
		/*LD (HL),C*/ case 0x71:
		{
			break;
		}
		/*LD (HL),D*/ case 0x72:
		{
			break;
		}
		/*LD (HL),E*/ case 0x73:
		{
			break;
		}
		/*LD (HL),H*/ case 0x74:
		{
			break;
		}
		/*LD (HL),L*/ case 0x75:
		{
			break;
		}
		/*LD (HL),n*/ case 0x76:
		{
			break;
		}

		/*LD A,(BC)*/ case 0x0A:
		{
			break;
		}
		/*LD A,(DE)*/ case 0x1A:
		{
			break;
		}

		/*LD A,(nn)*/ case 0xFA :
		{
			break;
		}
		/*LD A,#*/ case 0x3E:
		{
			break;
		}


		/*LD n,A*/
		/*LD B,A*/ case 0x47:
		{
			break;
		}
		/*LD C,A*/ case 0x4F:
		{
			break;
		}
		/*LD D,A*/ case 0x57:
		{
			break;
		}
		/*LD E,A*/ case 0x5F:
		{
			break;
		}
		/*LD H,A*/ case 0x67:
		{
			break;
		}
		/*LD L,A*/ case 0x6F:
		{
			break;
		}
		/*LD (BC),A*/ case 0x02:
		{
			break;
		}
		/*LD (DE),A*/ case 0x12:
		{
			break;
		}
		/*LD (HL),A*/ case 0x77:
		{
			break;
		}
		/*LD (nn),A*/ case 0xEA:
		{
			break;
		}

		
		/*LD A, (C)*/ case 0xF2:
		{
			break;
		}

		/*LD (C), A*/ case 0xE2:
		{
			break;
		}

		/*LD A,(HLD)*/
		/*LD A,(HL-)*/
		/*LDD A,(HL)*/ case 0x3A:
		{
			break;
		}

		/*LD (HLD),A */
		/*LD (HL-),A */
		/*LDD (HL),A*/ case 0x32:
		{
			break;
		}

		/*LD A,(HLI) */
		/*LD A,(HL+) */
		/*LDI A,(HL)*/
		case 0x2A:
		{
			break;
		}

		/*LD (HLI),A */
/*LD (HL+),A */
/*LDI (HL),A*/
		case 0x22:
		{
			break;
		}

		/*LDH (n),A*/
		/*LD ($FF00+n),A*/ case 0xE0:
		{
			break;
		}

		/*LDH A,(n)*/
		/*LD A,($FF00+n)*/ case 0xF0: 
		{
			break;
		}

		/*LD n,nn*/
		/*LD BC,nn*/ case 0x01: {
			break;
		}
		/*LD DE,nn*/ case 0x11: {
			break;
		}
		/*LD HL,nn*/ case 0x21: {
			break;
		}
		/*LD SP,nn*/ case 0x31: {
			break;
		}


		/*LD SP, HL*/ case 0xF9: {
			break;

		}



		default: 
		{
			std::cout << "Following opcode needs to be implemented: " << std::hex << opcode << std::endl;
		}
	}

}