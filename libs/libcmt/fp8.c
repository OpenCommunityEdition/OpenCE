/*
FP8.C

symbols in this file:
	__setdefaultprecision (.text, 0x12)
*/

/* ---------- constants */

#define _MCW_PC 0x00030000
#define _PC_53  0x00010000

/* ---------- external code */

unsigned int __cdecl _controlfp(
	unsigned int new_value,
	unsigned int mask);

/* ---------- public code */

void __cdecl _setdefaultprecision(
	void)
{
	_controlfp(_PC_53, _MCW_PC);

	return;
}
