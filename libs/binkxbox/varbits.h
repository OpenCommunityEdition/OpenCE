#ifndef BINK_VARBITS_H
#define BINK_VARBITS_H

/* 32-bit, little-endian word bitstream state. buffer points to the next
 * input/output word; start is retained for position bookkeeping elsewhere.
 * Readers maintain 0..31 available bits and writers 0..31 pending bits.
 * The caller supplies aligned, sufficiently sized buffers.
 */
struct VARBITS
{
	unsigned long *buffer;
	unsigned long *start;
	unsigned long bits;
	unsigned long count;
};
extern const double _VarBitsalign;
extern const unsigned long VarBitsLens[33];
extern const double _bitlevelsalign;
extern const unsigned char _bitlevels[129];

/* Both macros are used only with a width in 1..31 here. The flush branch
 * explicitly excludes a zero remainder, avoiding a shift by 32.
 */
#define VARBITS_GET(value, stream, width) \
	do { \
		if ((stream)->count >= (width)) { \
			(value)= (stream)->bits & (~0UL >> (32-(width))); \
			(stream)->bits >>= (width); \
			(stream)->count -= (width); \
		} else { \
			unsigned long next_word= *(stream)->buffer; \
			(value)= ((next_word << (stream)->count)|(stream)->bits) & (~0UL >> (32-(width))); \
			(stream)->bits= next_word >> ((width)-(stream)->count); \
			(stream)->count += 32-(width); \
			++(stream)->buffer; \
		} \
	} while (0)

#define VARBITS_PUT(stream, value, width) \
	do { \
		unsigned long masked_value= (value)&VarBitsLens[(width)]; \
		(stream)->bits |= masked_value << (stream)->count; \
		(stream)->count += (width); \
		if ((stream)->count >= 32) { \
			*(stream)->buffer= (stream)->bits; \
			++(stream)->buffer; \
			(stream)->count -= 32; \
			if ((stream)->count) \
				(stream)->bits= masked_value >> ((width)-(stream)->count); \
			else \
				(stream)->bits= 0; \
		} \
	} while (0)

void VarBitsCopy(
	struct VARBITS *destination,
	struct VARBITS *source,
	unsigned long bits);
#endif
