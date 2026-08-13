/*
WCTOMB.C

symbols in this file:
	_wctomb (.text, 0x2b)
*/

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/* ---------- external code */

int * __cdecl _errno(
	void);

/* ---------- public code */

int __cdecl wctomb(
	char *destination,
	wchar_t character)
{
	if (!destination)
		return 0;

	if (character > UCHAR_MAX)
	{
		*_errno() = EILSEQ;
		return -1;
	}

	*destination = (char)character;
	return 1;
}
