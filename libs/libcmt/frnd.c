/*
FRND.C

symbols in this file:
	__frnd (.text, 0x13)
*/

double __cdecl _frnd(
	double x)
{
	double result;

	__asm
	{
		fld x
		frndint
		fstp result
	}

	return result;
}
