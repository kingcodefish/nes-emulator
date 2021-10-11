#pragma once

#include <vector>

namespace CPU
{

	typedef enum {
		IMMED = 0, /* Immediate */
		ABSOL, /* Absolute */
		ZEROP, /* Zero Page */
		IMPLI, /* Implied */
		INDIA, /* Indirect Absolute */
		ABSIX, /* Absolute indexed with X */
		ABSIY, /* Absolute indexed with Y */
		ZEPIX, /* Zero page indexed with X */
		ZEPIY, /* Zero page indexed with Y */
		INDIN, /* Indexed indirect (with X) */
		ININD, /* Indirect indexed (with Y) */
		RELAT, /* Relative */
		ACCUM /* Accumulator */
	} addressing_mode_e;

	void execute();
	void power();
}
