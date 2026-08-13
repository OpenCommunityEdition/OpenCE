/*
CMISCDAT.C

symbols in this file:
	__cfltcvt_tab (.data, 0x18)
*/

typedef void (__cdecl *cfltcvt_proc)(void);

/* ---------- externals */

void __cdecl _fptrap(void);

/* ---------- public data */

cfltcvt_proc _cfltcvt_tab[6] =
{
	_fptrap,
	_fptrap,
	_fptrap,
	_fptrap,
	_fptrap,
	_fptrap
};
