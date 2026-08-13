/*
ISCTYPE.C

symbols in this file:
	__chvalidator (.text, 0x13)
	__isctype (.text, 0x21)
*/

#include <limits.h>

/* ---------- external data */

extern unsigned short *_pctype;

/* ---------- public code */

int __cdecl _chvalidator(
	int character,
	int character_type)
{
	return _pctype[character] & character_type;
}

int __cdecl _isctype(
	int character,
	int character_type)
{
	if ((unsigned int)character + 1 <= UCHAR_MAX + 1)
		return _pctype[character] & character_type;

	return 0;
}
