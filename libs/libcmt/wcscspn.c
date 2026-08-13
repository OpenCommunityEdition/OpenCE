/*
WCSCSPN.C

symbols in this file:
	_wcscspn (.text, 0x43)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- public code */

unsigned long __cdecl wcscspn(
	wchar_t const *string,
	wchar_t const *character_set)
{
	wchar_t const *current;
	wchar_t const *character;

	current = string;
	while (*current)
	{
		character = character_set;
		while (*character)
		{
			if (*character == *current)
				return current - string;

			character++;
		}

		current++;
	}

	return current - string;
}
