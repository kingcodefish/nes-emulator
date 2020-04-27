#include "Cartridge.h"
#include "Mapper.h"
#include "Mapper000.h"

namespace Cartridge
{
	Mapper* mapper = nullptr;

	void load(const char *filename)
	{
		FILE* f;
		fopen_s(&f, filename, "rb");

		// Find size of file in bytes and reset file pointer.
		fseek(f, 0, SEEK_END);
		int size = ftell(f);
		fseek(f, 0, SEEK_SET);

		// Allocate and store file into heap.
		uint8_t *rom = new uint8_t[size];
		fread(rom, size, 1, f);
		fclose(f);

		// Retrieve encoded mapper type from ROM
		int mapperNum = (rom[7] & 0xF0) | (rom[6] >> 4);
		if(loaded()) delete mapper;
		switch(mapperNum)
		{
		case 0:  mapper = new Mapper000(rom); break;
		/*case 1:  mapper = new Mapper001(rom); break;
		case 2:  mapper = new Mapper002(rom); break;
		case 3:  mapper = new Mapper003(rom); break;
		case 4:  mapper = new Mapper004(rom); break;*/
		}
	}

	bool loaded()
	{
		return mapper != nullptr;
	}
}
