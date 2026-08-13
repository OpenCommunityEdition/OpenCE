/*
CRT0FP.C

symbols in this file:
	__fptrap (.text, 0x9)
*/

/* ---------- external code */

void __cdecl _amsg_exit(
	int error_number);

/* ---------- public code */

void __cdecl _fptrap(
	void)
{
	_amsg_exit(2);

	return;
}
