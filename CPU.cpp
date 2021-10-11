#include "CPU.h"
#include "Cartridge.h"
#include "PPU.h"
#include "APU.h"

#include <iostream>
#include <string>

/*
* CPU Performance Rundown:
* - CPU is powered on, setting the PC to $8000 and SP to $00 on boot.
* - CPU does initial setup, initializing RAM, Accumulator, X-Reg, and Y-Reg.
* - CPU jumps to subroutine located at the Reset Interrupt registers and executes until RTI is reached.
* - CPU jumps back to the ROM start location from the RTI command and begins executing instructions procedurally.
* - A descending stack is used for the stack pointer.
*
* When interrupt occurs:
* - Recognize interrupt request has occurred.
* - Complete execution of the current instruction.
* - Push the program counter and status register on to the stack.
* - Set the interrupt disable flag to prevent further interrupts.
* - Load the address of the interrupt handling routine from the vector table into the program counter.
* - Execute the interrupt handling routine.
* - After  executing  a  RTI  (Return  From  Interrupt)  instruction,
*      pull  the  program  counter  and  status register values from the stack.
* - Resume execution of the program.
*/
/*
* Possible Bugs:
* - Addressing modes haven't been fully tested.
* - Negative flags are set to match 8th bit of opcode result, not only if they are equal to 1.
*   This shouldn't make a difference though.
* - Jump Indirect needs to be tested.
*/
namespace CPU
{
	uint16_t PC = 0x8000; // Program Counter
	uint8_t SP = 0x00; // Stack Pointer
	struct status {
		uint8_t $carry     : 1,
						$zero      : 1,
						$interrupt : 1,
						$decimal   : 1,
						$break     : 1,
						$overflow  : 1,
						$negative  : 1;
	} status;

	uint8_t* ram;
	uint8_t accum;
	uint8_t x_reg;
	uint8_t y_reg;

	/*
	* Read a byte from address in RAM.
	*/
	uint8_t read(uint16_t addr)
	{
		if (addr < 0x2000) // Addressing RAM
		{
			return ram[addr % 0x800]; // Read from non-mirrored address
		}
		else if (addr < 0x4000) // Addressing PPU registers
		{
			return PPU::readRegister(addr);
		}
		else if (addr < 0x4018)
		{
			return APU::readRegister(addr);
		}
		else if (addr >= 0x8000) // Addressing PRG-ROM
		{
			return Cartridge::mapper->read(addr);
		}
		else
		{
			throw "Could not read from CPU RAM at address: " + addr;
		}
	}

	/*
	* Write a byte to address in RAM.
	*/
	void write(uint16_t addr, uint8_t value)
	{
		if (addr < 0x2000) // Addressing RAM
		{
			ram[addr % 0x800] = value; // Write to non-mirrored address
		}
		else if (addr < 0x4000) // Addressing PPU registers
		{
			PPU::writeRegister(addr, value);
		}
		else if (addr < 0x4018) // Addressing APU registers
		{
			APU::writeRegister(addr, value);
		}
		else
		{
			throw "Could not write to CPU RAM at address: " + addr;
		}
	}

	/*
	* Push to Stack a 16 or 8-bit value.
	* 
	* NOTE: When the stack is full, the stack pointer wraps back around because unsigned. ;)
	*/
	template<typename bitWidth>
	void stackPush(bitWidth value)
	{
		if (sizeof(bitWidth) == sizeof(uint16_t))
		{
			write(0x0100 + --SP, value >> 8);
			write(0x0100 + --SP, value & 0xFF);
		}
		else
		{
			write(0x0100 + --SP, value);
		}
	}

	/*
	* Pull from Stack a 16 or 8-bit value.
	* 
	* We maintain the stack pointer one past the top value.
	*/
	template<typename bitWidth>
	bitWidth stackPull()
	{
		bitWidth value = 0;

		if (sizeof(bitWidth) == sizeof(uint16_t))
		{
			value = read(0x100 + SP);
			value += read(0x100 + SP + 1) << 8;
			write(0x0100 + SP++, 0);
			write(0x0100 + SP++, 0);
		}
		else
		{
			value = read(0x100 + SP);
			write(0x0100 + SP++, 0);
		}

		return value;
	}

	/*
	* Add With Carry
	*
	* Notes:
	* - NES has no BCD.
	*/
	template<addressing_mode_e MODE>
	void ADC()
	{
		uint16_t addr;
		uint8_t value;
		uint8_t initial = accum;
		uint8_t result;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			break;
		case ABSIY:
			addr = read(++PC);
			addr += read(++PC) << 8 + y_reg;
			break;
		case INDIN:
			addr = read((read(++PC) + x_reg) % 0xFF) + (read((read(PC) + x_reg + 1) % 0xFF) << 8);
			break;
		case ININD:
			addr = read(read(++PC)) + (read(read(PC) + 1) << 8) + y_reg;
			break;
		}
		value = read(addr);
		result = accum + value + status.$carry;
		accum = result;

		// Carry Flag
		status.$carry = (initial + value + status.$carry) >> 8;
		
		// Zero Flag
		if(result == 0)
		{
			status.$zero = 1;
		}

		// Overflow Flag
		if(initial > 0x7F && result <= 0x7F
			|| initial <= 0x7F && result > 0x7F)
		{
			status.$overflow = 1;
		}
		else
		{
			status.$overflow = 0;
		}

		// Negative Flag
		status.$negative = result >> 7;
	}

	/*
	* Bitwise AND with Accumulator
	*/
	template<addressing_mode_e MODE>
	void AND()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			break;
		case ABSIY:
			addr = read(++PC);
			addr += read(++PC) << 8 + y_reg;
			break;
		case INDIN:
			addr = read((read(++PC) + x_reg) % 0xFF) + (read((read(PC) + x_reg + 1) % 0xFF) << 8);
			break;
		case ININD:
			addr = read(read(++PC)) + (read(read(PC) + 1) << 8) + y_reg;
			break;
		}
		value = read(addr);
		accum &= value;

		// Zero Flag
		if(accum == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = accum >> 7;
	}
	
	/*
	* Arithmetic Shift Left
	*/
	template<addressing_mode_e MODE>
	void ASL()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case ACCUM:
			status.$carry = accum >> 7; // Set CPU status carry flag to leftmost bit in accumulator.
			accum <<= 1;
			break;
		case ZEROP:
			addr = read(++PC);
			value = read(addr);
			status.$carry = value >> 7;
			write(addr, value << 1);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			value = read(addr);
			status.$carry = value >> 7;
			write(addr, value << 1);
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			value = read(addr);
			status.$carry = value >> 7;
			write(addr, value << 1);
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			value = read(addr);
			status.$carry = value >> 7;
			write(addr, value << 1);
			break;
		}

		// Zero Flag
		if(MODE == ACCUM && accum == 0 || MODE != ACCUM && value == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = (MODE == ACCUM) ? (accum >> 7) : (value >> 7);
	}

	/*
	* Branch if Carry Clear
	*/
	template<addressing_mode_e MODE>
	void BCC()
	{
		int8_t value = static_cast<int8_t>(read(++PC)); // Convert unsigned relative value to signed.
		if(status.$carry == 0)
		{
			PC += value; // Important to note that effective address value will
									 // be one after this location because the PC is incremented to fetch next opcode.
		}
	}

	/*
	* Branch if Carry Set
	*/
	template<addressing_mode_e MODE>
	void BCS()
	{
		int8_t value = static_cast<int8_t>(read(++PC)); // Convert unsigned relative value to signed.
		if(status.$carry == 1)
		{
			PC += value; // Important to note that effective address value will
									 // be one after this location because the PC is incremented to fetch next opcode.
		}
	}

	/*
	* Branch if Equal
	*/
	template<addressing_mode_e MODE>
	void BEQ()
	{
		int8_t value = static_cast<int8_t>(read(++PC)); // Convert unsigned relative value to signed.
		if(status.$zero == 1)
		{
			PC += value; // Important to note that effective address value will
									 // be one after this location because the PC is incremented to fetch next opcode.
		}
	}

	/*
	* BIT Test
	*/
	template<addressing_mode_e MODE>
	void BIT()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case ZEROP:
			addr = read(++PC);
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		}
		value = read(addr);
		accum &= value;

		// Zero Flag
		if((accum & value) == 0)
		{
			status.$zero = 1;
		}

		// Overflow Flag
		status.$overflow = (value & 0x40) >> 6; // Set as bit 7 of memory value

		// Negative Flag
		status.$negative = value >> 7; // Set as bit 8 of memory value
	}

	/*
	* Branch if Minus
	*/
	template<addressing_mode_e MODE>
	void BMI()
	{
		int8_t value = static_cast<int8_t>(read(++PC)); // Convert unsigned relative value to signed.
		if(status.$negative == 1)
		{
			PC += value; // Important to note that effective address value will
									 // be one after this location because the PC is incremented to fetch next opcode.
		}
	}

	/*
	* Branch if Not Equal
	*/
	template<addressing_mode_e MODE>
	void BNE()
	{
		int8_t value = static_cast<int8_t>(read(++PC)); // Convert unsigned relative value to signed.
		if(status.$zero == 0)
		{
			PC += value; // Important to note that effective address value will
									 // be one after this location because the PC is incremented to fetch next opcode.
		}
	}

	/*
	* Branch if Positive
	*/
	template<addressing_mode_e MODE>
	void BPL()
	{
		int8_t value = static_cast<int8_t>(read(++PC)); // Convert unsigned relative value to signed.
		if(status.$negative == 0)
		{
			PC += value; // Important to note that effective address value will
									 // be one after this location because the PC is incremented to fetch next opcode.
		}
	}

	/*
	* Force Interrupt
	* - To be implemented.
	*/
	template<addressing_mode_e MODE>
	void BRK()
	{
		// The program counter and processor status are pushed on the stack then the
		// IRQ interrupt vector at $FFFE/F is loaded into the PC
		status.$break = 1;
	}

	/*
	* Branch if Overflow Clear
	*/
	template<addressing_mode_e MODE>
	void BVC()
	{
		int8_t value = static_cast<int8_t>(read(++PC)); // Convert unsigned relative value to signed.
		if(status.$overflow == 0)
		{
			PC += value; // Important to note that effective address value will
									 // be one after this location because the PC is incremented to fetch next opcode.
		}
	}

	/*
	* Branch if Overflow Set
	*/
	template<addressing_mode_e MODE>
	void BVS()
	{
		int8_t value = static_cast<int8_t>(read(++PC)); // Convert unsigned relative value to signed.
		if(status.$overflow == 1)
		{
			PC += value; // Important to note that effective address value will
									 // be one after this location because the PC is incremented to fetch next opcode.
		}
	}

	/*
	* Clear Carry Flag
	*/
	template<addressing_mode_e MODE>
	void CLC()
	{
		status.$carry = 0;
	}

	/*
	* Clear Decimal Mode
	*/
	template<addressing_mode_e MODE>
	void CLD()
	{
		status.$decimal = 0;
	}

	/*
	* Clear Interrupt Disable
	*/
	template<addressing_mode_e MODE>
	void CLI()
	{
		status.$interrupt = 0;
	}

	/*
	* Clear Overflow Flag
	*/
	template<addressing_mode_e MODE>
	void CLV()
	{
		status.$overflow = 0;
	}

	/*
	* Compare
	*/
	template<addressing_mode_e MODE>
	void CMP()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			break;
		case ABSIY:
			addr = read(++PC);
			addr += read(++PC) << 8 + y_reg;
			break;
		case INDIN:
			addr = read((read(++PC) + x_reg) % 0xFF) + (read((read(PC) + x_reg + 1) % 0xFF) << 8);
			break;
		case ININD:
			addr = read(read(++PC)) + (read(read(PC) + 1) << 8) + y_reg;
			break;
		}
		value = read(addr);

		// Carry Flag
		if(accum >= value)
		{
			status.$carry = 1;
		}
		
		// Zero Flag
		status.$zero = (accum == value) ? 1 : 0;

		// Negative Flag
		status.$negative = accum >> 7;
	}

	/*
	* Compare X-Register
	*/
	template<addressing_mode_e MODE>
	void CPX()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		}
		value = read(addr);

		// Carry Flag
		if(x_reg >= value)
		{
			status.$carry = 1;
		}

		// Zero Flag
		status.$zero = (x_reg == value) ? 1 : 0;

		// Negative Flag
		status.$negative = x_reg >> 7;
	}

	/*
	* Compare Y-Register
	*/
	template<addressing_mode_e MODE>
	void CPY()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		}
		value = read(addr);

		// Carry Flag
		if(y_reg >= value)
		{
			status.$carry = 1;
		}

		// Zero Flag
		status.$zero = (y_reg == value) ? 1 : 0;

		// Negative Flag
		status.$negative = y_reg >> 7;
	}

	/*
	* Decrement Memory
	*/
	template<addressing_mode_e MODE>
	void DEC()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			break;
		}
		value = read(addr) - 1;
		write(addr, value);

		// Zero Flag
		if(value == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = value >> 7;
	}

	/*
	* Decrement X-Register
	*/
	template<addressing_mode_e MODE>
	void DEX()
	{
		--x_reg;

		// Zero Flag
		if(x_reg == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = x_reg >> 7;
	}

	/*
	* Decrement Y-Register
	*/
	template<addressing_mode_e MODE>
	void DEY()
	{
		--y_reg;

		// Zero Flag
		if(y_reg == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = y_reg >> 7;
	}

	/*
	* Exclusive OR
	*/
	template<addressing_mode_e MODE>
	void EOR()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			break;
		case ABSIY:
			addr = read(++PC);
			addr += read(++PC) << 8 + y_reg;
			break;
		case INDIN:
			addr = read((read(++PC) + x_reg) % 0xFF) + (read((read(PC) + x_reg + 1) % 0xFF) << 8);
			break;
		case ININD:
			addr = read(read(++PC)) + (read(read(PC) + 1) << 8) + y_reg;
			break;
		}
		value = read(addr);
		accum ^= value;

		// Zero Flag
		if(accum == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = accum >> 7;
	}

	/*
	* Increment Memory
	*/
	template<addressing_mode_e MODE>
	void INC()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			break;
		}
		value = read(addr) + 1;
		write(addr, value);

		// Zero Flag
		if(value == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = value >> 7;
	}

	/*
	* Increment X-Register
	*/
	template<addressing_mode_e MODE>
	void INX()
	{
		++x_reg;

		// Zero Flag
		if(x_reg == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = x_reg >> 7;
	}

	/*
	* Increment Y-Register
	*/
	template<addressing_mode_e MODE>
	void INY()
	{
		++y_reg;

		// Zero Flag
		if(y_reg == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = y_reg >> 7;
	}

	/*
	* Jump
	* NOTE: An original 6502 has does not correctly fetch the target address if the indirect
	* vector falls on a page boundary (e.g. $xxFF where xx is any value from $00 to $FF).
	* In this case fetches the LSB from $xxFF as expected but takes the MSB from $xx00.
	* This is fixed in some later chips like the 65SC02 so for compatibility always ensure the
	* indirect vector is not at the end of the page.
	*/
	template<addressing_mode_e MODE>
	void JMP()
	{
		uint16_t addr;

		switch(MODE)
		{
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case INDIA:
			addr = read(read(++PC) + (read(++PC) << 8)) + read(read(++PC) + (read(++PC) << 8) + 1);
			break;
		}
		PC = addr - 1; // Branch to address directly before subroutine because PC is incremented on next cycle.
	}

	/*
	* Jump to Subroutine
	*/
	template<addressing_mode_e MODE>
	void JSR()
	{
		uint16_t addr = read(++PC) + (read(++PC) << 8);
		
		// Push to Stack
		stackPush<uint16_t>(PC); // PC - 1 is what it should do, but this might work better....??

		PC = addr - 1; // Branch to address directly before subroutine because PC is incremented on next cycle.
	}

	/*
	* Load Accumulator
	*/
	template<addressing_mode_e MODE>
	void LDA()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			break;
		case ABSIY:
			addr = read(++PC);
			addr += read(++PC) << 8 + y_reg;
			break;
		case INDIN:
			addr = read((read(++PC) + x_reg) % 0xFF) + (read((read(PC) + x_reg + 1) % 0xFF) << 8);
			break;
		case ININD:
			addr = read(read(++PC)) + (read(read(PC) + 1) << 8) + y_reg;
			break;
		}
		value = read(addr);
		accum = value;

		// Zero Flag
		if(accum == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		if(accum >> 7 == 1)
		{
			status.$negative = 1;
		}
	}

	/*
	* Load X-Register
	*/
	template<addressing_mode_e MODE>
	void LDX()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIY:
			addr = (read(++PC) + y_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIY:
			addr = read(++PC);
			addr += read(++PC) << 8 + y_reg;
			break;
		}
		value = read(addr);
		x_reg = value;

		// Zero Flag
		if(x_reg == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		if(x_reg >> 7 == 1)
		{
			status.$negative = 1;
		}
	}

	/*
	* Load Y-Register
	*/
	template<addressing_mode_e MODE>
	void LDY()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			break;
		}
		value = read(addr);
		y_reg = value;

		// Zero Flag
		if(y_reg == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		if(y_reg >> 7 == 1)
		{
			status.$negative = 1;
		}
	}

	/*
	* Logical Shift Right
	*/
	template<addressing_mode_e MODE>
	void LSR()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case ACCUM:
			status.$carry = accum & 1; // Set CPU status carry flag to rightmost bit in accumulator.
			accum >>= 1;
			break;
		case ZEROP:
			addr = read(++PC);
			value = read(addr);
			status.$carry = value & 1;
			write(addr, value >> 1);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			value = read(addr);
			status.$carry = value & 1;
			write(addr, value >> 1);
			break;
		case ABSOL:
			addr = read(++PC) + (read(++PC) << 8);
			value = read(addr);
			status.$carry = value & 1;
			write(addr, value >> 1);
			break;
		case ABSIX:
			addr = read(++PC) + (read(++PC) << 8) + x_reg;
			value = read(addr);
			status.$carry = value & 1;
			write(addr, value >> 1);
			break;
		}

		// Zero Flag
		if(MODE == ACCUM && accum == 0 || MODE != ACCUM && value == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = (MODE == ACCUM) ? (accum >> 7) : (value >> 7); // Sort of doesn't make sense.
	}

	/*
	* Logical Inclusive OR <-- Odd Name, but Okay.
	*/
	template<addressing_mode_e MODE>
	void ORA()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC) + (read(++PC) << 8);
			break;
		case ABSIX:
			addr = read(++PC) + (read(++PC) << 8) + x_reg;
			break;
		case ABSIY:
			addr = read(++PC) + (read(++PC) << 8) + y_reg;
			break;
		case INDIN:
			addr = read((read(++PC) + x_reg) % 0xFF) + (read((read(PC) + x_reg + 1) % 0xFF) << 8);
			break;
		case ININD:
			addr = read(read(++PC)) + (read(read(PC) + 1) << 8) + y_reg;
			break;
		}
		value = read(addr);
		accum |= value;

		// Zero Flag
		if(accum == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = accum >> 7;
	}

	/*
	* Push Accumulator
	*/
	template<addressing_mode_e MODE>
	void PHA()
	{
		stackPush<uint8_t>(accum);
	}

	/*
	* Push Processor Status
	*/
	template<addressing_mode_e MODE>
	void PHP()
	{
		stackPush<uint8_t>(status.$negative << 7 |
			status.$overflow << 6 |
			status.$break << 4 |
			status.$decimal << 3 |
			status.$interrupt << 2 |
			status.$zero << 1 |
			status.$carry);
	}

	/*
	* Pull Accumulator
	*/
	template<addressing_mode_e MODE>
	void PLA()
	{
		accum = stackPull<uint8_t>();
	}

	/*
	* Pull Processor Status
	*/
	template<addressing_mode_e MODE>
	void PLP()
	{
		uint8_t value = stackPull<uint8_t>();

		status.$negative = value >> 7;
		status.$overflow = (value >> 6) & 1;
		status.$break = (value >> 4) & 1;
		status.$decimal = (value >> 2) & 1;
		status.$interrupt = (value >> 3) & 1;
		status.$zero = (value >> 1) & 1;
		status.$carry = value & 1;
	}

	/*
	* Rotate Left
	*/
	template<addressing_mode_e MODE>
	void ROL()
	{
		uint16_t addr;
		uint8_t value;
		uint8_t temp; // Used to hold carry flag.

		switch(MODE)
		{
		case ACCUM:
			temp = status.$carry;
			status.$carry = accum >> 7; // Set CPU status carry flag to leftmost bit in accumulator.
			accum <<= 1;
			accum += temp;
			break;
		case ZEROP:
			addr = read(++PC);
			value = read(addr);
			temp = status.$carry;
			status.$carry = value >> 7;
			write(addr, (value << 1) + temp);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			value = read(addr);
			temp = status.$carry;
			status.$carry = value >> 7;
			write(addr, (value << 1) + temp);
			break;
		case ABSOL:
			addr = read(++PC) + (read(++PC) << 8);
			value = read(addr);
			temp = status.$carry;
			status.$carry = value >> 7;
			write(addr, (value << 1) + temp);
			break;
		case ABSIX:
			addr = read(++PC) + (read(++PC) << 8) + x_reg;
			value = read(addr);
			temp = status.$carry;
			status.$carry = value >> 7;
			write(addr, (value << 1) + temp);
			break;
		}

		// Zero Flag
		if(MODE == ACCUM && accum == 0 || MODE != ACCUM && read(addr) == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = (MODE == ACCUM) ? (accum >> 7) : (read(addr) >> 7);
	}

	/*
	* Rotate Right
	*/
	template<addressing_mode_e MODE>
	void ROR()
	{
		uint16_t addr;
		uint8_t value;
		uint8_t temp; // Used to hold carry flag.

		switch(MODE)
		{
		case ACCUM:
			temp = status.$carry;
			status.$carry = accum & 1; // Set CPU status carry flag to rightmost bit in accumulator.
			accum >>= 1;
			accum += temp << 7;
			break;
		case ZEROP:
			addr = read(++PC);
			value = read(addr);
			temp = status.$carry;
			status.$carry = accum & 1;
			write(addr, (value >> 1) + (temp << 7));
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			value = read(addr);
			temp = status.$carry;
			status.$carry = accum & 1;
			write(addr, (value >> 1) + (temp << 7));
			break;
		case ABSOL:
			addr = read(++PC) + (read(++PC) << 8);
			value = read(addr);
			temp = status.$carry;
			status.$carry = accum & 1;
			write(addr, (value >> 1) + (temp << 7));
			break;
		case ABSIX:
			addr = read(++PC) + (read(++PC) << 8) + x_reg;
			value = read(addr);
			temp = status.$carry;
			status.$carry = accum & 1;
			write(addr, (value >> 1) + (temp << 7));
			break;
		}

		// Zero Flag
		if(MODE == ACCUM && accum == 0 || MODE != ACCUM && read(addr) == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		status.$negative = (MODE == ACCUM) ? (accum >> 7) : (read(addr) >> 7);
	}

	/*
	* Return from Interrupt
	*/
	template<addressing_mode_e MODE>
	void RTI()
	{
		PLP<IMPLI>(); // Pull processor flags from stack...
		PC = stackPull<uint16_t>(); // ...followed by the program counter. PC - 1???
	}

	/*
	* Return from Subroutine
	*/
	template<addressing_mode_e MODE>
	void RTS()
	{
		PC = stackPull<uint16_t>(); // Pull the program counter. PC - 1???
	}

	/*
	* Subtract With Carry
	*
	* Notes:
	* - NES has no BCD.
	*/
	template<addressing_mode_e MODE>
	void SBC()
	{
		uint16_t addr;
		uint8_t value;
		uint8_t initial = accum;
		uint8_t result;

		switch(MODE)
		{
		case IMMED:
			addr = ++PC;
			break;
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC) + (read(++PC) << 8);
			break;
		case ABSIX:
			addr = read(++PC) + (read(++PC) << 8) + x_reg;
			break;
		case ABSIY:
			addr = read(++PC) + (read(++PC) << 8) + y_reg;
			break;
		case INDIN:
			addr = read((read(++PC) + x_reg) % 0xFF) + (read((read(PC) + x_reg + 1) % 0xFF) << 8);
			break;
		case ININD:
			addr = read(read(++PC)) + (read(read(PC) + 1) << 8) + y_reg;
			break;
		}
		value = read(addr) ^ 0xFF;
		result = accum + value + status.$carry;
		accum = result;

		// Carry Flag
		status.$carry = (initial + value) >> 8;

		// Zero Flag
		if(result == 0)
		{
			status.$zero = 1;
		}

		// Overflow Flag
		if(initial > 0x7F && result <= 0x7F
			|| initial <= 0x7F && result > 0x7F)
		{
			status.$overflow = 1;
		}
		else
		{
			status.$overflow = 0;
		}

		// Negative Flag
		status.$negative = result >> 7;
	}

	/*
	* Set Carry Flag
	*/
	template<addressing_mode_e MODE>
	void SEC()
	{
		status.$carry = 1;
	}

	/*
	* Set Decimal Flag
	*/
	template<addressing_mode_e MODE>
	void SED()
	{
		status.$decimal = 1;
	}

	/*
	* Set Interrupt Disable
	*/
	template<addressing_mode_e MODE>
	void SEI()
	{
		status.$interrupt = 1;
	}

	/*
	* Store Accumulator
	*/
	template<addressing_mode_e MODE>
	void STA()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		case ABSIX:
			addr = read(++PC);
			addr += read(++PC) << 8 + x_reg;
			break;
		case ABSIY:
			addr = read(++PC);
			addr += read(++PC) << 8 + y_reg;
			break;
		case INDIN:
			addr = read((read(++PC) + x_reg) % 0xFF) + (read((read(PC) + x_reg + 1) % 0xFF) << 8);
			break;
		case ININD:
			addr = read(read(++PC)) + (read(read(PC) + 1) << 8) + y_reg;
			break;
		}
		value = accum;
		write(addr, value);
	}

	/*
	* Store X-Register
	*/
	template<addressing_mode_e MODE>
	void STX()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIY:
			addr = (read(++PC) + y_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		}
		value = x_reg;
		write(addr, value);
	}

	/*
	* Store Y-Register
	*/
	template<addressing_mode_e MODE>
	void STY()
	{
		uint16_t addr;
		uint8_t value;

		switch(MODE)
		{
		case ZEROP:
			addr = read(++PC);
			break;
		case ZEPIX:
			addr = (read(++PC) + x_reg) % 0xFF;
			break;
		case ABSOL:
			addr = read(++PC);
			addr += read(++PC) << 8;
			break;
		}
		value = y_reg;
		write(addr, value);
	}

	/*
	* Transfer Accumulator to X-Register
	*/
	template<addressing_mode_e MODE>
	void TAX()
	{
		x_reg = accum;

		// Zero Flag
		if(x_reg == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		if(x_reg >> 7 == 1)
		{
			status.$negative = 1;
		}
	}

	/*
	* Transfer Accumulator to Y-Register
	*/
	template<addressing_mode_e MODE>
	void TAY()
	{
		y_reg = accum;

		// Zero Flag
		if(y_reg == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		if(y_reg >> 7 == 1)
		{
			status.$negative = 1;
		}
	}

	/*
	* Transfer Stack Pointer to X-Register
	*/
	template<addressing_mode_e MODE>
	void TSX()
	{
		x_reg = SP;

		// Zero Flag
		if(x_reg == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		if(x_reg >> 7 == 1)
		{
			status.$negative = 1;
		}
	}

	/*
	* Transfer X-Register to Accumulator
	*/
	template<addressing_mode_e MODE>
	void TXA()
	{
		accum = x_reg;

		// Zero Flag
		if(accum == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		if(accum >> 7 == 1)
		{
			status.$negative = 1;
		}
	}

	/*
	* Transfer X-Register tot Stack Pointer
	*/
	template<addressing_mode_e MODE>
	void TXS()
	{
		SP = x_reg;
	}

	/*
	* Transfer Y-Register to Accumulator
	*/
	template<addressing_mode_e MODE>
	void TYA()
	{
		accum = y_reg;

		// Zero Flag
		if(accum == 0)
		{
			status.$zero = 1;
		}

		// Negative Flag
		if(accum >> 7 == 1)
		{
			status.$negative = 1;
		}
	}

	/**
	* Execute instruction at program counter.
	*/
	void execute()
	{
		if(read(PC) == 0x69)
		{
			std::cout << "Instruction: Add With Carry (0x69 IMMED) with " << (int)read(PC+1);
		}
		else
		{
			std::cout << "Instruction: " << (int)read(PC);
		}
		std::cout << "\nProgram Counter: " << (int)PC
			<< "\nStack Pointer: " << (int)SP
			<< "\nAccumulator: " << (int)accum
			<< "\nX Register: " << (int)x_reg
			<< "\nY Register: " << (int)y_reg
			<< "\nCarry Flag: " << (int)status.$carry
			<< "\nZero Flag: " << (int)status.$zero
			<< "\nInterrupt Flag: " << (int)status.$interrupt
			<< "\nDecimal Flag: " << (int)status.$decimal
			<< "\nBreak Flag: " << (int)status.$break
			<< "\nOverflow Flag: " << (int)status.$overflow
			<< "\nNegative Flag: " << (int)status.$negative << "\n\n";

		switch(read(PC))
		{
			case 0x69: ADC<IMMED>(); break;
			case 0x65: ADC<ZEROP>(); break;
			case 0x75: ADC<ZEPIX>(); break;
			case 0x6D: ADC<ABSOL>(); break;
			case 0x7D: ADC<ABSIX>(); break;
			case 0x79: ADC<ABSIY>(); break;
			case 0x61: ADC<INDIN>(); break;
			case 0x71: ADC<ININD>(); break;
			
			case 0x29: AND<IMMED>(); break;
			case 0x25: AND<ZEROP>(); break;
			case 0x35: AND<ZEPIX>(); break;
			case 0x2D: AND<ABSOL>(); break;
			case 0x3D: AND<ABSIX>(); break;
			case 0x39: AND<ABSIY>(); break;
			case 0x21: AND<INDIN>(); break;
			case 0x31: AND<ININD>(); break;

			case 0x0A: ASL<ACCUM>(); break;
			case 0x06: ASL<ZEROP>(); break;
			case 0x16: ASL<ZEPIX>(); break;
			case 0x0E: ASL<ABSOL>(); break;
			case 0x1E: ASL<ABSIX>(); break;

			case 0x90: BCC<RELAT>(); break;

			case 0xB0: BCS<RELAT>(); break;
			
			case 0xF0: BEQ<RELAT>(); break;
			
			case 0x24: BIT<ZEROP>(); break;
			case 0x2C: BIT<ABSOL>(); break;
			
			case 0x30: BMI<RELAT>(); break;

			case 0xD0: BNE<RELAT>(); break;

			case 0x10: BPL<RELAT>(); break;
			
			case 0x00: BRK<IMPLI>(); break; // Finish implementation

			case 0x50: BVC<RELAT>(); break;

			case 0x70: BVS<RELAT>(); break;
			
			case 0x18: CLC<IMPLI>(); break;

			case 0xD8: CLD<IMPLI>(); break;

			case 0x58: CLI<IMPLI>(); break;

			case 0xB8: CLV<IMPLI>(); break;
			
			case 0xC9: CMP<IMMED>(); break;
			case 0xC5: CMP<ZEROP>(); break;
			case 0xD5: CMP<ZEPIX>(); break;
			case 0xCD: CMP<ABSOL>(); break;
			case 0xDD: CMP<ABSIX>(); break;
			case 0xD9: CMP<ABSIY>(); break;
			case 0xC1: CMP<INDIN>(); break;
			case 0xD1: CMP<ININD>(); break;
			
			case 0xE0: CPX<IMMED>(); break;
			case 0xE4: CPX<ZEROP>(); break;
			case 0xEC: CPX<ABSOL>(); break;

			case 0xC0: CPY<IMMED>(); break;
			case 0xC4: CPY<ZEROP>(); break;
			case 0xCC: CPY<ABSOL>(); break;

			case 0xC6: DEC<ZEROP>(); break;
			case 0xD6: DEC<ZEPIX>(); break;
			case 0xCE: DEC<ABSOL>(); break;
			case 0xDE: DEC<ABSIX>(); break;

			case 0xCA: DEX<IMPLI>(); break;

			case 0x88: DEY<IMPLI>(); break;

			case 0x49: EOR<IMMED>(); break;
			case 0x45: EOR<ZEROP>(); break;
			case 0x55: EOR<ZEPIX>(); break;
			case 0x4D: EOR<ABSOL>(); break;
			case 0x5D: EOR<ABSIX>(); break;
			case 0x59: EOR<ABSIY>(); break;
			case 0x41: EOR<INDIN>(); break;
			case 0x51: EOR<ININD>(); break;
			
			case 0xE6: INC<ZEROP>(); break;
			case 0xF6: INC<ZEPIX>(); break;
			case 0xEE: INC<ABSOL>(); break;
			case 0xFE: INC<ABSIX>(); break;

			case 0xE8: INX<IMPLI>(); break;

			case 0xC8: INY<IMPLI>(); break;
			
			case 0x4C: JMP<ABSOL>(); break;
			case 0x6C: JMP<INDIA>(); break;
			
			case 0x20: JSR<ABSOL>(); break;

			case 0xA9: LDA<IMMED>(); break;
			case 0xA5: LDA<ZEROP>(); break;
			case 0xB5: LDA<ZEPIX>(); break;
			case 0xAD: LDA<ABSOL>(); break;
			case 0xBD: LDA<ABSIX>(); break;
			case 0xB9: LDA<ABSIY>(); break;
			case 0xA1: LDA<INDIN>(); break;
			case 0xB1: LDA<ININD>(); break;

			case 0xA2: LDX<IMMED>(); break;
			case 0xA6: LDX<ZEROP>(); break;
			case 0xB6: LDX<ZEPIY>(); break;
			case 0xAE: LDX<ABSOL>(); break;
			case 0xBE: LDX<ABSIY>(); break;

			case 0xA0: LDY<IMMED>(); break;
			case 0xA4: LDY<ZEROP>(); break;
			case 0xB4: LDY<ZEPIX>(); break;
			case 0xAC: LDY<ABSOL>(); break;
			case 0xBC: LDY<ABSIX>(); break;

			case 0x4A: LSR<ACCUM>(); break;
			case 0x46: LSR<ZEROP>(); break;
			case 0x56: LSR<ZEPIX>(); break;
			case 0x4E: LSR<ABSOL>(); break;
			case 0x5E: LSR<ABSIX>(); break;
			
			case 0xEA: break; // NOP; Does nothing.
			
			case 0x09: ORA<IMMED>(); break;
			case 0x05: ORA<ZEROP>(); break;
			case 0x15: ORA<ZEPIX>(); break;
			case 0x0D: ORA<ABSOL>(); break;
			case 0x1D: ORA<ABSIX>(); break;
			case 0x19: ORA<ABSIY>(); break;
			case 0x01: ORA<INDIN>(); break;
			case 0x11: ORA<ININD>(); break;

			case 0x48: PHA<IMPLI>(); break;

			case 0x08: PHP<IMPLI>(); break;

			case 0x68: PLA<IMPLI>(); break;

			case 0x28: PLP<IMPLI>(); break;
			
			case 0x2A: ROL<ACCUM>(); break;
			case 0x26: ROL<ZEROP>(); break;
			case 0x36: ROL<ZEPIX>(); break;
			case 0x2E: ROL<ABSOL>(); break;
			case 0x3E: ROL<ABSIX>(); break;

			case 0x6A: ROR<ACCUM>(); break;
			case 0x66: ROR<ZEROP>(); break;
			case 0x76: ROR<ZEPIX>(); break;
			case 0x6E: ROR<ABSOL>(); break;
			case 0x7E: ROR<ABSIX>(); break;
			
			case 0x40: RTI<IMPLI>(); break;

			case 0x60: RTS<IMPLI>(); break;
			
			case 0xE9: SBC<IMMED>(); break;
			case 0xE5: SBC<ZEROP>(); break;
			case 0xF5: SBC<ZEPIX>(); break;
			case 0xED: SBC<ABSOL>(); break;
			case 0xFD: SBC<ABSIX>(); break;
			case 0xF9: SBC<ABSIY>(); break;
			case 0xE1: SBC<INDIN>(); break;
			case 0xF1: SBC<ININD>(); break;
			
			case 0x38: SEC<IMPLI>(); break;

			case 0xF8: SED<IMPLI>(); break;

			case 0x78: SEI<IMPLI>(); break;
			
			case 0x85: STA<ZEROP>(); break;
			case 0x95: STA<ZEPIX>(); break;
			case 0x8D: STA<ABSOL>(); break;
			case 0x9D: STA<ABSIX>(); break;
			case 0x99: STA<ABSIY>(); break;
			case 0x81: STA<INDIN>(); break;
			case 0x91: STA<ININD>(); break;
			
			case 0x86: STX<ZEROP>(); break;
			case 0x96: STX<ZEPIY>(); break;
			case 0x8E: STX<ABSOL>(); break;

			case 0x84: STY<ZEROP>(); break;
			case 0x94: STY<ZEPIX>(); break;
			case 0x8C: STY<ABSOL>(); break;
			
			case 0xAA: TAX<IMPLI>(); break;

			case 0xA8: TAY<IMPLI>(); break;

			case 0xBA: TSX<IMPLI>(); break;

			case 0x8A: TXA<IMPLI>(); break;

			case 0x9A: TXS<IMPLI>(); break;

			case 0x98: TYA<IMPLI>(); break;
		}
		++PC;
	}

	/*
	* Initialize CPU, called by power() after proper reset.
	*/
	void initialize()
	{
		ram = new uint8_t[0x800]; // RAM is addressable from $0000 to $0FFF and mirrored at $0800-$0FFF, $1000-$17FF, and $1800-$1FFF
		memset(ram, 0xFF, sizeof(ram)); // Fill memory with $FF values (erasures in EEPROMs set to $FF)
		accum = 0x00;
		x_reg = 0x00;
		y_reg = 0x00;

		// This is the RESET sequence, which is not a write cycle, so the stack pointer is moved like a BRK but no data is pushed.
		// Jump to Reset Interrupt ($FFFC and $FFFD), retrieve address, jump to subroutine...
		uint16_t addr = read(0xFFFC) + (read(0xFFFD) << 8);
		PC = addr;
		stackPush<uint16_t>(0);
		stackPush<uint8_t>(0);
	}

	/**
	* Turn on and reset CPU.
	*/
	void power()
	{
		// Do stuff to reset CPU
		PC = 0x8000;
		SP = 0x00;

		initialize();
	}
}
