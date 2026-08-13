/*
VSPRINTF.C

symbols in this file:
	_vsprintf (.text, 0x57)
	__vscprintf (.text, 0x30)
*/

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

/* ---------- external code */

int __cdecl _output(
	FILE *stream,
	char const *format,
	va_list arguments);

/* ---------- public code */

int __cdecl vsprintf(
	char *string,
	char const *format,
	va_list arguments)
{
	FILE stream;
	FILE *output = &stream;
	int result;

	output->_cnt = INT_MAX;
	output->_flag = _IOWRT | _IOSTRG;
	output->_ptr = output->_base = string;

	result = _output(output, format, arguments);

	if (string != NULL)
		putc('\0', output);

	return result;
}

int __cdecl _vscprintf(
	char const *format,
	va_list arguments)
{
	FILE stream;
	FILE *output = &stream;
	int result;

	output->_cnt = INT_MAX;
	output->_flag = _IOWRT | _IOSTRG;
	output->_ptr = output->_base = NULL;

	result = _output(output, format, arguments);

	return result;
}
