#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "cpu.h"
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
	char imeFlag;
	unsigned char screenData[160][144][3];

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

	struct flag
	{
		unsigned char Z;
		unsigned char N;
		unsigned char H;
		unsigned char C;
	} flags;

	 unsigned char ram[65536];
	 int countCycles;

void initialize()
{
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
	flags.Z = 0x1;
	flags.H = 0x1;
	flags.C = 0x1;
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

void updateFlagRegister() 
{
	registers.F = flags.Z << 7 | flags.N << 6 | flags.H << 5 | flags.C << 4;
}

void loadRom(const char *romName)
{
	//std::ifstream rom(romName, std::ifstream::binary);

	//rom.seekg(0, rom.end);
	//int length = rom.tellg();
	//rom.seekg(0, rom.beg);

	//if (rom)
	//	rom.read(ram, length);
	//else
	//	std::cout << "Couldn't open following rom: " << romName;

	//if (rom)
	//	std::cout << "Rom opened successfully." << std::endl;
	//else
	//	std::cout << "Error: only " << rom.gcount() << " could be read" << std::endl;

	////Close rom, since it's now loaded into ram
	//rom.close();

	//

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

unsigned char readRam(unsigned short address)
{
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

void startDmaTransfer(unsigned char data)
{
	unsigned short address = data << 8;
	for (int i = 0; i < 0xA0; i++)
	{
		writeRam(0xFE00 + i, readRam(address + i));
	}
}
/*16 bit writes*/
//TODO
void writeRam(unsigned short address, unsigned short data)
{

}

int fetchOpcode()
{
	unsigned char opcode = 0;

	//for (int i = 0; i < opcodes.at(ram[programCounter]); i++)
	//{
	//	opcode += ram[programCounter + i];
	//}


	//opcode = ram[programCounter] & 0xFF;
	opcode = readRam(programCounter);

	//TODO:
	//Both cases (0xCB and 0x10) can be easily solved with an additional nested switch
	//if (opcode == 0x00CB)
	//{
	//	programCounter++;
	//	opcode = opcode << 8 | readRam(programCounter);
	//}

	//if (opcode == 0x10)
	//{
	//	//ram[++programCounter] == 0x00 ? opcode = (opcode << 8 | ram[programCounter]) : programCounter--;
	//	if (readRam(programCounter+1) == 0x00)
	//		opcode = (opcode << 8 | readRam(++programCounter));
	//}
	programCounter++;
	//std::cout << "Opcode: " << std::hex << (opcode < 0x10 ? "0x0" : "0x") << (int)opcode << std::endl;

	return opcode;
}

void pushToStack(unsigned short data)
{
	stackPointer += 2;
	writeRam(stackPointer, data);
}

unsigned short popFromStack()
{
	unsigned short popValue = readRam(stackPointer);
	stackPointer -= 2;
	return popValue;
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
	irRequestFlagStatus = irRequestFlagStatus & !interruptNumber;
	writeRam(IR_REQUEST_ADDRESS, irRequestFlagStatus);
	switch (interruptNumber)
	{
		case VERTICAL_BLANKING: programCounter = 0x40; break;
		case LCDC: programCounter = 0x48; break;
		case TIMER_OVERFLOW: programCounter = 0x50; break;
		case SERIAL_IO_COMPLETION: programCounter = 0x58; break;
		case JOYPAD: programCounter = 0x60; break;
	}
}

void updateGraphics(int cycles)
{
	setLcdStatus();

	if (!isLcdEnabled())
	{
	}
	else
	{
		cyclesScanLine -= cycles;

		/*when cyclesScanLine is 0, then one entire scanLine has been drawn.*/
		if (cyclesScanLine <= 0)
		{
			unsigned char currentLine = readRam(LCDC_LY);
			currentLine++;
			writeRam(LCDC_LY, currentLine);

			cyclesScanLine = CYCLES_PER_SCAN_LINE;

			if (currentLine < 144)
				drawScanLine();

			else if (currentLine == 144)
				requestInterrupt(VERTICAL_BLANKING);

			else if (currentLine > 153)
				writeRam(LCDC_LY, (unsigned short)0);
		}
	}
}

void renderTiles()
{
	unsigned short tileData = 0, bgMemory = 0;
	bool unsign = true;


	unsigned char scrollY = readRam(0xFF42);
	unsigned char scrollX = readRam(0xFF43);
	unsigned char windowY = readRam(0xFF4A);
	unsigned char windowX = readRam(0xFF4B) - 7;
	

	bool useWindow = false;
	unsigned char control = readRam(LCDC_CTRL);



	//window enabled?
	if (control & 0x20)
	{
		if (windowY <= readRam(LCDC_LY))
			useWindow = true;

	}

	if (control & 0x10)
	{
		tileData = 0x8000;
	}
	else
	{
		tileData = 0x8800;
		unsign = false;
	}

	if (!useWindow)
	{
		if (control & 0x08)
			bgMemory = 0x9C00;
		else
			bgMemory = 0x9800;

	}
	else
	{
		if(control & 0x40)
			bgMemory = 0x9C00;
		else
			bgMemory = 0x9800;
	}

	unsigned char yPos = 0;

	if (useWindow)
		yPos = readRam(LCDC_LY) - windowY;
	else
		yPos = scrollY + readRam(LCDC_LY);

	// which of the 8 vertical pixels of the current
	// tile is the scanline on?
	unsigned short tileRow = (((unsigned char)(yPos / 8)) * 32);



	for (int pixel = 0; pixel < 160; pixel++)
	{
		unsigned char xPos = pixel + scrollX;

		if (useWindow)
		{
			if (pixel >= windowX)
				xPos = pixel - windowX;
		}
		unsigned short tileCol = (xPos / 8);
		short tileNum;

		unsigned short tileAddress = bgMemory + tileRow + tileCol;

		if (unsign)
			tileNum = (unsigned char)readRam(tileAddress);
		else
			tileNum = (char)readRam(tileAddress);

		unsigned short tileLocation = tileData;

		if (unsign)
			tileLocation += (tileNum * 16);
		else
			tileLocation += ((tileNum + 128) * 16);

		
		unsigned char line = yPos % 8;
		line *= 2; // each vertical line takes up two bytes of memory
		unsigned char data1 = readRam(tileLocation + line);
		unsigned char data2 = readRam(tileLocation + line + 1);


		// pixel 0 in the tile is bit 7 of data 1 and data 2
		// Pixel 1 in the tile is bit 6 of data 1 and data 2 etc.
		// hence the order needs to be turnt around
		// xPos goes from left to right (from low number to high number)
		// bits, however, go from right to left (from low number to high number)
		// so if you want the bit at xPos 0 you need bit number 7
		int colourBit = xPos % 8;
		/*colourBit -= 7;
		colourBit *= -1;*/
		colourBit = 7 - colourBit;

		// combine data 2 and data 1 to get the colour id for this pixel
		// in the tile
		int dotData = (data2 & (1 << (colourBit - 1)) ? 1 : 0);
		dotData <<= 1;
		dotData |= (data1 & (1 << (colourBit - 1)) ? 1 : 0);

		//lets get the shade from the palette at 0xFF47
		unsigned char palette = readRam(0xFF47);
		int shade;
		
		//replaces below switch
		shade = (palette & (1 << (2 * dotData + 1))) ? 1 : 0;
		shade <<= 1;
		shade |= (palette & (1 << (2 * dotData))) ? 1 : 0;

		int red = 0;
		int green = 0;
		int blue = 0;

		// setup the RGB values
		switch (shade)
		{
			case 0: red = 255;  green = 255; blue = 255; break;
			case 1: red = 0xCC; green = 0xCC; blue = 0xCC; break;
			case 2: red = 0x77; green = 0x77; blue = 0x77; break;
			case 3: red = 0x00; green = 0x00; blue = 0x00; break;
		}

		int ly = readRam(LCDC_LY);

		// safety check to make sure what im about
		// to set is int the 160x144 bounds
		if ((ly < 0) || (ly > 143) || (pixel < 0) || (pixel > 159))
		{
			//skip this cycle of the loop
		}
		else 
		{
			screenData[pixel][ly][0] = red;
			screenData[pixel][ly][1] = green;
			screenData[pixel][ly][2] = blue;
		}
		//switch (dotData)
		//{
		//	case 3: //Bit 7-6
		//	{
		//		shade = (palette & 0x80) ? 1 : 0;
		//		shade <<= 1;
		//		shade |= (palette & 0x40) ? 1 : 0;
		//		break;
		//	}
		//	case 2: //Bit 5-4
		//	{
		//		shade = (palette & 0x20) ? 1 : 0;
		//		shade <<= 1;
		//		shade |= (palette & 0x10) ? 1 : 0;
		//		break;
		//	}
		//	case 1: //Bit 3-2
		//	{
		//		shade = (palette & 0x08) ? 1 : 0;
		//		shade <<= 1;
		//		shade |= (palette & 0x04) ? 1 : 0;
		//		break;
		//	}
		//	case 0: //Bit 1-0
		//	{
		//		shade = (palette & 0x02) ? 1 : 0;
		//		shade <<= 1;
		//		shade |= (palette & 0x01) ? 1 : 0;
		//		break;
		//	}

		//}


	}
}

void renderSprites()
{
	unsigned char lcdcControl = readRam(LCDC_CTRL);
	bool use8x16 = false;
	if (lcdcControl & 0x02)
		use8x16 = true;

	for (int sprite = 0; sprite < 40; sprite++)
	{
		unsigned char index = sprite * 4;
		unsigned char yPos = readRam(0xFE00 + index) - 16;
		unsigned char xPos = readRam(0xFE00 + index + 1) - 8;
		unsigned char spriteLocation = readRam(0xFE00 + index + 2);
		unsigned char attributes = readRam(0xFE00 + index + 3);

		bool yFlip = (attributes & 0x40) ? true : false;
		bool xFlip = (attributes & 0x20) ? true : false;

		int ly = readRam(LCDC_LY);

		int ySize = 8;
		if (use8x16)
			ySize = 16;

		if ((ly >= yPos) && ly < (yPos + ySize))
		{
			int line = ly - yPos;

			//read the line in backwards
			if (yFlip)
			{
				line -= ySize;
				line *= -1;
			}

			line *= 2; 
			unsigned short dataAddress = (0x8000 + (spriteLocation * 16) + line);
			unsigned char data1 = readRam(dataAddress);
			unsigned char data2 = readRam(dataAddress + 1);

			// its easier to read in from right to left as pixel 0 is
			// bit 7 in the colour data, pixel 1 is bit 6 etc...
			for (int tilePixel = 7; tilePixel >= 0; tilePixel--)
			{
				int colourBit = tilePixel;
				// read the sprite in backwards for the x axis
				if (xFlip)
				{
					colourBit -= 7;
					colourBit *= -1;
				}

				// combine data 2 and data 1 to get the colour id for this pixel
				// in the tile
				int dotData = (data2 & (1 << (colourBit - 1)) ? 1 : 0);
				dotData <<= 1;
				dotData |= (data1 & (1 << (colourBit - 1)) ? 1 : 0);

				unsigned short paletteAddress = (attributes & 0x08) ? 0xFF49 : 0xFF48;

				//lets get the shade from the palette at 0xFF47
				unsigned char palette = readRam(paletteAddress);
				int shade;

				//replaces below switch
				shade = (palette & (1 << (2 * dotData + 1))) ? 1 : 0;
				shade <<= 1;
				shade |= (palette & (1 << (2 * dotData))) ? 1 : 0;

				int red = 0;
				int green = 0;
				int blue = 0;

				// setup the RGB values
				switch (shade)
				{
					case 0: red = 255;  green = 255; blue = 255; break;
					case 1: red = 0xCC; green = 0xCC; blue = 0xCC; break;
					case 2: red = 0x77; green = 0x77; blue = 0x77; break;
					case 3: red = 0x00; green = 0x00; blue = 0x00; break;
				}

				int xPix = 0 - tilePixel;
				xPix += 7;
				int pixel = xPos + xPix;

				int ly = readRam(LCDC_LY);

				// safety check to make sure what im about
				// to set is int the 160x144 bounds
				if ((ly < 0) || (ly > 143) || (pixel < 0) || (pixel > 159))
				{
					//skip this cycle of the loop
				}
				else
				{
					screenData[pixel][ly][0] = red;
					screenData[pixel][ly][1] = green;
					screenData[pixel][ly][2] = blue;
				}

			}
		}

	}
}


void drawScanLine()
{
	unsigned char control = readRam(LCDC_CTRL);
	if (control & 0x1)
		renderTiles();
	if (control & 0x2)
		renderSprites();
}

void setLcdStatus()
{
	unsigned char status = readRam(LCDC_STAT);
	if (!isLcdEnabled())
	{
		cyclesScanLine = CYCLES_PER_SCAN_LINE;
		writeRam(LCDC_LY, (unsigned char)0);

		//set mode to 01 (vblank mode)
		status &= 0xFC;
		status |= 0x01;
		writeRam(LCDC_STAT, status);
	}
	else
	{
		unsigned char currentLine = readRam(LCDC_LY);
		unsigned char currentMode = status & 0x03;

		unsigned char modeToSet = 0;
		unsigned short requestInt = 0;

		//over 144, so go into vblank mode
		if (currentLine > 144)
		{
			modeToSet = 0x01;
			status &= 0xFC; //delete status bits
			status |= modeToSet;
			requestInt = status & 0x10;

		}
		else
		{
			int mode2bounds = 456 - 80;
			int mode3bounds = mode2bounds - 172;

			if (cyclesScanLine >= mode2bounds)
			{
				modeToSet = 0x02;
				status &= 0xFC;
				status |= modeToSet;
				requestInt = status & 0x20;
			}
			else if (cyclesScanLine >= mode3bounds)
			{
				modeToSet = 0x03;
				status &= 0xFC;
				status |= modeToSet;
			}
			else
			{
				modeToSet = 0x00;
				status &= 0xFC;
				status |= modeToSet;
				requestInt = status & 0x08;
			}
		
		}

		if (requestInt && (modeToSet != currentMode))
			requestInterrupt(0x01);

		//check coincidence flag
		if (currentLine == readRam(0xFF45))
		{
			status |= 0x02;
			if (status & 0x40)
				requestInterrupt(0x01);
		}
		else
		{
			//make sure the coincidence flag (bit 2) is 0
			status &= 0xFD;
		}

		writeRam(LCDC_STAT, status);



	}
}

bool isLcdEnabled()
{
	return ((readRam(LCDC_CTRL) & 0x80) == 0x80);
}

//0xF0 (0x02B3 PC bei meinem) Befehl dort stehengeblieben. Er lädt von FF00+44 den Wert 00, obwohl es 3E sein sollte. Anscheinend ist das ein LCD wert/register das immer wieder inkrementiert wird. 
//Dem nachgehen.
int executeOpcode(unsigned char opcode)
{
	std::cout << "Opcode: " << std::hex << (opcode < 0x10 ? "0x0" : "0x") << (int)opcode << std::endl;
	std::cout << "af: 0x" << std::hex << +registers.A << +registers.F << std::endl;
	std::cout << "bc: 0x" << std::hex << (unsigned int)registers.B << (unsigned int)registers.C << std::endl;
	std::cout << "de: 0x" << std::hex << (unsigned int)registers.D << (unsigned int)registers.E << std::endl;
	std::cout << "hl: 0x" << std::hex << (unsigned int)registers.H << (unsigned short)registers.L << std::endl;
	std::cout << "sp: 0x" << std::hex << (int)stackPointer << std::endl;
	std::cout << "pc: 0x" << std::hex << (int)programCounter << std::endl;
	//bc: 0xed10 opcode:0x20 (danach ff44 ++)

	//0x0D first instruction after first loop 
		//TODO: 0xFA implementieren
	switch (opcode)
	{
		case 0x00:
		{
			return 4;
		}



		/*LD nn, n*/
		/*LD B, n*/
		case 0x06:
		{
			//registers.B = ram[programCounter];
			registers.B = readRam(programCounter);
			programCounter++;
			return 8;
		}
		/*LD C, n*/
		case 0x0E:
		{
			//registers.C = ram[programCounter];
			registers.C = readRam(programCounter);
			programCounter++;
			return 8;

		}
		///*LD D, n*/
		//case 0x16:
		//{
		//	break;

		//}
		///*LD E, n*/
		//case 0x1E:
		//{
		//	break;

		//}
		///*LD H, n*/
		//case 0x26:
		//{
		//	break;
		//}
		///*LD L, n*/
		//case 0x2E:
		//{
		//	break;
		//}


		///*LD r1, r2*/
		///*LD A,A*/ case 0x7F:
		//{
		//	break;
		//}
		///*LD A,B*/ case 0x78:
		//{
		//	break;
		//}
		///*LD A,C*/ case 0x79:
		//{
		//	break;
		//}
		///*LD A,D*/ case 0x7A:
		//{
		//	break;
		//}
		///*LD A,E*/ case 0x7B:
		//{
		//	break;
		//}
		///*LD A,H*/ case 0x7C:
		//{
		//	break;
		//}
		///*LD A,L*/ case 0x7D:
		//{
		//	break;
		//}
		///*LD A,(HL)*/ case 0x7E:
		//{
		//	break;
		//}
		///*LD B,B*/ case 0x40:
		//{
		//	break;
		//}
		///*LD B,C*/ case 0x41:
		//{
		//	break;
		//}
		///*LD B,D*/ case 0x42:
		//{
		//	break;
		//}
		///*LD B,E*/ case 0x43:
		//{
		//	break;
		//}
		///*LD B,H*/ case 0x44:
		//{
		//	break;
		//}
		///*LD B,L*/ case 0x45:
		//{
		//	break;
		//}
		///*LD B,(HL)*/ case 0x46:
		//{
		//	break;
		//}
		///*LD C,B*/ case 0x48:
		//{
		//	break;
		//}
		///*LD C,C*/ case 0x49:
		//{
		//	break;
		//}
		///*LD C,D*/ case 0x4A:
		//{
		//	break;
		//}
		///*LD C,E*/ case 0x4B:
		//{
		//	break;
		//}
		///*LD C,H*/ case 0x4C:
		//{
		//	break;
		//}
		///*LD C,L*/ case 0x4D:
		//{
		//	break;
		//}
		///*LD C,(HL)*/ case 0x4E:
		//{
		//	break;
		//}
		///*LD D,B*/ case 0x50:
		//{
		//	break;
		//}
		///*LD D,C*/ case 0x51:
		//{
		//	break;
		//}

		//{
		//	break;
		//}
		///*LD D,D*/ case 0x52:
		//{
		//	break;
		//}
		///*LD D,E*/ case 0x53:
		//{
		//	break;
		//}
		///*LD D,H*/ case 0x54:
		//{
		//	break;
		//}
		///*LD D,L*/ case 0x55:
		//{
		//	break;
		//}
		///*LD D,(HL)*/ case 0x56:
		//{
		//	break;
		//}
		///*LD E,B*/ case 0x58:
		//{
		//	break;
		//}
		///*LD E,C*/ case 0x59:
		//{
		//	break;
		//}
		///*LD E,D*/ case 0x5A:
		//{
		//	break;
		//}
		///*LD E,E*/ case 0x5B:
		//{
		//	break;
		//}
		///*LD E,H*/ case 0x5C:
		//{
		//	break;
		//}
		///*LD E,L*/ case 0x5D:
		//{
		//	break;
		//}
		///*LD E,(HL)*/ case 0x5E:
		//{
		//	break;
		//}
		///*LD H,B*/ case 0x60:
		//{
		//	break;
		//}
		///*LD H,C*/ case 0x61:
		//{
		//	break;
		//}
		///*LD H,D*/ case 0x62:
		//{
		//	break;
		//}
		///*LD H,E*/ case 0x63:
		//{
		//	break;
		//}
		///*LD H,H*/ case 0x64:
		//{
		//	break;
		//}
		///*LD H,L*/ case 0x65:
		//{
		//	break;
		//}
		///*LD H,(HL)*/ case 0x66:
		//{
		//	break;
		//}
		///*LD L,B*/ case 0x68:
		//{
		//	break;
		//}
		///*LD L,C*/ case 0x69:
		//{
		//	break;
		//}
		///*LD L,D*/ case 0x6A:
		//{
		//	break;
		//}
		///*LD L,E*/ case 0x6B:
		//{
		//	break;
		//}
		///*LD L,H*/ case 0x6C:
		//{
		//	break;
		//}
		///*LD L,L*/ case 0x6D:
		//{
		//	break;
		//}
		///*LD L,(HL)*/ case 0x6E:
		//{
		//	break;
		//}
		///*LD (HL),B*/ case 0x70:
		//{
		//	break;
		//}
		///*LD (HL),C*/ case 0x71:
		//{
		//	break;
		//}
		///*LD (HL),D*/ case 0x72:
		//{
		//	break;
		//}
		///*LD (HL),E*/ case 0x73:
		//{
		//	break;
		//}
		///*LD (HL),H*/ case 0x74:
		//{
		//	break;
		//}
		///*LD (HL),L*/ case 0x75:
		//{
		//	break;
		//}
		///*LD (HL),n*/ case 0x36:
		//{
		//	break;
		//}

		///*LD A,(BC)*/ case 0x0A:
		//{
		//	break;
		//}
		///*LD A,(DE)*/ case 0x1A:
		//{
		//	break;
		//}

		///*LD A,(nn)*/ case 0xFA:
		//{
		//	break;
		//}
		/*LD A,#*/ case 0x3E:
		{
			//registers.A = ram[programCounter];
			registers.A = readRam(programCounter);
			programCounter++;
			return 8;
		}


		///*LD n,A*/
		///*LD B,A*/ case 0x47:
		//{
		//	break;
		//}
		///*LD C,A*/ case 0x4F:
		//{
		//	break;
		//}
		///*LD D,A*/ case 0x57:
		//{
		//	break;
		//}
		///*LD E,A*/ case 0x5F:
		//{
		//	break;
		//}
		///*LD H,A*/ case 0x67:
		//{
		//	break;
		//}
		///*LD L,A*/ case 0x6F:
		//{
		//	break;
		//}
		///*LD (BC),A*/ case 0x02:
		//{
		//	break;
		//}
		///*LD (DE),A*/ case 0x12:
		//{
		//	break;
		//}
		///*LD (HL),A*/ case 0x77:
		//{
		//	break;
		//}
		///*LD (nn),A*/ case 0xEA:
		//{
		//	break;
		//}


		///*LD A, (C)*/ case 0xF2:
		//{
		//	break;
		//}

		///*LD (C), A*/ case 0xE2:
		//{
		//	break;
		//}

		///*LD A,(HLD)*/
		///*LD A,(HL-)*/
		///*LDD A,(HL)*/ case 0x3A:
		//{
		//	break;
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

//		/*LD A,(HLI) */
//		/*LD A,(HL+) */
//		/*LDI A,(HL)*/
//		case 0x2A:
//		{
//			break;
//		}
//
//		/*LD (HLI),A */
///*LD (HL+),A */
///*LDI (HL),A*/
//		case 0x22:
//		{
//			break;
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
			//registers.A = readRam(0xFF00+n);
			registers.A = 0x3E;
			programCounter++;
			return 12;
		}

		///*LD n,nn*/
		///*LD BC,nn*/ case 0x01: {
		//	break;
		//}
		///*LD DE,nn*/ case 0x11: {
		//	break;
		//}
		/*LD HL,nn*/ case 0x21: {
			//registers.L = ram[programCounter];
			//registers.H = ram[++programCounter];
			registers.L = readRam(programCounter);
			registers.H = readRam(++programCounter);
			programCounter++;
			return 12;
		}
		///*LD SP,nn*/ case 0x31: {
		//	break;
		//}


		///*LD SP, HL*/ case 0xF9: {
		//	break;

		//}

		///*LDHL SP, n*/ case 0xF8: {
		//	break;

		//}


		///*LD (nn),SP*/ case 0x08: {
		//	break;

		//}


		///*PUSH AF*/ case 0xF5: {
		//	break;

		//}


		///*PUSH BC*/ case 0xC5: {
		//	break;

		//}


		///*PUSH DE*/ case 0xD5: {
		//	break;

		//}


		///*PUSH HL*/ case 0xE5: {
		//	break;

		//}



		///*POP AF*/ case 0xF1: {

		//	break;

		//}


		///*POP BC*/ case 0xC1: {
		//	break;

		//}


		///*POP DE*/ case 0xD1: {
		//	break;

		//}


		///*POP HL*/ case 0xE1: {
		//	break;

		//}


		///*ADD A,A*/ case 0x87:
		//{
		//	break;
		//}
		///*ADD A,B*/ case 0x80:
		//{
		//	break;
		//}
		///*ADD A,C*/ case 0x81:
		//{
		//	break;
		//}
		///*ADD A,D*/ case 0x82:
		//{
		//	break;
		//}
		///*ADD A,E*/ case 0x83:
		//{
		//	break;
		//}
		///*ADD A,H*/ case 0x84:
		//{
		//	break;
		//}
		///*ADD A,L*/ case 0x85:
		//{
		//	break;
		//}
		///*ADD A,(HL)*/ case 0x86:
		//{
		//	break;
		//}
		///*ADD A,#*/ case 0xC6:
		//{
		//	break;
		//}



		///*ADC A,A*/ case 0x8F:
		//{
		//	break;
		//}
		///*ADC A,B*/ case 0x88:
		//{
		//	break;
		//}
		///*ADC A,C*/ case 0x89:
		//{
		//	break;
		//}
		///*ADC A,D*/ case 0x8A:
		//{
		//	break;
		//}
		///*ADC A,E*/ case 0x8B:
		//{
		//	break;
		//}
		///*ADC A,H*/ case 0x8C:
		//{
		//	break;
		//}
		///*ADC A,L*/ case 0x8D:
		//{
		//	break;
		//}
		///*ADC A,(HL)*/ case 0x8E:
		//{
		//	break;
		//}
		///*ADC A,#*/ case 0xCE:
		//{
		//	break;
		//}


		///*SUB A*/ case 0x97:
		//{
		//	break;
		//}
		///*SUB B*/ case 0x90:
		//{
		//	break;
		//}
		///*SUB C*/ case 0x91:
		//{
		//	break;
		//}
		///*SUB D*/ case 0x92:
		//{
		//	break;
		//}
		///*SUB E*/ case 0x93:
		//{
		//	break;
		//}
		///*SUB H*/ case 0x94:
		//{
		//	break;
		//}
		///*SUB L*/ case 0x95:
		//{
		//	break;
		//}
		///*SUB (HL)*/ case 0x96:
		//{
		//	break;
		//}
		///*SUB #*/ case 0xD6:
		//{
		//	break;
		//}


		///*SBC A,A*/ case 0x9F:
		//{
		//	break;
		//}
		///*SBC A,B*/ case 0x98:
		//{
		//	break;
		//}
		///*SBC A,C*/ case 0x99:
		//{
		//	break;
		//}
		///*SBC A,D*/ case 0x9A:
		//{
		//	break;
		//}
		///*SBC A,E*/ case 0x9B:
		//{
		//	break;
		//}
		///*SBC A,H*/ case 0x9C:
		//{
		//	break;
		//}
		///*SBC A,L*/ case 0x9D:
		//{
		//	break;
		//}
		///*SBC A,(HL)*/ case 0x9E:
		//{
		//	break;
		//}
		///*SBC A,#*/ case 0xDE:
		//{
		//	break;
		//}



		///*AND A*/ case 0xA7:
		//{
		//	break;
		//}
		///*AND B*/ case 0xA0:
		//{
		//	break;
		//}
		///*AND C*/ case 0xA1:
		//{
		//	break;
		//}
		///*AND D*/ case 0xA2:
		//{
		//	break;
		//}
		///*AND E*/ case 0xA3:
		//{
		//	break;
		//}
		///*AND H*/ case 0xA4:
		//{
		//	break;
		//}
		///*AND L*/ case 0xA5:
		//{
		//	break;
		//}
		///*AND (HL)*/ case 0xA6:
		//{
		//	break;
		//}
		///*AND #*/ case 0xE6:
		//{
		//	break;
		//}



		///*OR A*/ case 0xB7:
		//{
		//	break;
		//}
		///*OR B*/ case 0xB0:
		//{
		//	break;
		//}
		///*OR C*/ case 0xB1:
		//{
		//	break;
		//}
		///*OR D*/ case 0xB2:
		//{
		//	break;
		//}
		///*OR E*/ case 0xB3:
		//{
		//	break;
		//}
		///*OR H*/ case 0xB4:
		//{
		//	break;
		//}
		///*OR L*/ case 0xB5:
		//{
		//	break;
		//}
		///*OR (HL)*/ case 0xB6:
		//{
		//	break;
		//}
		///*OR #*/ case 0xF6:
		//{
		//	break;
		//}



		/*XOR A*/ case 0xAF:
		{
			registers.A ^= registers.A;
			if (registers.A == 0x00)
				flags.Z = 0x1;

			
			flags.N = 0x0;
			flags.C = 0x0;
			flags.H = 0x0;
			return 4;
		}
		///*XOR B*/ case 0xA8:
		//{
		//	break;
		//}
		///*XOR C*/ case 0xA9:
		//{
		//	break;
		//}
		///*XOR D*/ case 0xAA:
		//{
		//	break;
		//}
		///*XOR E*/ case 0xAB:
		//{
		//	break;
		//}
		///*XOR H*/ case 0xAC:
		//{
		//	break;
		//}
		///*XOR L */case 0xAD:
		//{
		//	break;
		//}
		///*XOR (HL)*/ case 0xAE:
		//{
		//	break;
		//}
		///*XOR * */ case 0xEE:
		//{
		//	break;
		//}



		///*CP A*/ case 0xBF:
		//{
		//	break;
		//}
		///*CP B*/ case 0xB8:
		//{
		//	break;
		//}
		///*CP C*/ case 0xB9:
		//{
		//	break;
		//}
		///*CP D*/ case 0xBA:
		//{
		//	break;
		//}
		///*CP E*/ case 0xBB:
		//{
		//	break;
		//}
		///*CP H*/ case 0xBC:
		//{
		//	break;
		//}
		///*CP L*/ case 0xBD:
		//{
		//	break;
		//}
		///*CP (HL)*/ case 0xBE:
		//{
		//	break;
		//}
		/*CP #*/ case 0xFE:
		{
			//unsigned char res = registers.A - ram[programCounter];
			unsigned char n = readRam(programCounter);
			unsigned char res = registers.A - n;
			if (res == 0)
			{
				flags.Z = 1;
			}
			else
			{
				flags.Z = 0;
			}
			
			flags.N = 1;
			//flags.H = (((registers.A) ^ ram[programCounter] ^ res) & 0x10) >> 4;
			flags.H = (((registers.A) ^ n ^ res) & 0x10) >> 4;
			registers.A < n ? flags.C = 1 : flags.C = 0;
			if ((unsigned)registers.A < n) {
				flags.C = 1;
			}
			else flags.C = 0;


			return 8;
		}


		///*INC A*/ case 0x3C:
		//{
		//	break;
		//}
		///*INC B*/ case 0x04:
		//{
		//	break;
		//}
		///*INC C*/ case 0x0C:
		//{
		//	break;
		//}
		///*INC D*/ case 0x14:
		//{
		//	break;
		//}
		///*INC E*/ case 0x1C:
		//{
		//	break;
		//}
		///*INC H*/ case 0x24:
		//{
		//	break;
		//}
		///*INC L*/ case 0x2C:
		//{
		//	break;
		//}
		///*INC (HL)*/ case 0x34:
		//{
		//	break;
		//}



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

			return 4;
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

			flags.N = 0x1;



			return 4;
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

			return 4;
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

			
			return 4;
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

			
			return 4;
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

			
			return 4;
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

			
			return 4;
		}
		///*DEC (HL)*/ case 0x35:
		//{
		//	//unsigned char res = registers.C - 1;
		//	//unsigned char tmp = registers.C;



		//	//if (tmp - 1 == 0x0)
		//	//	flags.Z = 0x1;
		//	//else flags.Z = 0x0;

		//	//flags.H = (((registers.C) ^ (1) ^ res) & 0x10) >> 4;
		//	//tmp--;
		//	//registers.C = tmp;
		//	//flags.N = 0x1;

		//	
		//	break;
		//}



		///*ADD HL,BC*/ case 0x09:
		//{
		//	break;
		//}
		///*ADD HL,DE*/ case 0x19:
		//{
		//	break;
		//}
		///*ADD HL,HL*/ case 0x29:
		//{
		//	break;
		//}
		///*ADD HL,SP*/ case 0x39:
		//{
		//	break;
		//}


		///*ADD SP, #*/ case 0xE8:
		//{
		//	break;
		//}



		///*INC BC*/ case 0x03:
		//{
		//	break;
		//}
		///*INC DE*/ case 0x13:
		//{
		//	break;
		//}
		///*INC HL*/ case 0x23:
		//{
		//	break;
		//}
		///*INC SP*/ case 0x33:
		//{
		//	break;
		//}



		///*DEC BC*/ case 0x0B:
		//{
		//	break;
		//}
		///*DEC DE*/ case 0x1B:
		//{
		//	break;
		//}
		///*DEC HL*/ case 0x2B:
		//{
		//	break;
		//}
		///*DEC SP*/ case 0x3B:
		//{
		//	break;
		//}




		///*SWAP A*/ case 0xCB37:
		//{
		//	break;
		//}
		///*SWAP B*/ case 0xCB30:
		//{
		//	break;
		//}
		///*SWAP C*/ case 0xCB31:
		//{
		//	break;
		//}
		///*SWAP D*/ case 0xCB32:
		//{
		//	break;
		//}
		///*SWAP E*/ case 0xCB33:
		//{
		//	break;
		//}
		///*SWAP H*/ case 0xCB34:
		//{
		//	break;
		//}
		///*SWAP L*/ case 0xCB35:
		//{
		//	break;
		//}
		///*SWAP (HL)*/ case 0xCB36:
		//{
		//	break;
		//}



		///*DAA*/ case 0x27:
		//{
		//	break;
		//}

		///*CPL*/ case 0x2F:
		//{
		//	break;
		//}


		///*CCF*/ case 0x3F:
		//{
		//	break;
		//}

		///*SCF*/ case 0x37:
		//{
		//	break;
		//}

		///*HALT*/ case 0x76:
		//{
		//	break;
		//}

		///*STOP*/ case 0x1000:
		//{


		//	break;
		//}


		///*DI*/ case 0xF3:
		//{
		//	imeFlag = 0x00;
		//	break;
		//}



		///*EI*/ case 0xFB:
		//{
		//	break;
		//}

		///*RLCA*/ case 0x07:
		//{
		//	break;
		//}

		///*RLA*/ case 0x17:
		//{
		//	break;
		//}

		///*RRCA*/ case 0x0F:
		//{
		//	break;
		//}

		///*RRA*/ case 0x1F:
		//{
		//	break;
		//}

		///*RLC A*/ case 0xCB07:
		//{
		//	break;
		//}
		///*RLC B*/ case 0xCB00:
		//{
		//	break;
		//}
		///*RLC C*/ case 0xCB01:
		//{
		//	break;
		//}
		///*RLC D*/ case 0xCB02:
		//{
		//	break;
		//}
		///*RLC E*/ case 0xCB03:
		//{
		//	break;
		//}
		///*RLC H*/ case 0xCB04:
		//{
		//	break;
		//}
		///*RLC L*/ case 0xCB05:
		//{
		//	break;
		//}
		///*RLC (HL)*/ case 0xCB06:
		//{
		//	break;
		//}


		///*RL A*/ case 0xCB17:
		//{
		//	break;
		//}
		///*RL B*/ case 0xCB10:
		//{
		//	break;
		//}
		///*RL C*/ case 0xCB11:
		//{
		//	break;
		//}
		///*RL D*/ case 0xCB12:
		//{
		//	break;
		//}
		///*RL E*/ case 0xCB13:
		//{
		//	break;
		//}
		///*RL H*/ case 0xCB14:
		//{
		//	break;
		//}
		///*RL L*/ case 0xCB15:
		//{
		//	break;
		//}
		///*RL (HL)*/ case 0xCB16:
		//{
		//	break;
		//}



		///*RRC A*/ case 0xCB0F:
		//{
		//	break;
		//}
		///*RRC B*/ case 0xCB08:
		//{
		//	break;
		//}
		///*RRC C*/ case 0xCB09:
		//{
		//	break;
		//}
		///*RRC D*/ case 0xCB0A:
		//{
		//	break;
		//}
		///*RRC E*/ case 0xCB0B:
		//{
		//	break;
		//}
		///*RRC H*/ case 0xCB0C:
		//{
		//	break;
		//}
		///*RRC L*/ case 0xCB0D:
		//{
		//	break;
		//}
		///*RRC (HL)*/ case 0xCB0E:
		//{
		//	break;
		//}

		///*RR A */case 0xCB1F:
		//{
		//	break;
		//}
		///*RR B */case 0xCB18:
		//{
		//	break;
		//}
		///*RR C */case 0xCB19:
		//{
		//	break;
		//}
		///*RR D */case 0xCB1A:
		//{
		//	break;
		//}
		///*RR E */case 0xCB1B:
		//{
		//	break;
		//}
		///*RR H */case 0xCB1C:
		//{
		//	break;
		//}
		///*RR L */case 0xCB1D:
		//{
		//	break;
		//}
		///*RR (HL)*/case 0xCB1E:
		//{
		//	break;
		//}

		///*SLA A*/ case 0xCB27:
		//{
		//	break;
		//}
		///*SLA B*/ case 0xCB20:
		//{
		//	break;
		//}
		///*SLA C*/ case 0xCB21:
		//{
		//	break;
		//}
		///*SLA D*/ case 0xCB22:
		//{
		//	break;
		//}
		///*SLA E*/ case 0xCB23:
		//{
		//	break;
		//}
		///*SLA H*/ case 0xCB24:
		//{
		//	break;
		//}
		///*SLA L*/ case 0xCB25:
		//{
		//	break;
		//}
		///*SLA (HL)*/ case 0xCB26:
		//{
		//	break;
		//}

		///*SRA A*/ case 0xCB2F:
		//{
		//	break;
		//}
		///*SRA B*/ case 0xCB28:
		//{
		//	break;
		//}
		///*SRA C*/ case 0xCB29:
		//{
		//	break;
		//}
		///*SRA D*/ case 0xCB2A:
		//{
		//	break;
		//}
		///*SRA E*/ case 0xCB2B:
		//{
		//	break;
		//}
		///*SRA H*/ case 0xCB2C:
		//{
		//	break;
		//}
		///*SRA L*/ case 0xCB2D:
		//{
		//	break;
		//}
		///*SRA (HL)*/ case 0xCB2E:
		//{
		//	break;
		//}

		///*SRL A*/ case 0xCB3F:
		//{
		//	break;
		//}
		///*SRL B*/ case 0xCB38:
		//{
		//	break;
		//}
		///*SRL C*/ case 0xCB39:
		//{
		//	break;
		//}
		///*SRL D*/ case 0xCB3A:
		//{
		//	break;
		//}
		///*SRL E*/ case 0xCB3B:
		//{
		//	break;
		//}
		///*SRL H*/ case 0xCB3C:
		//{
		//	break;
		//}
		///*SRL L*/ case 0xCB3D:
		//{
		//	break;
		//}
		///*SRL (HL)*/ case 0xCB3E:
		//{
		//	break;
		//}


		///*BIT b,A*/ case 0xCB47:
		//{
		//	break;
		//}
		///*BIT b,B*/ case 0xCB40:
		//{
		//	break;
		//}
		///*BIT b,C*/ case 0xCB41:
		//{
		//	break;
		//}
		///*BIT b,D*/ case 0xCB42:
		//{
		//	break;
		//}
		///*BIT b,E*/ case 0xCB43:
		//{
		//	break;
		//}
		///*BIT b,H*/ case 0xCB44:
		//{
		//	break;
		//}
		///*BIT b,L*/ case 0xCB45:
		//{
		//	break;
		//}
		///*BIT b,(HL)*/ case 0xCB46:
		//{
		//	break;
		//}


		///*SET b,A*/ case 0xCBC7:
		//{
		//	break;
		//}
		///*SET b,B*/ case 0xCBC0:
		//{
		//	break;
		//}
		///*SET b,C*/ case 0xCBC1:
		//{
		//	break;
		//}
		///*SET b,D*/ case 0xCBC2:
		//{
		//	break;
		//}
		///*SET b,E*/ case 0xCBC3:
		//{
		//	break;
		//}
		///*SET b,H*/ case 0xCBC4:
		//{
		//	break;
		//}
		///*SET b,L*/ case 0xCBC5:
		//{
		//	break;
		//}
		///*SET b,(HL)*/ case 0xCBC6:
		//{
		//	break;
		//}

		///*RES b,A*/ case 0xCB87:
		//{
		//	break;
		//}
		///*RES b,B*/ case 0xCB80:
		//{
		//	break;
		//}
		///*RES b,C*/ case 0xCB81:
		//{
		//	break;
		//}
		///*RES b,D*/ case 0xCB82:
		//{
		//	break;
		//}
		///*RES b,E*/ case 0xCB83:
		//{
		//	break;
		//}
		///*RES b,H*/ case 0xCB84:
		//{
		//	break;
		//}
		///*RES b,L*/ case 0xCB85:
		//{
		//	break;
		//}
		///*RES b,(HL)*/ case 0xCB86:
		//{
		//	break;
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
		//	break;
		//}
		///*JP Z,nn*/ case 0xCA:
		//{
		//	break;
		//}
		///*JP NC,nn*/ case 0xD2:
		//{
		//	break;
		//}
		///*JP C,nn*/ case 0xDA:
		//{
		//	break;
		//}

		///*JP (HL)*/ case 0xE9:
		//{
		//	break;
		//}
		///*JR n*/ case 0x18:
		//{
		//	break;
		//}

		/*JR NZ,**/ case 0x20:
		{
			if (flags.Z == 0x00)
			{
				//programCounter += (signed)ram[programCounter];
				programCounter += (signed char)readRam(programCounter);
				programCounter++;
			}
			else programCounter++;
			return 8;
		}
		///*JR Z,* */case 0x28:
		//{
		//	break;
		//}
		///*JR NC,**/ case 0x30:
		//{
		//	break;
		//}
		///*JR C,* */case 0x38:
		//{
		//	break;
		//}

		///*CALL nn*/ case 0xCD:
		//{
		//	break;
		//}

		///*CALL NZ,nn*/ case 0xC4:
		//{
		//	break;
		//}
		///*CALL Z,nn*/ case 0xCC:
		//{
		//	break;
		//}
		///*CALL NC,nn*/ case 0xD4:
		//{
		//	break;
		//}
		///*CALL C,nn*/ case 0xDC:
		//{
		//	break;
		//}

		///*RST 00H*/ case 0xC7:
		//{
		//	break;
		//}
		///*RST 08H*/ case 0xCF:
		//{
		//	break;
		//}
		///*RST 10H*/ case 0xD7:
		//{
		//	break;
		//}
		///*RST 18H*/ case 0xDF:
		//{
		//	break;
		//}
		///*RST 20H*/ case 0xE7:
		//{
		//	break;
		//}
		///*RST 28H*/ case 0xEF:
		//{
		//	break;
		//}
		///*RST 30H*/ case 0xF7:
		//{
		//	break;
		//}
		///*RST 38H*/ case 0xFF:
		//{
		//	break;
		//}

		///*RET -/-*/ case 0xC9:
		//{
		//	break;
		//}

		///*RET NZ*/ case 0xC0:
		//{
		//	break;
		//}
		///*RET Z*/ case 0xC8:
		//{
		//	break;
		//}
		///*RET NC */case 0xD0:
		//{
		//	break;
		//}
		///*RET C*/ case 0xD8:
		//{
		//	break;
		//}

		///*RETI -/-*/ case 0xD9:
		//{
		//	break;
		//}


		default:
		{
			std::cout << "Following opcode needs to be implemented: " << std::hex << (int)opcode << std::endl;
		};
	}

}