#include "PPU.h"

#include <iostream>

namespace PPU
{
	uint8_t* registers;
	uint8_t* ram;

	void initialize()
	{
		registers = new uint8_t[0x2000];
		ram = new uint8_t[0x4000];
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
			return ram[addr]; // All addressable locations
		}
		else if(addr >= 0x3000 && addr < 0x3F00)
		{
			return ram[addr - 0x1000]; // Read from non-mirrored address.
		}
		else if(addr >= 0x3F20 && addr < 0x4000)
		{
			return ram[(addr - 0x20) % 0x20]; // Read Palette RAM indexes every 0x20 increments.
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
			ram[addr] = value; // All addressable locations
		}
		else if(addr >= 0x3000 && addr < 0x3F00)
		{
			ram[addr - 0x1000] = value; // Write to non-mirrored address.
		}
		else if(addr >= 0x3F20 && addr < 0x4000)
		{
			ram[(addr - 0x20) % 0x20] = value; // Write to  Palette RAM indexes every 0x20 increments.
		}
		else
		{
			throw "Could not write to PPU RAM at: " + addr;
		}
	}
}