#ifndef BINK_RAD_MATH_H
#define BINK_RAD_MATH_H
/* Original exported name. The January implementation and a period RAD.H use
 * a 64-bit x86 MUL followed by 32-bit DIV. This ordinary-C draft returns the
 * same quotient for d != 0 and a quotient fitting 32 bits. It is NOT byte exact:
 * VC7 emits __aulldiv instead. Zero divisor/quotient overflow are not a portable
 * replacement for the original processor exceptions. No assembly is imported.
 * The definition belongs to this math owner, before the BINK code section.
 */
unsigned long mult64anddiv(
	unsigned long m1,
	unsigned long m2,
	unsigned long d)
{
	return (unsigned long)(((unsigned __int64)m1*m2)/d);
}
#endif
