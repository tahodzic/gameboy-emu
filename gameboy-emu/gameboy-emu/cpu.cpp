#include "cpu.h"
#include "opcodes.h"
#include <fstream>
#include <iostream>

int main()
{
	initialize();
	loadRom("D:/Other/Gameboy/Game/Tetris (World).gb");

	while(1)
	{
		executeOpcode(fetchOpcode());
		updateFlagRegister();
	}


	return 0;
}

void initialize()
{
	programCounter = 0x100;
	stackPointer = 0xE000; //stack: from $C000 - $DFFF of ram, grows downward
	registers.A = 0x1;
	registers.F = 0xB0;
	registers.B = 0x0;
	registers.C = 0x13;
	registers.D = 0x0;
	registers.E = 0xD8;
	registers.H = 0x01;
	registers.L = 0x4D;
	flags.Z = 0x1;
	flags.H = 0x1;
	flags.C = 0x1;

}

void updateFlagRegister() 
{
	registers.F = flags.Z << 7 | flags.N << 6 | flags.H << 5 | flags.C << 4;
}

void loadRom(const char *romName)
{
	std::ifstream rom(romName, std::ifstream::binary);

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

int fetchOpcode()
{
	int opcode = 0;

	//for (int i = 0; i < opcodes.at(ram[programCounter]); i++)
	//{
	//	opcode += ram[programCounter + i];
	//}


	opcode = ram[programCounter] & 0xFF;
	if (opcode == 0x00CB)
	{
		programCounter++;
		opcode = opcode << 8 | ram[programCounter];
	}

	if (opcode == 0x10)
	{
		//ram[++programCounter] == 0x00 ? opcode = (opcode << 8 | ram[programCounter]) : programCounter--;
		if (ram[programCounter + 1] == 0x00)
			opcode = (opcode << 8 | ram[++programCounter]);
	}
	programCounter++;
	std::cout << "Opcode: " << std::hex << opcode << std::endl;

	return opcode;
}

//0xF0 (0x02B2 PC) Befehl dort stehengeblieben. Er lädt von FF00+44 den Wert 00, obwohl es 3E sein sollte. Anscheinend ist das ein LCD wert/register das immer wieder inkrementiert wird. 
//Dem nachgehen.
void executeOpcode(int opcode)
{
	switch (opcode)
	{
		case 0x00:
		{
			break;
		}



		/*LD nn, n*/
		/*LD B, n*/
		case 0x06:
		{
			registers.B = ram[programCounter];
			programCounter++;
			break;
		}
		/*LD C, n*/
		case 0x0E:
		{
			registers.C = ram[programCounter];
			programCounter++;
			break;

		}
		/*LD D, n*/
		case 0x16:
		{
			break;

		}
		/*LD E, n*/
		case 0x1E:
		{
			break;

		}
		/*LD H, n*/
		case 0x26:
		{
			break;
		}
		/*LD L, n*/
		case 0x2E:
		{
			break;
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
		/*LD (HL),n*/ case 0x36:
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

		/*LD A,(nn)*/ case 0xFA:
		{
			break;
		}
		/*LD A,#*/ case 0x3E:
		{
			registers.A = ram[programCounter];
			programCounter++;
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
			int regHL = ((registers.H << 8) & 0xFF00) + (registers.L & 0xFF);
			ram[regHL]= registers.A;
			regHL--;
			registers.H = (regHL >> 8) & 0xFF;
			registers.L = regHL & 0xFF;
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
			int n = ram[programCounter] & 0xFF;
			ram[0xFF00 + n] = registers.A;
			programCounter++;
			break;
		}

		/*LDH A,(n)*/
		/*LD A,($FF00+n)*/ case 0xF0:
		{
			int n = ram[programCounter] & 0xFF;
			registers.A = ram[0xFF00 + n] & 0xFF;
			programCounter++;
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
			registers.L = ram[programCounter];
			registers.H = ram[++programCounter];
			programCounter++;
			break;
		}
		/*LD SP,nn*/ case 0x31: {
			break;
		}


		/*LD SP, HL*/ case 0xF9: {
			break;

		}

		/*LDHL SP, n*/ case 0xF8: {
			break;

		}


		/*LD (nn),SP*/ case 0x08: {
			break;

		}


		/*PUSH AF*/ case 0xF5: {
			break;

		}


		/*PUSH BC*/ case 0xC5: {
			break;

		}


		/*PUSH DE*/ case 0xD5: {
			break;

		}


		/*PUSH HL*/ case 0xE5: {
			break;

		}



		/*POP AF*/ case 0xF1: {

			break;

		}


		/*POP BC*/ case 0xC1: {
			break;

		}


		/*POP DE*/ case 0xD1: {
			break;

		}


		/*POP HL*/ case 0xE1: {
			break;

		}


		/*ADD A,A*/ case 0x87:
		{
			break;
		}
		/*ADD A,B*/ case 0x80:
		{
			break;
		}
		/*ADD A,C*/ case 0x81:
		{
			break;
		}
		/*ADD A,D*/ case 0x82:
		{
			break;
		}
		/*ADD A,E*/ case 0x83:
		{
			break;
		}
		/*ADD A,H*/ case 0x84:
		{
			break;
		}
		/*ADD A,L*/ case 0x85:
		{
			break;
		}
		/*ADD A,(HL)*/ case 0x86:
		{
			break;
		}
		/*ADD A,#*/ case 0xC6:
		{
			break;
		}



		/*ADC A,A*/ case 0x8F:
		{
			break;
		}
		/*ADC A,B*/ case 0x88:
		{
			break;
		}
		/*ADC A,C*/ case 0x89:
		{
			break;
		}
		/*ADC A,D*/ case 0x8A:
		{
			break;
		}
		/*ADC A,E*/ case 0x8B:
		{
			break;
		}
		/*ADC A,H*/ case 0x8C:
		{
			break;
		}
		/*ADC A,L*/ case 0x8D:
		{
			break;
		}
		/*ADC A,(HL)*/ case 0x8E:
		{
			break;
		}
		/*ADC A,#*/ case 0xCE:
		{
			break;
		}


		/*SUB A*/ case 0x97:
		{
			break;
		}
		/*SUB B*/ case 0x90:
		{
			break;
		}
		/*SUB C*/ case 0x91:
		{
			break;
		}
		/*SUB D*/ case 0x92:
		{
			break;
		}
		/*SUB E*/ case 0x93:
		{
			break;
		}
		/*SUB H*/ case 0x94:
		{
			break;
		}
		/*SUB L*/ case 0x95:
		{
			break;
		}
		/*SUB (HL)*/ case 0x96:
		{
			break;
		}
		/*SUB #*/ case 0xD6:
		{
			break;
		}


		/*SBC A,A*/ case 0x9F:
		{
			break;
		}
		/*SBC A,B*/ case 0x98:
		{
			break;
		}
		/*SBC A,C*/ case 0x99:
		{
			break;
		}
		/*SBC A,D*/ case 0x9A:
		{
			break;
		}
		/*SBC A,E*/ case 0x9B:
		{
			break;
		}
		/*SBC A,H*/ case 0x9C:
		{
			break;
		}
		/*SBC A,L*/ case 0x9D:
		{
			break;
		}
		/*SBC A,(HL)*/ case 0x9E:
		{
			break;
		}
		/*SBC A,#*/ case 0xDE:
		{
			break;
		}



		/*AND A*/ case 0xA7:
		{
			break;
		}
		/*AND B*/ case 0xA0:
		{
			break;
		}
		/*AND C*/ case 0xA1:
		{
			break;
		}
		/*AND D*/ case 0xA2:
		{
			break;
		}
		/*AND E*/ case 0xA3:
		{
			break;
		}
		/*AND H*/ case 0xA4:
		{
			break;
		}
		/*AND L*/ case 0xA5:
		{
			break;
		}
		/*AND (HL)*/ case 0xA6:
		{
			break;
		}
		/*AND #*/ case 0xE6:
		{
			break;
		}



		/*OR A*/ case 0xB7:
		{
			break;
		}
		/*OR B*/ case 0xB0:
		{
			break;
		}
		/*OR C*/ case 0xB1:
		{
			break;
		}
		/*OR D*/ case 0xB2:
		{
			break;
		}
		/*OR E*/ case 0xB3:
		{
			break;
		}
		/*OR H*/ case 0xB4:
		{
			break;
		}
		/*OR L*/ case 0xB5:
		{
			break;
		}
		/*OR (HL)*/ case 0xB6:
		{
			break;
		}
		/*OR #*/ case 0xF6:
		{
			break;
		}



		/*XOR A*/ case 0xAF:
		{
			registers.A ^= registers.A;
			if (registers.A == 0x00)
				flags.Z = 0x1;

			
			flags.N = 0x0;
			flags.C = 0x0;
			flags.H = 0x0;
			break;
		}
		/*XOR B*/ case 0xA8:
		{
			break;
		}
		/*XOR C*/ case 0xA9:
		{
			break;
		}
		/*XOR D*/ case 0xAA:
		{
			break;
		}
		/*XOR E*/ case 0xAB:
		{
			break;
		}
		/*XOR H*/ case 0xAC:
		{
			break;
		}
		/*XOR L */case 0xAD:
		{
			break;
		}
		/*XOR (HL)*/ case 0xAE:
		{
			break;
		}
		/*XOR * */ case 0xEE:
		{
			break;
		}



		/*CP A*/ case 0xBF:
		{
			break;
		}
		/*CP B*/ case 0xB8:
		{
			break;
		}
		/*CP C*/ case 0xB9:
		{
			break;
		}
		/*CP D*/ case 0xBA:
		{
			break;
		}
		/*CP E*/ case 0xBB:
		{
			break;
		}
		/*CP H*/ case 0xBC:
		{
			break;
		}
		/*CP L*/ case 0xBD:
		{
			break;
		}
		/*CP (HL)*/ case 0xBE:
		{
			break;
		}
		/*CP #*/ case 0xFE:
		{
			break;
		}


		/*INC A*/ case 0x3C:
		{
			break;
		}
		/*INC B*/ case 0x04:
		{
			break;
		}
		/*INC C*/ case 0x0C:
		{
			break;
		}
		/*INC D*/ case 0x14:
		{
			break;
		}
		/*INC E*/ case 0x1C:
		{
			break;
		}
		/*INC H*/ case 0x24:
		{
			break;
		}
		/*INC L*/ case 0x2C:
		{
			break;
		}
		/*INC (HL)*/ case 0x34:
		{
			break;
		}



		/*DEC A*/ case 0x3D:
		{
			unsigned char res = registers.A - 1;
			unsigned char tmp = registers.A;


			if (tmp - 1 == 0x0)
				flags.Z = 0x1;
			else flags.Z = 0x0;

			flags.H = (((registers.A) ^ (1) ^ res) & 0x10) >> 4;
			tmp--;
			registers.A = tmp;
			flags.N = 0x1;

			break;
		}
		/*DEC B*/ case 0x05:
		{
			unsigned char res = registers.B - 1;
			unsigned char tmp = registers.B;


			if (tmp - 1 == 0x0)
				flags.Z = 0x1;
			else flags.Z = 0x0;
			
			flags.H = (((registers.B) ^ (1) ^ res) & 0x10) >> 4;
			tmp--;
			registers.B = tmp;
			//if (registers.B == 0x00)
			//{
			//	registers.B = 0xFF;
			//	flags.Z = 0x0;

			//}
			//else if (registers.B - 1 == 0x0)
			//{
			//	flags.Z = 0x1;
			//	registers.B--;
			//}
			//else 
			//{
			//	registers.B--;
			//	flags.Z = 0x0;
			//}

			flags.N = 0x1;



			break;
		}
		/*DEC C*/ case 0x0D:
		{
			unsigned char res = registers.C - 1;
			unsigned char tmp = registers.C;


			if (tmp - 1 == 0x0)
				flags.Z = 0x1;
			else flags.Z = 0x0;

			flags.H = (((registers.C) ^ (1) ^ res) & 0x10) >> 4;
			tmp--;
			registers.C = tmp;
			flags.N = 0x1;

			break;
		}
		/*DEC D*/ case 0x15:
		{
			unsigned char res = registers.D - 1;
			unsigned char tmp = registers.D;


			if (tmp - 1 == 0x0)
				flags.Z = 0x1;
			else flags.Z = 0x0;

			flags.H = (((registers.D) ^ (1) ^ res) & 0x10) >> 4;
			tmp--;
			registers.D = tmp;
			flags.N = 0x1;

			
			break;
		}
		/*DEC E*/ case 0x1D:
		{
			unsigned char res = registers.E - 1;
			unsigned char tmp = registers.E;


			if (tmp - 1 == 0x0)
				flags.Z = 0x1;
			else flags.Z = 0x0;

			flags.H = (((registers.E) ^ (1) ^ res) & 0x10) >> 4;
			tmp--;
			registers.E = tmp;
			flags.N = 0x1;

			
			break;
		}
		/*DEC H*/ case 0x25:
		{
			unsigned char res = registers.H - 1;
			unsigned char tmp = registers.H;


			if (tmp - 1 == 0x0)
				flags.Z = 0x1;
			else flags.Z = 0x0;

			flags.H = (((registers.H) ^ (1) ^ res) & 0x10) >> 4;
			tmp--;
			registers.H = tmp;
			flags.N = 0x1;

			
			break;
		}
		/*DEC L*/ case 0x2D:
		{
			unsigned char res = registers.H - 1;
			unsigned char tmp = registers.H;


			if (tmp - 1 == 0x0)
				flags.Z = 0x1;
			else flags.Z = 0x0;

			flags.H = (((registers.H) ^ (1) ^ res) & 0x10) >> 4;
			tmp--;
			registers.H = tmp;
			flags.N = 0x1;

			
			break;
		}
		/*DEC (HL)*/ case 0x35:
		{
			//unsigned char res = registers.C - 1;
			//unsigned char tmp = registers.C;



			//if (tmp - 1 == 0x0)
			//	flags.Z = 0x1;
			//else flags.Z = 0x0;

			//flags.H = (((registers.C) ^ (1) ^ res) & 0x10) >> 4;
			//tmp--;
			//registers.C = tmp;
			//flags.N = 0x1;

			
			break;
		}



		/*ADD HL,BC*/ case 0x09:
		{
			break;
		}
		/*ADD HL,DE*/ case 0x19:
		{
			break;
		}
		/*ADD HL,HL*/ case 0x29:
		{
			break;
		}
		/*ADD HL,SP*/ case 0x39:
		{
			break;
		}


		/*ADD SP, #*/ case 0xE8:
		{
			break;
		}



		/*INC BC*/ case 0x03:
		{
			break;
		}
		/*INC DE*/ case 0x13:
		{
			break;
		}
		/*INC HL*/ case 0x23:
		{
			break;
		}
		/*INC SP*/ case 0x33:
		{
			break;
		}



		/*DEC BC*/ case 0x0B:
		{
			break;
		}
		/*DEC DE*/ case 0x1B:
		{
			break;
		}
		/*DEC HL*/ case 0x2B:
		{
			break;
		}
		/*DEC SP*/ case 0x3B:
		{
			break;
		}




		/*SWAP A*/ case 0xCB37:
		{
			break;
		}
		/*SWAP B*/ case 0xCB30:
		{
			break;
		}
		/*SWAP C*/ case 0xCB31:
		{
			break;
		}
		/*SWAP D*/ case 0xCB32:
		{
			break;
		}
		/*SWAP E*/ case 0xCB33:
		{
			break;
		}
		/*SWAP H*/ case 0xCB34:
		{
			break;
		}
		/*SWAP L*/ case 0xCB35:
		{
			break;
		}
		/*SWAP (HL)*/ case 0xCB36:
		{
			break;
		}



		/*DAA*/ case 0x27:
		{
			break;
		}

		/*CPL*/ case 0x2F:
		{
			break;
		}


		/*CCF*/ case 0x3F:
		{
			break;
		}

		/*SCF*/ case 0x37:
		{
			break;
		}

		/*HALT*/ case 0x76:
		{
			break;
		}

		/*STOP*/ case 0x1000:
		{


			break;
		}


		/*DI*/ case 0xF3:
		{
			imeFlag = 0x00;
			break;
		}



		/*EI*/ case 0xFB:
		{
			break;
		}

		/*RLCA*/ case 0x07:
		{
			break;
		}

		/*RLA*/ case 0x17:
		{
			break;
		}

		/*RRCA*/ case 0x0F:
		{
			break;
		}

		/*RRA*/ case 0x1F:
		{
			break;
		}

		/*RLC A*/ case 0xCB07:
		{
			break;
		}
		/*RLC B*/ case 0xCB00:
		{
			break;
		}
		/*RLC C*/ case 0xCB01:
		{
			break;
		}
		/*RLC D*/ case 0xCB02:
		{
			break;
		}
		/*RLC E*/ case 0xCB03:
		{
			break;
		}
		/*RLC H*/ case 0xCB04:
		{
			break;
		}
		/*RLC L*/ case 0xCB05:
		{
			break;
		}
		/*RLC (HL)*/ case 0xCB06:
		{
			break;
		}


		/*RL A*/ case 0xCB17:
		{
			break;
		}
		/*RL B*/ case 0xCB10:
		{
			break;
		}
		/*RL C*/ case 0xCB11:
		{
			break;
		}
		/*RL D*/ case 0xCB12:
		{
			break;
		}
		/*RL E*/ case 0xCB13:
		{
			break;
		}
		/*RL H*/ case 0xCB14:
		{
			break;
		}
		/*RL L*/ case 0xCB15:
		{
			break;
		}
		/*RL (HL)*/ case 0xCB16:
		{
			break;
		}



		/*RRC A*/ case 0xCB0F:
		{
			break;
		}
		/*RRC B*/ case 0xCB08:
		{
			break;
		}
		/*RRC C*/ case 0xCB09:
		{
			break;
		}
		/*RRC D*/ case 0xCB0A:
		{
			break;
		}
		/*RRC E*/ case 0xCB0B:
		{
			break;
		}
		/*RRC H*/ case 0xCB0C:
		{
			break;
		}
		/*RRC L*/ case 0xCB0D:
		{
			break;
		}
		/*RRC (HL)*/ case 0xCB0E:
		{
			break;
		}

		/*RR A */case 0xCB1F:
		{
			break;
		}
		/*RR B */case 0xCB18:
		{
			break;
		}
		/*RR C */case 0xCB19:
		{
			break;
		}
		/*RR D */case 0xCB1A:
		{
			break;
		}
		/*RR E */case 0xCB1B:
		{
			break;
		}
		/*RR H */case 0xCB1C:
		{
			break;
		}
		/*RR L */case 0xCB1D:
		{
			break;
		}
		/*RR (HL)*/case 0xCB1E:
		{
			break;
		}

		/*SLA A*/ case 0xCB27:
		{
			break;
		}
		/*SLA B*/ case 0xCB20:
		{
			break;
		}
		/*SLA C*/ case 0xCB21:
		{
			break;
		}
		/*SLA D*/ case 0xCB22:
		{
			break;
		}
		/*SLA E*/ case 0xCB23:
		{
			break;
		}
		/*SLA H*/ case 0xCB24:
		{
			break;
		}
		/*SLA L*/ case 0xCB25:
		{
			break;
		}
		/*SLA (HL)*/ case 0xCB26:
		{
			break;
		}

		/*SRA A*/ case 0xCB2F:
		{
			break;
		}
		/*SRA B*/ case 0xCB28:
		{
			break;
		}
		/*SRA C*/ case 0xCB29:
		{
			break;
		}
		/*SRA D*/ case 0xCB2A:
		{
			break;
		}
		/*SRA E*/ case 0xCB2B:
		{
			break;
		}
		/*SRA H*/ case 0xCB2C:
		{
			break;
		}
		/*SRA L*/ case 0xCB2D:
		{
			break;
		}
		/*SRA (HL)*/ case 0xCB2E:
		{
			break;
		}

		/*SRL A*/ case 0xCB3F:
		{
			break;
		}
		/*SRL B*/ case 0xCB38:
		{
			break;
		}
		/*SRL C*/ case 0xCB39:
		{
			break;
		}
		/*SRL D*/ case 0xCB3A:
		{
			break;
		}
		/*SRL E*/ case 0xCB3B:
		{
			break;
		}
		/*SRL H*/ case 0xCB3C:
		{
			break;
		}
		/*SRL L*/ case 0xCB3D:
		{
			break;
		}
		/*SRL (HL)*/ case 0xCB3E:
		{
			break;
		}


		/*BIT b,A*/ case 0xCB47:
		{
			break;
		}
		/*BIT b,B*/ case 0xCB40:
		{
			break;
		}
		/*BIT b,C*/ case 0xCB41:
		{
			break;
		}
		/*BIT b,D*/ case 0xCB42:
		{
			break;
		}
		/*BIT b,E*/ case 0xCB43:
		{
			break;
		}
		/*BIT b,H*/ case 0xCB44:
		{
			break;
		}
		/*BIT b,L*/ case 0xCB45:
		{
			break;
		}
		/*BIT b,(HL)*/ case 0xCB46:
		{
			break;
		}


		/*SET b,A*/ case 0xCBC7:
		{
			break;
		}
		/*SET b,B*/ case 0xCBC0:
		{
			break;
		}
		/*SET b,C*/ case 0xCBC1:
		{
			break;
		}
		/*SET b,D*/ case 0xCBC2:
		{
			break;
		}
		/*SET b,E*/ case 0xCBC3:
		{
			break;
		}
		/*SET b,H*/ case 0xCBC4:
		{
			break;
		}
		/*SET b,L*/ case 0xCBC5:
		{
			break;
		}
		/*SET b,(HL)*/ case 0xCBC6:
		{
			break;
		}

		/*RES b,A*/ case 0xCB87:
		{
			break;
		}
		/*RES b,B*/ case 0xCB80:
		{
			break;
		}
		/*RES b,C*/ case 0xCB81:
		{
			break;
		}
		/*RES b,D*/ case 0xCB82:
		{
			break;
		}
		/*RES b,E*/ case 0xCB83:
		{
			break;
		}
		/*RES b,H*/ case 0xCB84:
		{
			break;
		}
		/*RES b,L*/ case 0xCB85:
		{
			break;
		}
		/*RES b,(HL)*/ case 0xCB86:
		{
			break;
		}


		/*JP nn*/ case 0xC3:
		{
		    int newAddressToJumpTo = ((ram[programCounter + 1] << 8) + (ram[programCounter]&0xFF));
			programCounter = newAddressToJumpTo;
			break;
		}

		/*JP NZ,nn*/ case 0xC2:
		{
			break;
		}
		/*JP Z,nn*/ case 0xCA:
		{
			break;
		}
		/*JP NC,nn*/ case 0xD2:
		{
			break;
		}
		/*JP C,nn*/ case 0xDA:
		{
			break;
		}

		/*JP (HL)*/ case 0xE9:
		{
			break;
		}
		/*JR n*/ case 0x18:
		{
			break;
		}

		/*JR NZ,**/ case 0x20:
		{
			if (flags.Z == 0x00)
			{
				programCounter += (signed)ram[programCounter];
				programCounter++;
			}
			else programCounter++;
			break;
		}
		/*JR Z,* */case 0x28:
		{
			break;
		}
		/*JR NC,**/ case 0x30:
		{
			break;
		}
		/*JR C,* */case 0x38:
		{
			break;
		}

		/*CALL nn*/ case 0xCD:
		{
			break;
		}

		/*CALL NZ,nn*/ case 0xC4:
		{
			break;
		}
		/*CALL Z,nn*/ case 0xCC:
		{
			break;
		}
		/*CALL NC,nn*/ case 0xD4:
		{
			break;
		}
		/*CALL C,nn*/ case 0xDC:
		{
			break;
		}

		/*RST 00H*/ case 0xC7:
		{
			break;
		}
		/*RST 08H*/ case 0xCF:
		{
			break;
		}
		/*RST 10H*/ case 0xD7:
		{
			break;
		}
		/*RST 18H*/ case 0xDF:
		{
			break;
		}
		/*RST 20H*/ case 0xE7:
		{
			break;
		}
		/*RST 28H*/ case 0xEF:
		{
			break;
		}
		/*RST 30H*/ case 0xF7:
		{
			break;
		}
		/*RST 38H*/ case 0xFF:
		{
			break;
		}

		/*RET -/-*/ case 0xC9:
		{
			break;
		}

		/*RET NZ*/ case 0xC0:
		{
			break;
		}
		/*RET Z*/ case 0xC8:
		{
			break;
		}
		/*RET NC */case 0xD0:
		{
			break;
		}
		/*RET C*/ case 0xD8:
		{
			break;
		}

		/*RETI -/-*/ case 0xD9:
		{
			break;
		}


		default:
		{
			std::cout << "Following opcode needs to be implemented: " << std::hex << opcode << std::endl;
		};
	}

}