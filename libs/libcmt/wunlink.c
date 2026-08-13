/*
WUNLINK.C

symbols in this file:
	__wremove (.text, 0x29)
	__wunlink (.text, 0x5)
*/

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- external code */

int __cdecl __crtDeleteFileW(
	wchar_t const *path);

unsigned long __stdcall GetLastError(
	void);

void __cdecl _dosmaperr(
	unsigned long error);

/* ---------- public code */

int __cdecl _wremove(
	wchar_t const *path)
{
	unsigned long error;

	if (!__crtDeleteFileW(path))
	{
		error = GetLastError();
	}
	else
	{
		error = 0;
	}

	if (error)
	{
		_dosmaperr(error);
		return -1;
	}

	return 0;
}

int __cdecl _wunlink(
	wchar_t const *path)
{
	return _wremove(path);
}
