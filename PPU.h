#pragma once

#include <iostream>

namespace PPU
{
	void initialize();
	uint8_t readRegister(uint16_t addr);
	void writeRegister(uint16_t addr, uint8_t value);
	uint8_t readRam(uint16_t addr);
	void writeRam(uint16_t addr, uint8_t value);
	void dma(uint8_t* data);
	void execute();
}