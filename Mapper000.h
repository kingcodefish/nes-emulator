#pragma once
#include "Mapper.h"

class Mapper000 : public Mapper
{
public:
	Mapper000(uint8_t *rom) : Mapper(rom)
	{
		map_prg<32>(0, 0);
		map_chr<4>(0, 0);
	}
};