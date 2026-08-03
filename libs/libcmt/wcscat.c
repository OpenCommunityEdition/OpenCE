/*
WCSCAT.C

symbols in this file:
	_wcscat (.text, 0x2a)
	_wcscpy (.text, 0x1c)

Contains wcscat() and wcscpy(); the CRT keeps both in this translation unit,
which is why there is no separate wcscpy object.
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- prototypes */

wchar_t * __cdecl wcscat(wchar_t *dst, const wchar_t *src);
wchar_t * __cdecl wcscpy(wchar_t *dst, const wchar_t *src);

/* ---------- public code */

wchar_t * __cdecl wcscat(
	wchar_t *dst,
	const wchar_t *src)
{
	wchar_t *cp = dst;

	while (*cp)
		cp++;					/* find end of dst */

	while (*cp++ = *src++)
		;						/* Copy src to end of dst */

	return dst;
}

wchar_t * __cdecl wcscpy(
	wchar_t *dst,
	const wchar_t *src)
{
	wchar_t *cp = dst;

	while (*cp++ = *src++)
		;						/* Copy src over dst */

	return dst;
}
