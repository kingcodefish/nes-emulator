#pragma once

#include <vector>
#include <fstream>
#include "Mapper.h"

namespace Cartridge
{
	extern Mapper* mapper;

	void load(const char *filename);
	bool loaded();
};

