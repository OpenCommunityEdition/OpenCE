/* Bitstream copy and authenticated mask/bit-level tables. */
#include "varbits.h"

const double _VarBitsalign= 0.0;
const unsigned long VarBitsLens[33]=
{
	0x00000000UL, 0x00000001UL, 0x00000003UL, 0x00000007UL, 0x0000000fUL, 0x0000001fUL,
	0x0000003fUL, 0x0000007fUL, 0x000000ffUL, 0x000001ffUL, 0x000003ffUL, 0x000007ffUL,
	0x00000fffUL, 0x00001fffUL, 0x00003fffUL, 0x00007fffUL, 0x0000ffffUL, 0x0001ffffUL,
	0x0003ffffUL, 0x0007ffffUL, 0x000fffffUL, 0x001fffffUL, 0x003fffffUL, 0x007fffffUL,
	0x00ffffffUL, 0x01ffffffUL, 0x03ffffffUL, 0x07ffffffUL, 0x0fffffffUL, 0x1fffffffUL,
	0x3fffffffUL, 0x7fffffffUL, 0xffffffffUL,
};
const double _bitlevelsalign= 0.0;
const unsigned char _bitlevels[129]=
{
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8,
};

#pragma code_seg("BINK")
void VarBitsCopy(
	struct VARBITS *destination,
	struct VARBITS *source,
	unsigned long bits)
{
	unsigned long value;
	while (bits >= 8)
	{
		VARBITS_GET(value, source, 8);
		VARBITS_PUT(destination, value, 8);
		bits -= 8;
	}
	if (bits)
	{
		VARBITS_GET(value, source, bits);
		VARBITS_PUT(destination, value, bits);
	}
	return;
}
