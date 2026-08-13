/*
SNPRINTF.C

symbols in this file:
	__snprintf (.text, 0x57)
*/

#include <stdarg.h>
#include <stdio.h>

/* ---------- external code */

int __cdecl _output(
	FILE *stream,
	char const *format,
	va_list arguments);

/* ---------- public code */

int __cdecl _snprintf(
	char *string,
	size_t count,
	char const *format,
	...)
{
	FILE stream;
	FILE *output = &stream;
	va_list arguments;
	int result;

	va_start(arguments, format);

	output->_cnt = (int)count;
	output->_flag = _IOWRT | _IOSTRG;
	output->_ptr = output->_base = string;

	result = _output(output, format, arguments);

	if (string != NULL)
		putc('\0', output);

	return result;
}
