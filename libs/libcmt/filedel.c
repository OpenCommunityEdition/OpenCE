/*
FILEDEL.C

symbols in this file:
	___crtDeleteFileW (.text, 0x39)
*/

/* ---------- constants */

enum
{
	_max_path = 260
};

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- external code */

unsigned long __cdecl wcstombs(
	char *destination,
	wchar_t const *source,
	unsigned long count);

int __stdcall DeleteFileA(
	char const *path);

/* ---------- public code */

int __cdecl __crtDeleteFileW(
	wchar_t const *file_name)
{
	char path_name[_max_path];

	path_name[0] = 0;
	wcstombs(path_name, file_name, sizeof(path_name));
	path_name[_max_path - 1] = 0;

	return DeleteFileA(path_name);
}
