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
	//prevent automatic flush of cout after every "\n" 
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
/*oid writeRam(unsigned short address, unsigned short data)
{

}*/

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
		//		herewasabreak;
		//	}
		//	case 2: //Bit 5-4
		//	{
		//		shade = (palette & 0x20) ? 1 : 0;
		//		shade <<= 1;
		//		shade |= (palette & 0x10) ? 1 : 0;
		//		herewasabreak;
		//	}
		//	case 1: //Bit 3-2
		//	{
		//		shade = (palette & 0x08) ? 1 : 0;
		//		shade <<= 1;
		//		shade |= (palette & 0x04) ? 1 : 0;
		//		herewasabreak;
		//	}
		//	case 0: //Bit 1-0
		//	{
		//		shade = (palette & 0x02) ? 1 : 0;
		//		shade <<= 1;
		//		shade |= (palette & 0x01) ? 1 : 0;
		//		herewasabreak;
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
					case 0: red = 255;  green = 255; blue = 255;   break;
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

//0xF0 (0x02B3 PC bei meinem) Befehl dort stehengeblieben. Er l�dt von FF00+44 den Wert 00, obwohl es 3E sein sollte. Anscheinend ist das ein LCD wert/register das immer wieder inkrementiert wird. 
//Dem nachgehen.
int executeOpcode(unsigned char opcode)
{
	std::cout << "Opcode: " << std::uppercase << std::hex << (opcode < 0x10 ? "0x0" : "0x") << (int)opcode << "\n";
	std::cout << "af: 0x" << std::uppercase << std::hex << +registers.A << +registers.F << "\n";
	std::cout << "bc: 0x" << std::uppercase << std::hex << +registers.B << +registers.C << "\n";
	std::cout << "de: 0x" << std::uppercase << std::hex << +registers.D << +registers.E << "\n";
	std::cout << "hl: 0x" << std::uppercase << std::hex << +registers.H << +registers.L << "\n";
	std::cout << "sp: 0x" << std::uppercase << std::hex << +stackPointer << "\n";
	std::cout << "pc: 0x" << std::uppercase << std::hex << +programCounter << "\n\n";
	//bc: 0xed10 opcode:0x20 (danach ff44 ++)

	//0x0D first instruction after first loop 
		//TODO: 0x36 implementierenF
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
		//	herewasabreak;

		//}
		///*LD E, n*/
		//case 0x1E:
		//{
		//	herewasabreak;

		//}
		///*LD H, n*/
		//case 0x26:
		//{
		//	herewasabreak;
		//}
		///*LD L, n*/
		//case 0x2E:
		//{
		//	herewasabreak;
		//}


		///*LD r1, r2*/
		///*LD A,A*/ case 0x7F:
		//{
		//	herewasabreak;
		//}
		///*LD A,B*/ case 0x78:
		//{
		//	herewasabreak;
		//}
		///*LD A,C*/ case 0x79:
		//{
		//	herewasabreak;
		//}
		///*LD A,D*/ case 0x7A:
		//{
		//	herewasabreak;
		//}
		///*LD A,E*/ case 0x7B:
		//{
		//	herewasabreak;
		//}
		///*LD A,H*/ case 0x7C:
		//{
		//	herewasabreak;
		//}
		///*LD A,L*/ case 0x7D:
		//{
		//	herewasabreak;
		//}
		///*LD A,(HL)*/ case 0x7E:
		//{
		//	herewasabreak;
		//}
		///*LD B,B*/ case 0x40:
		//{
		//	herewasabreak;
		//}
		///*LD B,C*/ case 0x41:
		//{
		//	herewasabreak;
		//}
		///*LD B,D*/ case 0x42:
		//{
		//	herewasabreak;
		//}
		///*LD B,E*/ case 0x43:
		//{
		//	herewasabreak;
		//}
		///*LD B,H*/ case 0x44:
		//{
		//	herewasabreak;
		//}
		///*LD B,L*/ case 0x45:
		//{
		//	herewasabreak;
		//}
		///*LD B,(HL)*/ case 0x46:
		//{
		//	herewasabreak;
		//}
		///*LD C,B*/ case 0x48:
		//{
		//	herewasabreak;
		//}
		///*LD C,C*/ case 0x49:
		//{
		//	herewasabreak;
		//}
		///*LD C,D*/ case 0x4A:
		//{
		//	herewasabreak;
		//}
		///*LD C,E*/ case 0x4B:
		//{
		//	herewasabreak;
		//}
		///*LD C,H*/ case 0x4C:
		//{
		//	herewasabreak;
		//}
		///*LD C,L*/ case 0x4D:
		//{
		//	herewasabreak;
		//}
		///*LD C,(HL)*/ case 0x4E:
		//{
		//	herewasabreak;
		//}
		///*LD D,B*/ case 0x50:
		//{
		//	herewasabreak;
		//}
		///*LD D,C*/ case 0x51:
		//{
		//	herewasabreak;
		//}

		//{
		//	herewasabreak;
		//}
		///*LD D,D*/ case 0x52:
		//{
		//	herewasabreak;
		//}
		///*LD D,E*/ case 0x53:
		//{
		//	herewasabreak;
		//}
		///*LD D,H*/ case 0x54:
		//{
		//	herewasabreak;
		//}
		///*LD D,L*/ case 0x55:
		//{
		//	herewasabreak;
		//}
		///*LD D,(HL)*/ case 0x56:
		//{
		//	herewasabreak;
		//}
		///*LD E,B*/ case 0x58:
		//{
		//	herewasabreak;
		//}
		///*LD E,C*/ case 0x59:
		//{
		//	herewasabreak;
		//}
		///*LD E,D*/ case 0x5A:
		//{
		//	herewasabreak;
		//}
		///*LD E,E*/ case 0x5B:
		//{
		//	herewasabreak;
		//}
		///*LD E,H*/ case 0x5C:
		//{
		//	herewasabreak;
		//}
		///*LD E,L*/ case 0x5D:
		//{
		//	herewasabreak;
		//}
		///*LD E,(HL)*/ case 0x5E:
		//{
		//	herewasabreak;
		//}
		///*LD H,B*/ case 0x60:
		//{
		//	herewasabreak;
		//}
		///*LD H,C*/ case 0x61:
		//{
		//	herewasabreak;
		//}
		///*LD H,D*/ case 0x62:
		//{
		//	herewasabreak;
		//}
		///*LD H,E*/ case 0x63:
		//{
		//	herewasabreak;
		//}
		///*LD H,H*/ case 0x64:
		//{
		//	herewasabreak;
		//}
		///*LD H,L*/ case 0x65:
		//{
		//	herewasabreak;
		//}
		///*LD H,(HL)*/ case 0x66:
		//{
		//	herewasabreak;
		//}
		///*LD L,B*/ case 0x68:
		//{
		//	herewasabreak;
		//}
		///*LD L,C*/ case 0x69:
		//{
		//	herewasabreak;
		//}
		///*LD L,D*/ case 0x6A:
		//{
		//	herewasabreak;
		//}
		///*LD L,E*/ case 0x6B:
		//{
		//	herewasabreak;
		//}
		///*LD L,H*/ case 0x6C:
		//{
		//	herewasabreak;
		//}
		///*LD L,L*/ case 0x6D:
		//{
		//	herewasabreak;
		//}
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

		///*LD A,(nn)*/ case 0xFA:
		//{
		//	
		//	herewasabreak;
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
		//	herewasabreak;
		//}
		///*LD C,A*/ case 0x4F:
		//{
		//	herewasabreak;
		//}
		///*LD D,A*/ case 0x57:
		//{
		//	herewasabreak;
		//}
		///*LD E,A*/ case 0x5F:
		//{
		//	herewasabreak;
		//}
		///*LD H,A*/ case 0x67:
		//{
		//	herewasabreak;
		//}
		///*LD L,A*/ case 0x6F:
		//{
		//	herewasabreak;
		//}
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

		///*LD (C), A*/ case 0xE2:
		//{
		//	herewasabreak;
		//}

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
			programCounter++;
			std::cout.flush();
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
		///*LD BC,nn*/ case 0x01: {
		//	herewasabreak;
		//}
		///*LD DE,nn*/ case 0x11: {
		//	herewasabreak;
		//}
		/*LD HL,nn*/ case 0x21: {
			//registers.L = ram[programCounter];
			//registers.H = ram[++programCounter];
			registers.L = readRam(programCounter);
			registers.H = readRam(++programCounter);
			programCounter++;
			return 12;
		}
		/*LD SP,nn*/ case 0x31: {
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


		///*PUSH AF*/ case 0xF5: {
		//	herewasabreak;

		//}


		///*PUSH BC*/ case 0xC5: {
		//	herewasabreak;

		//}


		///*PUSH DE*/ case 0xD5: {
		//	herewasabreak;

		//}


		///*PUSH HL*/ case 0xE5: {
		//	herewasabreak;

		//}



		///*POP AF*/ case 0xF1: {

		//	herewasabreak;

		//}


		///*POP BC*/ case 0xC1: {
		//	herewasabreak;

		//}


		///*POP DE*/ case 0xD1: {
		//	herewasabreak;

		//}


		///*POP HL*/ case 0xE1: {
		//	herewasabreak;

		//}


		///*ADD A,A*/ case 0x87:
		//{
		//	herewasabreak;
		//}
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



		///*AND A*/ case 0xA7:
		//{
		//	herewasabreak;
		//}
		///*AND B*/ case 0xA0:
		//{
		//	herewasabreak;
		//}
		///*AND C*/ case 0xA1:
		//{
		//	herewasabreak;
		//}
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
		///*AND #*/ case 0xE6:
		//{
		//	herewasabreak;
		//}



		///*OR A*/ case 0xB7:
		//{
		//	herewasabreak;
		//}
		///*OR B*/ case 0xB0:
		//{
		//	herewasabreak;
		//}
		///*OR C*/ case 0xB1:
		//{
		//	herewasabreak;
		//}
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
				flags.Z = 0x1;

			
			flags.N = 0x0;
			flags.C = 0x0;
			flags.H = 0x0;
			return 4;
		}
		///*XOR B*/ case 0xA8:
		//{
		//	herewasabreak;
		//}
		///*XOR C*/ case 0xA9:
		//{
		//	herewasabreak;
		//}
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
			programCounter++;

			return 8;
		}


		///*INC A*/ case 0x3C:
		//{
		//	herewasabreak;
		//}
		///*INC B*/ case 0x04:
		//{
		//	herewasabreak;
		//}
		///*INC C*/ case 0x0C:
		//{
		//	herewasabreak;
		//}
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
		///*INC (HL)*/ case 0x34:
		//{
		//	herewasabreak;
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
		//	herewasabreak;
		//}



		///*ADD HL,BC*/ case 0x09:
		//{
		//	herewasabreak;
		//}
		///*ADD HL,DE*/ case 0x19:
		//{
		//	herewasabreak;
		//}
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



		///*DEC BC*/ case 0x0B:
		//{
		//	herewasabreak;
		//}
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

		///*CPL*/ case 0x2F:
		//{
		//	herewasabreak;
		//}


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
			imeFlag = 0x00;
			return 4;
		}



		///*EI*/ case 0xFB:
		//{
		//	herewasabreak;
		//}

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
		//	herewasabreak;
		//}
		///*JR NC,**/ case 0x30:
		//{
		//	herewasabreak;
		//}
		///*JR C,* */case 0x38:
		//{
		//	herewasabreak;
		//}

		///*CALL nn*/ case 0xCD:
		//{
		//	herewasabreak;
		//}

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
		///*RST 28H*/ case 0xEF:
		//{
		//	herewasabreak;
		//}
		///*RST 30H*/ case 0xF7:
		//{
		//	herewasabreak;
		//}
		///*RST 38H*/ case 0xFF:
		//{
		//	herewasabreak;
		//}

		///*RET -/-*/ case 0xC9:
		//{
		//	herewasabreak;
		//}

		///*RET NZ*/ case 0xC0:
		//{
		//	herewasabreak;
		//}
		///*RET Z*/ case 0xC8:
		//{
		//	herewasabreak;
		//}
		///*RET NC */case 0xD0:
		//{
		//	herewasabreak;
		//}
		///*RET C*/ case 0xD8:
		//{
		//	herewasabreak;
		//}

		///*RETI -/-*/ case 0xD9:
		//{
		//	herewasabreak;
		//}


		default:
		{
			std::cout << "Following opcode needs to be implemented: " << std::hex << +opcode << std::endl;
		};
	}

}