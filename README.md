# NES Emulator
Yet Another NES Emulator<br>
Greatly indebted to: http://nesdev.com/NESDoc.pdf

# Architecture Basics
## Memory
RAM - $0000-$1FFF<br>
I/O Registers & PPU Control Registers - $2000-$401F<br>
Expansion ROM - $4020-$5FFF<br>
SRAM - $6000-$7FFF<br>
PRG-ROM (Program) - $8000-$10000

## CPU (Ricoh 2A03 --- based on MOS 6502)
Little-Endian
16-bit address bus
56 different instructions, with addressing modes, this is 151 valid opcodes. The rest are considered unofficial opcodes used in certain, especially later, ROMs.
Program Counter<br>
Stack Pointer<br>
Accumulator<br>
Index Register X & Y<br>
Status Flags - NV0BDIZC (Carry, Zero, Interrupt Disable, Decimal Mode, Break Command, N/A, Overflow, Negative)

### Vector-based Interrupts
IRQ/BRK - Maskable Interrupt - Can be triggered by processor through BRK. Ignored by processor if I is set; jumps to address stored at $FFFE-$FFFF.<br>
NMI - Non-Maskable Interrupt - Generated by PPU when V-Blank occurs at end of frame. Only prevented if MSb of is clear. Jumps to address stored at $FFFA-$FFFB.<br>
Reset - Occurs when the system first starts and when user presses reset button; jumps to address stored at $FFFC-$FFFD.
Takes 7 clock cycles to begin executing interrupt handler. (Interrupt latency)

## PPU
### Pattern Tables
### Name Tables
### Attribute Tables
### Sprite and Image Palettes

## Memory Mappers

## ROM File Format
Need ability to discern NES 2.0 from iNES in case we use extended features.
NES 2.0 (subset of iNES and backwards-compatible. :) )<br>
First three bytes are "NES" followed by 0x1A (EOF character).<br>
Two bytes representing number of 16K PRG-ROM blocks and 8K CHR-ROM blocks respectively (can be up to two PRG-ROM if no mappers; if CHR is 0, means board uses CHR-RAM).<br>
Flags 6-10, which contain a lot of data.
11-15 = unused padding in iNES which contains more data in NES 2.0
Full details here: http://wiki.nesdev.com/w/index.php/INES and here: http://wiki.nesdev.com/w/index.php/NES_2.0
