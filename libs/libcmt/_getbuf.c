/*
_GETBUF.C

symbols in this file:
	__getbuf (.text, 0x44)
*/

#include <stdio.h>
#include <stdlib.h>

/* ---------- constants */

enum
{
	internal_buffer_size = 4096,
	unbuffered_character_capacity = 2
};

/* ---------- external globals */

extern int _cflush;

/* ---------- public code */

void __cdecl _getbuf(
	FILE *stream)
{
	++_cflush;

	stream->_base = malloc(internal_buffer_size);
	if (stream->_base)
	{
		stream->_flag |= _IOMYBUF;
		stream->_bufsiz = internal_buffer_size;
	}
	else
	{
		stream->_flag |= _IONBF;
		stream->_base = (char *)&stream->_charbuf;
		stream->_bufsiz = unbuffered_character_capacity;
	}

	stream->_ptr = stream->_base;
	stream->_cnt = 0;
}
