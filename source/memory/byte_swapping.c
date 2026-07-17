/*
BYTE_SWAPPING.C

symbols in this file:
00107E40 01d0:
	_byte_swap_memory (0000)
00108010 03b0:
	_code_00108010 (0000)
001083C0 0040:
	_byte_swap_codes_size (0000)
00108400 00d0:
	_byte_swap_data (0000)
001084D0 00b0:
	_byte_swap_data_explicit (0000)
0027D040 0006:
	??_C@_05DIFFJEPM@int64?$AA@ (0000)
0027D048 0005:
	??_C@_04EHIOMBPF@word?$AA@ (0000)
0027D050 0005:
	??_C@_04IHGKJMLH@byte?$AA@ (0000)
0027D058 002d:
	??_C@_0CN@BDKNKFGE@code?$DN?$DN_2byte?5?$HM?$HM?5code?$DN?$DN_4byte?5?$HM?$HM?5@ (0000)
0027D088 0009:
	??_C@_08BGCOPMCM@count?$DO?$DN0?$AA@ (0000)
0027D094 0007:
	??_C@_06HLKELJGF@memory?$AA@ (0000)
0027D09C 0026:
	??_C@_0CG@KHPDJMKE@c?3?2halo?2SOURCE?2memory?2byte_swapp@ (0000)
0027D0C4 0023:
	??_C@_0CD@HDOCNCKO@?$CFs?5bs?5?$EA?$CFp?4?$CD?$CFd?5has?5invalid?5code?5?$CD@ (0000)
0027D0E8 002d:
	??_C@_0CN@KHMJHOOP@?$CFs?5bs?5data?5?$EA?$CFp?4?$CD1?5has?5invalid?5ar@ (0000)
0027D118 0024:
	??_C@_0CE@FFPMJICA@?$CFs?5bs?5data?5?$EA?$CFp?4?$CD0?5has?5bad?5start?5@ (0000)
0027D13C 001d:
	??_C@_0BN@HDOIFAOM@?$CFs?5bs?5data?5has?5bad?5signature?$AA@ (0000)
0027D15C 0038:
	??_C@_0DI@PAEJCKEI@got?5bs?5data?5with?5bad?5signature?5?$CI@ (0000)
0027D194 002e:
	??_C@_0CO@FJNCAFOG@?$CFs?5bs?5data?5?$EA?$CFp?5is?5?$CD?$CFd?5but?5should@ (0000)
0027D1C4 000e:
	??_C@_0O@JJIBLMBK@data_count?$DO?$DN0?$AA@ (0000)
0027D1D4 0006:
	??_C@_05CFHFIAJO@codes?$AA@ (0000)
00309DA8 0090:
	_data_00309da8 (0000)
	_byte_bs_definition (0010)
	_word_bs_definition (0034)
	_long_bs_definition (0058)
	_int64_bs_definition (007c)
*/

/* ---------- headers */

#include "cseries.h"
#include "byte_swapping.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void code_00108010(
	struct byte_swap_definition *definition,
	void *data,
	byte_swap_code *codes,
	long *size,
	byte_swap_code **next_code);

/* ---------- globals */

/* ---------- public code */

long byte_swap_codes_size(
	char *name,
	byte_swap_code *codes)
{
	struct byte_swap_definition definition;

	definition.name = name;
	definition.size = 0;
	definition.codes = codes;
	definition.signature = BYTE_SWAP_DEFINITION_SIGNATURE;
	code_00108010(&definition, NULL, codes, (long *)&name, &codes);

	return (long)name;
}

void byte_swap_data(
	struct byte_swap_definition *definition,
	void *data,
	long data_count)
{
	byte_swap_code *next_code;
	long calculated_size;
	long index;

	match_assert("c:\\halo\\SOURCE\\memory\\byte_swapping.c", 77, definition);

	if (!definition->verified && definition->size>=0)
	{
		code_00108010(
			definition,
			NULL,
			definition->codes,
			&calculated_size,
			&next_code);

		match_vassert(
			"c:\\halo\\SOURCE\\memory\\byte_swapping.c",
			88,
			calculated_size==definition->size,
			csprintf(
				temporary,
				"%s bs data @%p is #%d but should be #%d bytes",
				definition->name,
				definition,
				calculated_size,
				definition->size));
		definition->verified = TRUE;
	}

	if (data)
	{
		for (index = 0; index<data_count; index++)
		{
			code_00108010(
				definition,
				(byte *)data+definition->size*index,
				definition->codes,
				NULL,
				NULL);
		}
	}

	return;
}

void byte_swap_data_explicit(
	char *name,
	long size,
	byte_swap_code *codes,
	long data_count,
	void *data)
{
	struct byte_swap_definition definition;

	match_assert("c:\\halo\\SOURCE\\memory\\byte_swapping.c", 40, codes);
	match_assert("c:\\halo\\SOURCE\\memory\\byte_swapping.c", 41, data_count>=0);
	match_assert("c:\\halo\\SOURCE\\memory\\byte_swapping.c", 42, size>=0);

	definition.name = name;
	definition.size = size;
	definition.codes = codes;
	definition.signature = BYTE_SWAP_DEFINITION_SIGNATURE;
	definition.verified = data!=NULL;
	byte_swap_data(&definition, data, data_count);

	return;
}

/* ---------- private code */
