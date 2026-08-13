/*
_SFTBUF.C

symbols in this file:
	__stbuf (.text, 0x88)
	__ftbuf (.text, 0x2A)
	__stdbuf (.bss, 0x8)
*/

#include <io.h>
#include <stdio.h>
#include <stdlib.h>

/* ---------- private CRT constants */

enum
{
	_io_your_buffer = 0x0100,
	_io_flush_routine = 0x1000,
	_internal_buffer_size = 4096,
	_unbuffered_character_capacity = 2
};

/* ---------- external data */

extern int _cflush;

/* ---------- external code */

int __cdecl _flush(
	FILE *stream);

/* ---------- public data */

void *_stdbuf[2] = { NULL, NULL };

/* ---------- public code */

int __cdecl _stbuf(
	FILE *stream)
{
	int index;

	if (!_isatty(stream->_file))
		return 0;

	if (stream == stdout)
		index = 0;
	else if (stream == stderr)
		index = 1;
	else
		return 0;

	++_cflush;
	if (stream->_flag & (_IOMYBUF | _IONBF | _io_your_buffer))
		return 0;

	if (_stdbuf[index] == NULL &&
		(_stdbuf[index] = malloc(_internal_buffer_size)) == NULL)
	{
		stream->_ptr = stream->_base = (void *)&stream->_charbuf;
		stream->_cnt = stream->_bufsiz = _unbuffered_character_capacity;
	}
	else
	{
		stream->_ptr = stream->_base = _stdbuf[index];
		stream->_cnt = stream->_bufsiz = _internal_buffer_size;
	}

	stream->_flag |= _IOWRT | _io_your_buffer | _io_flush_routine;
	return 1;
}

void __cdecl _ftbuf(
	int temporary_buffer,
	FILE *stream)
{
	if (temporary_buffer && (stream->_flag & _io_flush_routine))
	{
		_flush(stream);
		stream->_flag &= ~(_io_your_buffer | _io_flush_routine);
		stream->_bufsiz = 0;
		stream->_base = stream->_ptr = NULL;
	}

	return;
}
