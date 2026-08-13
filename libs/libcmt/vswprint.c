/*
VSWPRINT.C

symbols in this file:
	_vswprintf (.text, 0x6e)
	__vscwprintf (.text, 0x30)
*/

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

/* ---------- external code */

int __cdecl _woutput(
	FILE *stream,
	wchar_t const *format,
	va_list arguments);

/* ---------- public code */

int __cdecl vswprintf(
	wchar_t *string,
	wchar_t const *format,
	va_list arguments)
{
	FILE stream;
	FILE *output = &stream;
	int result;

	output->_flag = _IOWRT | _IOSTRG;
	output->_ptr = output->_base = (char *)string;
	output->_cnt = INT_MAX;

	result = _woutput(output, format, arguments);
	putc('\0', output);
	putc('\0', output);

	return result;
}

int __cdecl _vscwprintf(
	wchar_t const *format,
	va_list arguments)
{
	FILE stream;
	FILE *output = &stream;
	int result;

	output->_cnt = INT_MAX;
	output->_flag = _IOWRT | _IOSTRG;
	output->_ptr = output->_base = NULL;

	result = _woutput(output, format, arguments);

	return result;
}
