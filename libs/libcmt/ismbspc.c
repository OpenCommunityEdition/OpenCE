/*
ISMBSPC.C

symbols in this file:
	__ismbcspace (.text, 0x30)
*/

#include <ctype.h>
#include <limits.h>

/* ---------- public code */

int __cdecl _ismbcspace(
	unsigned int character)
{
	if (character > UCHAR_MAX)
		return 0;

	return isspace(character);
}
