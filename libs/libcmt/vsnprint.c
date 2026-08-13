/*
VSNPRINT.C

symbols in this file:
	__vsnprintf (.text, 0x56)
*/

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

/* ---------- external code */

int __cdecl _output(
	FILE *stream,
	char const *format,
	va_list arguments);

/* ---------- public code */

int __cdecl _vsnprintf(
	char *string,
	size_t count,
	char const *format,
	va_list arguments)
{
	FILE stream;
	int result;

	stream._cnt = (int)count;
	stream._flag = _IOWRT | _IOSTRG;
	stream._ptr = stream._base = string;

	result = _output(&stream, format, arguments);
	if (string)
		putc('\0', &stream);

	return result;
}
