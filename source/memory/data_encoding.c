/*
DATA_ENCODING.C

symbols in this file:
00109470 0070:
	_data_encode_new (0000)
001094E0 0130:
	_data_encode_memory (0000)
00109610 0100:
	_data_encode_integer (0000)
00109710 0100:
	_data_encode_structures (0000)
00109810 0240:
	_data_encode_array (0000)
00109A50 00a0:
	_data_encode_string (0000)
00109AF0 0070:
	_data_decode_new (0000)
00109B60 00f0:
	_data_decode_structures (0000)
00109C50 0130:
	_data_decode_memory (0000)
00109D80 0070:
	_data_decode_byte (0000)
00109DF0 0080:
	_data_decode_short (0000)
00109E70 0080:
	_data_decode_long (0000)
00109EF0 0030:
	_data_decode_int64 (0000)
00109F20 0070:
	_data_decode_integer (0000)
00109F90 0170:
	_data_decode_array (0000)
0010A100 0050:
	_data_decode_string (0000)
0027D428 0026:
	??_C@_0CG@DKDENLGI@c?3?2halo?2SOURCE?2memory?2data_encod@ (0000)
0027D450 004f:
	??_C@_0EP@IBADKJMK@state?5?$CG?$CG?5state?9?$DObuffer?5?$CG?$CG?5state?9@ (0000)
0027D4A0 0010:
	??_C@_0BA@MMBBIMAK@maximum_value?$DO0?$AA@ (0000)
0027D4B0 000e:
	??_C@_0O@DLMDCMCD@bs_definition?$AA@ (0000)
0027D4C0 0012:
	??_C@_0BC@JKGHEGPN@source_structures?$AA@ (0000)
0027D4D4 0022:
	??_C@_0CC@ENPGOLI@element_count?$DM?$DNUNSIGNED_SHORT_MA@ (0000)
0027D4F8 0021:
	??_C@_0CB@ICNNLDBF@element_count?$DM?$DNUNSIGNED_CHAR_MAX@ (0000)
0027D51C 0011:
	??_C@_0BB@LOCFNCAH@element_count?$DO?$DN0?$AA@ (0000)
0027D530 000d:
	??_C@_0N@MOKGBPFF@source_array?$AA@ (0000)
0027D540 0032:
	??_C@_0DC@POGPGOIK@state?9?$DOoffset?$CLstring_length?$CL1?$DM?$DNs@ (0000)
0027D574 0013:
	??_C@_0BD@IAHGFBAA@structure_count?$DO?$DN0?$AA@ (0000)
0027D588 0050:
	??_C@_0FA@EJBJLMFJ@state?5?$CG?$CG?5state?9?$DObuffer?5?$CG?$CG?5state?9@ (0000)
0027D5D8 0018:
	??_C@_0BI@EADHMJMM@maximum_element_count?$DO0?$AA@ (0000)
0027D5F0 0018:
	??_C@_0BI@PJCDPPLJ@element_count_reference?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "memory/data_encoding.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void data_encode_new(
	struct data_encoding_state *state,
	void *buffer,
	long buffer_size)
{
	match_assert("c:\\halo\\SOURCE\\memory\\data_encoding.c", 25, buffer);
	match_assert("c:\\halo\\SOURCE\\memory\\data_encoding.c", 26, buffer_size>=0);

	csmemset(state, 0, sizeof(*state));
	state->buffer = buffer;
	state->buffer_size = buffer_size;

	return;
}

boolean data_encode_string(
	struct data_encoding_state *state,
	char const *string,
	short maximum_length)
{
	short string_length = (short)strnlen(string, maximum_length);
	char *destination = state->buffer + state->offset;

	match_assert("c:\\halo\\SOURCE\\memory\\data_encoding.c", 182, state->offset+string_length+1<=state->buffer_size);

	if (state->offset + string_length + 1 <= state->buffer_size && !state->overflow)
	{
		csstrncpy(destination, string, string_length);
		destination[string_length] = 0;
		state->offset += string_length + 1;
	}
	else
	{
		state->overflow = TRUE;
	}

	return !state->overflow;
}

void data_decode_new(
	struct data_encoding_state *state,
	void const *buffer,
	long buffer_size)
{
	match_assert("c:\\halo\\SOURCE\\memory\\data_encoding.c", 204, buffer);
	match_assert("c:\\halo\\SOURCE\\memory\\data_encoding.c", 205, buffer_size>=0);

	csmemset(state, 0, sizeof(*state));
	state->buffer = (byte *)buffer;
	state->buffer_size = buffer_size;

	return;
}

__int64 data_decode_int64(
	struct data_encoding_state *state)
{
	__int64 *value;

	return (value = data_decode_memory(state, 1, -sizeof(*value))) ? *value : 0;
}

long data_decode_integer(
	struct data_encoding_state *state,
	long maximum_value)
{
	match_assert("c:\\halo\\SOURCE\\memory\\data_encoding.c", 321, maximum_value>0);

	if (maximum_value <= UNSIGNED_CHAR_MAX)
		return data_decode_byte(state);
	if (maximum_value <= UNSIGNED_SHORT_MAX)
		return data_decode_short(state);

	return data_decode_long(state);
}

char *data_decode_string(
	struct data_encoding_state *state,
	long maximum_length)
{
	char *string = state->buffer + state->offset;
	short string_length = 0;

	while (state->offset + string_length < state->buffer_size)
	{
		if (!string[string_length])
		{
			state->offset += string_length + 1;
			return string;
		}
		string_length++;
	}

	state->overflow = TRUE;
	return NULL;
}

/* ---------- private code */
