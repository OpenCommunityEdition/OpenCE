/*
WACCESS.C

symbols in this file:
	__waccess (.text, 0x45)
*/

/* ---------- constants */

enum
{
	_file_attribute_read_only = 0x1,
	_file_access_write = 0x2,
	_error_access_denied = 5,
	_errno_access_denied = 13,
	_invalid_file_attributes = -1
};

/* ---------- types */

typedef unsigned short wchar_t;

/* ---------- external code */

unsigned long __cdecl __crtGetFileAttributesW(
	wchar_t const *path);

unsigned long __stdcall GetLastError(
	void);

void __cdecl _dosmaperr(
	unsigned long error);

int *__cdecl _errno(
	void);

unsigned long *__cdecl __doserrno(
	void);

#define errno (*_errno())
#define _doserrno (*__doserrno())

/* ---------- public code */

int __cdecl _waccess(
	wchar_t const *path,
	int mode)
{
	unsigned long attributes;

	attributes = __crtGetFileAttributesW(path);
	if (attributes == _invalid_file_attributes)
	{
		_dosmaperr(GetLastError());
		return -1;
	}

	if (attributes & _file_attribute_read_only && mode & _file_access_write)
	{
		errno = _errno_access_denied;
		_doserrno = _error_access_denied;
		return -1;
	}
	else
	{
		return 0;
	}
}
