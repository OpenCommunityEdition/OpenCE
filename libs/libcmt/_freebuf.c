/*
_FREEBUF.C

symbols in this file:
	__freebuf (.text, 0x2b)
*/

#include <stdio.h>
#include <stdlib.h>

/* ---------- private CRT flags */

/* Set when setvbuf supplied the stream buffer. */
#define _IOSETVBUF 0x0400

/* ---------- public code */

void __cdecl _freebuf(
	FILE *stream)
{
	if ((stream->_flag & (_IOREAD | _IOWRT | _IORW)) &&
		(stream->_flag & _IOMYBUF))
	{
		free(stream->_base);
		stream->_flag &= ~(_IOMYBUF | _IOSETVBUF);
		stream->_ptr = 0;
		stream->_base = 0;
		stream->_cnt = 0;
	}
}
