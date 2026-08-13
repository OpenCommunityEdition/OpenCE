/*
FILEATT.C

symbols in this file:
	___crtGetFileAttributesW (.text, 0x39)
	___crtSetFileAttributesW (.text, 0x3c)
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

unsigned long __stdcall GetFileAttributesA(
	char const *path);

int __stdcall SetFileAttributesA(
	char const *path,
	unsigned long file_attributes);

/* ---------- public code */

/*
BUG: The original helpers ignore conversion failure and truncation, then pass
the resulting buffer to the narrow file API. A defensive implementation would
check wcstombs' return value and fail before making the API call. This behavior
is preserved for exact compatibility with the original CRT.
*/

unsigned long __cdecl __crtGetFileAttributesW(
	wchar_t const *file_name)
{
	char path_name[_max_path];

	path_name[0] = 0;
	wcstombs(path_name, file_name, sizeof(path_name));
	path_name[_max_path - 1] = 0;

	return GetFileAttributesA(path_name);
}

int __cdecl __crtSetFileAttributesW(
	wchar_t const *file_name,
	unsigned long file_attributes)
{
	char path_name[_max_path];

	path_name[0] = 0;
	wcstombs(path_name, file_name, sizeof(path_name));
	path_name[_max_path - 1] = 0;

	return SetFileAttributesA(path_name, file_attributes);
}
