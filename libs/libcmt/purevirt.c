/*
PUREVIRT.C

symbols in this file:
	__purecall (.text, 0x9)
*/

/* ---------- prototypes */

void __cdecl _amsg_exit(
	int error_number);

/* ---------- public code */

void __cdecl _purecall(
	void)
{
	_amsg_exit(25);

	return;
}
