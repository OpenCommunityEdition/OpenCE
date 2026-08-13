/*
SNWPRINT.C

symbols in this file:
	__snwprintf (.text, 0x70)
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

int __cdecl _snwprintf(
	wchar_t *string,
	size_t count,
	wchar_t const *format,
	...)
{
	FILE stream;
	FILE *output = &stream;
	va_list arguments;
	int result;

	va_start(arguments, format);

	output->_flag = _IOWRT | _IOSTRG;
	output->_ptr = output->_base = (char *)string;
	output->_cnt = (int)(count * sizeof(wchar_t));

	result = _woutput(output, format, arguments);
	putc('\0', output);
	putc('\0', output);

	return result;
}
