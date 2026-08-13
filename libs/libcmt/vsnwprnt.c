/*
VSNWPRNT.C

symbols in this file:
	__vsnwprintf (.text, 0x6f)
*/

#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

/* ---------- external code */

int __cdecl _woutput(
	FILE *stream,
	wchar_t const *format,
	va_list arguments);

/* ---------- public code */

int __cdecl _vsnwprintf(
	wchar_t *string,
	size_t count,
	wchar_t const *format,
	va_list arguments)
{
	FILE stream;
	FILE *output = &stream;
	int result;

	output->_flag = _IOWRT | _IOSTRG;
	output->_ptr = output->_base = (char *)string;
	output->_cnt = (int)(count * sizeof(wchar_t));

	result = _woutput(output, format, arguments);
	putc('\0', output);
	putc('\0', output);

	return result;
}
