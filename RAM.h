#pragma once

#include <vector>
#include <array>

class RAM
{
private:
	std::array<uint8_t, 0xFFFF> data = {0};
public:
	RAM();
	~RAM();

	void write(uint16_t dest, uint8_t source)
	{
		if(dest > 0xFFFF || dest < 0x0000)
		{
			char hex_repr[4];
			//sprintf(hex_repr, "%04X", dest);
			throw std::out_of_range("RAM addressed out of bounds at address: " + dest);
		}

		data[dest] = source;
	}

	uint8_t read(uint16_t source)
	{
		if(source > 0xFFFF || source < 0x0000)
		{
			char hex_repr[4];
			//sprintf(hex_repr, "%04X", source);
			throw std::out_of_range("RAM addressed out of bounds at address: " + source);
		}

		return data[source];
	}
};

