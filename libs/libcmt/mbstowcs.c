/*
MBSTOWCS.C

symbols in this file:
	_mbstowcs (.text, 0x41)
*/

#include <string.h>

size_t __cdecl mbstowcs(
	wchar_t *pwcs,
	const char *s,
	size_t n)
{
	size_t count = 0;

	if (pwcs && n == 0)
		return (size_t)0;

	if (pwcs)
	{
		while (count < n)
		{
			*pwcs = (wchar_t)((unsigned char)s[count]);
			if (!s[count])
				return count;

			count++;
			pwcs++;
		}
		return count;
	}

	{
		char character;
		const char *first_character_after_source = s + 1;
		do
		{
			character = *s++;
		}
		while (character);
		return s - first_character_after_source;
	}
}
