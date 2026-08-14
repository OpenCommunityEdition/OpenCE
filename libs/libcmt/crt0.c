/*
CRT0.C

symbols in this file:
	__amsg_exit (.text, 0x1c)
	__aexit_rtn (.data, 0x4)
*/

/* ---------- types */

typedef void (__cdecl *_PVFI)(
	int);

/* ---------- external code */

void __cdecl _FF_MSGBANNER(
	void);

void __cdecl _NMSG_WRITE(
	int error_number);

void __cdecl _exit(
	int exit_code);

/* ---------- public data */

_PVFI _aexit_rtn = _exit;

/* ---------- public code */

void __cdecl _amsg_exit(
	int error_number)
{
	_FF_MSGBANNER();
	_NMSG_WRITE(error_number);
	(*_aexit_rtn)(255);

	return;
}
