/*
HEAPINIT.C

symbols in this file:
	___heap_select (.text, 0x4)
	__amblksiz (.data, 0x4)
*/

/* ---------- globals */

unsigned long _amblksiz = 16;

/* ---------- public code */

int __cdecl __heap_select(
	void)
{
	return 1;
}
