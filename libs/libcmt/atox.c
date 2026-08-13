/*
ATOX.C

symbols in this file:
	_atol (.text, 0x58)
	_atoi (.text, 0x5)
	__atoi64 (.text, 0x79)
*/

/* ---------- macros */

#define CHARACTER_TO_DIGIT(character) \
	((character) >= '0' && (character) <= '9' ? (character) - '0' : -1)

/* ---------- external code */

int __cdecl _ismbcspace(
	unsigned int character);

/* ---------- public code */

long __cdecl atol(
	char const *nptr)
{
	int character;
	long total;
	int sign;

	while (_ismbcspace((unsigned char)*nptr))
		++nptr;

	character = (unsigned char)*nptr++;
	sign = character;
	if (character == '-' || character == '+')
		character = (unsigned char)*nptr++;

	total = 0;

	while ((character = CHARACTER_TO_DIGIT(character)) != -1)
	{
		total = 10 * total + character;
		character = (unsigned char)*nptr++;
	}

	if (sign == '-')
		return -total;
	else
		return total;
}

int __cdecl atoi(
	char const *nptr)
{
	return (int)atol(nptr);
}

__int64 __cdecl _atoi64(
	char const *nptr)
{
	int character;
	__int64 total;
	int sign;

	while (_ismbcspace((unsigned char)*nptr))
		++nptr;

	character = (unsigned char)*nptr++;
	sign = character;
	if (character == '-' || character == '+')
		character = (unsigned char)*nptr++;

	total = 0;

	while ((character = CHARACTER_TO_DIGIT(character)) != -1)
	{
		total = 10 * total + character;
		character = (unsigned char)*nptr++;
	}

	if (sign == '-')
		return -total;
	else
		return total;
}
