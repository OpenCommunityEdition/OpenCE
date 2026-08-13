/*
SPRINTF.C

symbols in this file:
	_sprintf (.text, 0x58)
	__scprintf (.text, 0x31)
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

int __cdecl sprintf(
	char *string,
	char const *format,
	...)
{
	FILE stream;
	FILE *output = &stream;
	va_list arguments;
	int result;

	va_start(arguments, format);

	output->_cnt = INT_MAX;
	output->_flag = _IOWRT | _IOSTRG;
	output->_ptr = output->_base = string;

	result = _output(output, format, arguments);

	if (string != NULL)
		putc('\0', output);

	return result;
}

int __cdecl _scprintf(
	char const *format,
	...)
{
	FILE stream;
	FILE *output = &stream;
	va_list arguments;
	int result;

	va_start(arguments, format);

	output->_cnt = INT_MAX;
	output->_flag = _IOWRT | _IOSTRG;
	output->_ptr = output->_base = NULL;

	result = _output(output, format, arguments);

	return result;
}
