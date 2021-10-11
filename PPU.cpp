#include "PPU.h"

namespace PPU
{
	uint8_t* registers; // Registers for status, etc.
	uint8_t* vram; // Video RAM
	uint8_t* oam; // Object Attribute Memory

	void initialize()
	{
		registers = new uint8_t[0x2000]; // why is this 2000?
		vram = new uint8_t[0x4000];
		oam = new uint8_t[0x256];
	}

	uint8_t readRegister(uint16_t addr)
	{
		return registers[(addr - 0x2000) % 8]; // Read from non-mirrored address.
	}

	void writeRegister(uint16_t addr, uint8_t value)
	{
		registers[(addr - 0x2000) % 8] = value; // Write to non-mirrored address.
	}

	uint8_t readRam(uint16_t addr)
	{
		if(addr < 0x3000 || addr >= 0x3F00 && addr < 0x3F20)
		{
			return vram[addr]; // All addressable locations
		}
		else if(addr >= 0x3000 && addr < 0x3F00)
		{
			return vram[addr - 0x1000]; // Read from non-mirrored address.
		}
		else if(addr >= 0x3F20 && addr < 0x4000)
		{
			return vram[(addr - 0x20) % 0x20]; // Read Palette RAM indexes every 0x20 increments.
		}
		else
		{
			throw "Could not read from PPU RAM at: " + addr;
		}
	}

	void writeRam(uint16_t addr, uint8_t value)
	{
		if(addr < 0x3000 || addr >= 0x3F00 && addr < 0x3F20)
		{
			vram[addr] = value; // All addressable locations
		}
		else if(addr >= 0x3000 && addr < 0x3F00)
		{
			vram[addr - 0x1000] = value; // Write to non-mirrored address.
		}
		else if(addr >= 0x3F20 && addr < 0x4000)
		{
			vram[(addr - 0x20) % 0x20] = value; // Write to Palette RAM indexes every 0x20 increments.
		}
		else
		{
			throw "Could not write to PPU RAM at: " + addr;
		}
	}

	/*
	* Copy the block pointed to by data into OAM.
	* This is the result of setting the DMA register at $4014.
	*/
	void dma(uint8_t* data)
	{
		memcpy(&oam, &data, 256); // Size of uint8 is implied.
	}

	void execute()
	{

	}
}