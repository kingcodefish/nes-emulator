#pragma once

#include <iostream>

namespace APU
{
	void initialize();
	uint8_t readRegister(uint16_t addr);
	void writeRegister(uint16_t addr, uint8_t value);
}