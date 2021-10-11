#include "APU.h"

namespace APU
{
	uint8_t* registers;

	void initialize()
	{
		registers = new uint8_t[0x17];
	}

	uint8_t readRegister(uint16_t addr)
	{
		return registers[addr - 0x4000];
	}

	void writeRegister(uint16_t addr, uint8_t value)
	{
		registers[addr - 0x4000] = value;
	}
}