/*
UNLINK.C

symbols in this file:
	_remove (.text, 0x28)
	__unlink (.text, 0x5)
*/

/* ---------- external code */

int __stdcall DeleteFileA(
	char const *path);

unsigned long __stdcall GetLastError(
	void);

void __cdecl _dosmaperr(
	unsigned long error_number);

/* ---------- public code */

int __cdecl remove(
	char const *path)
{
	unsigned long error_number;

	if (!DeleteFileA(path))
		error_number = GetLastError();
	else
		error_number = 0;

	if (error_number)
	{
		_dosmaperr(error_number);
		return -1;
	}

	return 0;
}

int __cdecl _unlink(
	char const *path)
{
	return remove(path);
}
