/*
XTOW.C

symbols in this file:
	__itow (.text, 0x2b)
	__ltow (.text, 0x2b)
	__ultow (.text, 0x2b)
	__i64tow (.text, 0x2e)
	__ui64tow (.text, 0x2e)
*/

#include <stdlib.h>

wchar_t * __cdecl _itow(
	int value,
	wchar_t *string,
	int radix)
{
	char buffer[20];

	_itoa(value, buffer, radix);
	mbstowcs(string, buffer, sizeof(buffer));

	return string;
}

wchar_t * __cdecl _ltow(
	long value,
	wchar_t *string,
	int radix)
{
	char buffer[40];

	_ltoa(value, buffer, radix);
	mbstowcs(string, buffer, sizeof(buffer));

	return string;
}

wchar_t * __cdecl _ultow(
	unsigned long value,
	wchar_t *string,
	int radix)
{
	char buffer[40];

	_ultoa(value, buffer, radix);
	mbstowcs(string, buffer, sizeof(buffer));

	return string;
}

wchar_t * __cdecl _i64tow(
	__int64 value,
	wchar_t *string,
	int radix)
{
	char buffer[80];

	_i64toa(value, buffer, radix);
	mbstowcs(string, buffer, sizeof(buffer));

	return string;
}

wchar_t * __cdecl _ui64tow(
	unsigned __int64 value,
	wchar_t *string,
	int radix)
{
	char buffer[80];

	_ui64toa(value, buffer, radix);
	mbstowcs(string, buffer, sizeof(buffer));

	return string;
}
