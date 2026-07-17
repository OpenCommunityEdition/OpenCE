/*
DATA_ENCODING.H

header included in hcex build.
*/

#ifndef __DATA_ENCODING_H
#define __DATA_ENCODING_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct data_encoding_state
{
	byte *buffer;
	long offset;
	long buffer_size;
	boolean overflow;
	byte pad[3];
};

/* ---------- prototypes/DATA_ENCODING.C */

void data_encode_new(
	struct data_encoding_state *state,
	void *buffer,
	long buffer_size);

void data_encode_memory(
	struct data_encoding_state *state,
	void const *source,
	long element_count,
	long element_size);

void data_encode_integer(
	struct data_encoding_state *state,
	long value,
	long maximum_value);

boolean data_encode_string(
	struct data_encoding_state *state,
	char const *string,
	short maximum_length);

void data_decode_new(
	struct data_encoding_state *state,
	void const *buffer,
	long buffer_size);

void *data_decode_memory(
	struct data_encoding_state *state,
	long element_count,
	long element_size);

byte data_decode_byte(
	struct data_encoding_state *state);

short data_decode_short(
	struct data_encoding_state *state);

long data_decode_long(
	struct data_encoding_state *state);

__int64 data_decode_int64(
	struct data_encoding_state *state);

long data_decode_integer(
	struct data_encoding_state *state,
	long maximum_value);

char *data_decode_string(
	struct data_encoding_state *state,
	long maximum_length);

/* ---------- globals */

/* ---------- public code */

#endif // __DATA_ENCODING_H
