/*
Single-function TU reproducing cseries::stristr's register-allocation tie.
No headers: the two callees are declared locally so the compile is standalone.
The contest is which of {first, length} wins ebx; January gives it to first.
*/

unsigned long csstrlen(const char *s);
int _strnicmp(const char *a, const char *b, unsigned long n);

char *stristr(
	const char *haystack,
	const char *needle)
{
	char first = *needle++;
	long length;

	if (first)
	{
		length = csstrlen(needle);
		for (;;)
		{
			char character = *haystack++;

			if (!character)
				return 0;
			if (character==first && !_strnicmp(haystack, needle, length))
				return (char *)(haystack-1);
		}
	}

	return (char *)haystack;
}
