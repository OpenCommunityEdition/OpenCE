/* ---------- headers */

#include "cseries.h"
#include "byte_swapping.h"

/* ---------- constants */

/* ---------- macros */

#define SWAP8(q) \
	(((q)>>56) | (((q)>>40)&0xff00) | (((q)>>24)&0xff0000) | (((q)>>8)&0xff000000) | \
	 (((q)<<8)&0xff00000000ui64) | (((q)<<24)&0xff0000000000ui64) | \
	 (((q)<<40)&0xff000000000000ui64) | ((q)<<56))

/* ---------- structures */

/* ---------- prototypes */

static void _byte_swap_data(
	struct byte_swap_definition *definition,
	void *data,
	byte_swap_code *codes,
	long *size,
	long *next_code);

/* ---------- globals */

static byte_swap_code byte_bs_codes[] = { _begin_bs_array, 1, 1, _end_bs_array };
struct byte_swap_definition byte_bs_definition =
{
	"byte",
	1,
	byte_bs_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE
};

static byte_swap_code word_bs_codes[] = { _begin_bs_array, 1, 2, _end_bs_array };
struct byte_swap_definition word_bs_definition =
{
	"word",
	2,
	word_bs_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE
};

static byte_swap_code long_bs_codes[] = { _begin_bs_array, 1, 4, _end_bs_array };
struct byte_swap_definition long_bs_definition =
{
	"long",
	4,
	long_bs_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE
};

static byte_swap_code int64_bs_codes[] = { _begin_bs_array, 1, 8, _end_bs_array };
struct byte_swap_definition int64_bs_definition =
{
	"int64",
	8,
	int64_bs_codes,
	BYTE_SWAP_DEFINITION_SIGNATURE,
	FALSE
};

/* ---------- public code */

void byte_swap_memory(
	void *memory,
	long count,
	byte_swap_code code)
{
	match_assert(
		"c:\\halo\\SOURCE\\memory\\byte_swapping.c",
		0x71,
		memory);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\byte_swapping.c",
		0x72,
		count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\byte_swapping.c",
		0x73,
		code==_2byte || code==_4byte || code==_8byte);

	switch (code)
	{
	case _2byte:
	{
		unsigned short *values = memory;
		while (count>0)
		{
			*values = SWAP2(*values);
			values++;
			count--;
		}
		break;
	}

	case _4byte:
	{
		unsigned long *values = memory;
		while (count>0)
		{
			*values = SWAP4(*values);
			values++;
			count--;
		}
		break;
	}

	case _8byte:
	{
		unsigned __int64 *values = memory;
		while (count>0)
		{
			*values = SWAP8(*values);
			values++;
			count--;
		}
		break;
	}
	}

	return;
}

static void _byte_swap_data(
	struct byte_swap_definition *definition,
	void *data,
	byte_swap_code *codes,
	long *size,
	long *next_code)
{
	long code;
	long offset;
	long code_index = 0;
	register long array_count;
	long repeat_count;
	long local_size;
	long local_code_count;
	long external_size;

	if (definition->signature!=BYTE_SWAP_DEFINITION_SIGNATURE)
	{
		display_assert(
			csprintf(temporary, "got bs data with bad signature (assuming name is wrong)"),
			"c:\\halo\\SOURCE\\memory\\byte_swapping.c",
			0xb0,
			FALSE);

		match_vassert(
			"c:\\halo\\SOURCE\\memory\\byte_swapping.c",
			0xb2,
			definition->signature==BYTE_SWAP_DEFINITION_SIGNATURE,
			csprintf(temporary, "%s bs data has bad signature", definition->name));
	}

	match_vassert(
		"c:\\halo\\SOURCE\\memory\\byte_swapping.c",
		0xb7,
		codes[0]==_begin_bs_array,
		csprintf(
			temporary,
			"%s bs data @%p.#0 has bad start #%d",
			definition->name,
			codes,
			definition->codes[0]));

	array_count = codes[1];
	match_vassert(
		"c:\\halo\\SOURCE\\memory\\byte_swapping.c",
		0xbd,
		array_count>=0,
		csprintf(
			temporary,
			"%s bs data @%p.#1 has invalid array size #%d",
			definition->name,
			codes,
			array_count));

	offset = 0;
	if (array_count<=0)
		goto done;
	repeat_count = array_count;

	do
	{
		code_index = 2;
		for (;;)
		{
			code = codes[code_index];
			switch (code)
			{
			case _2byte:
				if (data)
				{
					unsigned short value = *(unsigned short *)((byte *)data+offset);
					*(unsigned short *)((byte *)data+offset) = (unsigned short)((value>>8) | (value<<8));
				}
				code_index++;
				offset += 2;
				break;

			case _4byte:
				if (data)
				{
					*(unsigned long *)((byte *)data+offset) =
						SWAP4(*(unsigned long *)((byte *)data+offset));
				}
				code_index++;
				offset += 4;
				break;

			case _8byte:
				if (data)
				{
					*(unsigned __int64 *)((byte *)data+offset) =
						SWAP8(*(unsigned __int64 *)((byte *)data+offset));
				}
				code_index++;
				offset += 8;
				break;

			case _begin_bs_array:
				_byte_swap_data(
					definition,
					data ? (byte *)data+offset : NULL,
					codes+code_index,
					&local_size,
					&local_code_count);
				code_index += local_code_count;
				offset += local_size;
				break;

			case _extern_bs_definition:
			{
				struct byte_swap_definition *external_definition = (struct byte_swap_definition *)codes[code_index+1];
				_byte_swap_data(
					external_definition,
					data ? (byte *)data+offset : NULL,
					external_definition->codes,
					&external_size,
					NULL);
				code_index += 2;
				offset += external_size;
				break;
			}

			case _end_bs_array:
				goto next_iteration;

			default:
				if (code>0)
				{
					code_index++;
					offset += code;
				}
				else
				{
					match_vassert(
						"c:\\halo\\SOURCE\\memory\\byte_swapping.c",
						0x129,
						FALSE,
						csprintf(
							temporary,
							"%s bs @%p.#%d has invalid code #%d",
							definition->name,
							codes,
							code_index,
							code));
				}
				break;
			}
		}

next_iteration:
		code_index++;
		repeat_count--;
	}
	while (repeat_count!=0);

done:
	if (size)
		*size = offset;
	if (next_code)
		*next_code = code_index;

	return;
}

long byte_swap_codes_size(
	char *name,
	byte_swap_code *codes)
{
	struct byte_swap_definition definition;

	definition.name = name;
	definition.size = 0;
	definition.codes = codes;
	definition.signature = BYTE_SWAP_DEFINITION_SIGNATURE;
	_byte_swap_data(&definition, NULL, codes, (long *)&name, (long *)&codes);

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
		_byte_swap_data(
			definition,
			NULL,
			definition->codes,
			&calculated_size,
			(long *)&next_code);

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
			_byte_swap_data(
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
