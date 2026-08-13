/*
MATHERR.C

symbols in this file:
	__matherr (.text, 0x3)
	_matherr_flag (.data, 0x4)
*/

/* ---------- structures */

struct _exception;

/* ---------- globals */

int _matherr_flag = 9876;

/* ---------- public code */

int __cdecl _matherr(
	struct _exception *exception)
{
	return 0;
}
