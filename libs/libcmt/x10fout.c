/*
X10FOUT.C

symbols in this file:
	_$I10_OUTPUT (.text, 0x277)
*/

#include <string.h>

typedef unsigned char u_char;
typedef char s_char;
typedef unsigned short u_short;
typedef signed short s_short;
typedef unsigned int u_long;
typedef int s_long;

#define _CALLTYPE5

typedef struct
{
	u_char ld[10];
} _LDOUBLE;

#define PTR_LD(x) ((u_char *)(&(x)->ld))

typedef struct
{
	u_char ld12[12];
} _LDBL12;

#define MAX_MAN_DIGITS 21
#define SO_FFORMAT 1

typedef struct _FloatOutStruct
{
	short exp;
	char sign;
	char ManLen;
	char man[MAX_MAN_DIGITS + 1];
} FOS;

typedef struct fixed_string_6
{
	char bytes[6];
} fixed_string_6;

typedef struct fixed_string_7
{
	char bytes[7];
} fixed_string_7;

#define PTR_12(x) ((u_char *)(&(x)->ld12))

#define MSB_USHORT ((u_short)0x8000)
#define MSB_ULONG ((u_long)0x80000000)
#define NAN_BIT (1 << 30)

#define _IS_MAN_INF(signbit, manhi, manlo) \
	((manhi) == MSB_ULONG && (manlo) == 0x0)
#define _IS_MAN_IND(signbit, manhi, manlo) \
	((signbit) && (manhi) == 0xC0000000 && (manlo) == 0)
#define _IS_MAN_QNAN(signbit, manhi, manlo) \
	((manhi) & NAN_BIT)
#define _IS_MAN_SNAN(signbit, manhi, manlo) \
	(!(_IS_MAN_INF(signbit, manhi, manlo) || \
		_IS_MAN_QNAN(signbit, manhi, manlo)))

#define U_EXP_12(p) ((u_short *)(PTR_12(p) + 10))
#define UL_MANHI_12(p) ((u_long *)(PTR_12(p) + 6))
#define UL_MANLO_12(p) ((u_long *)(PTR_12(p) + 2))
#define U_XT_12(p) ((u_short *)PTR_12(p))
#define UCHAR_12(p, i) ((u_char *)PTR_12(p) + (i))

#define U_EXP_LD(p) ((u_short *)(PTR_LD(p) + 8))
#define UL_MANHI_LD(p) ((u_long *)(PTR_LD(p) + 4))
#define UL_MANLO_LD(p) ((u_long *)PTR_LD(p))

#define COPY_FIXED_STRING_6(destination, source) \
	(*(fixed_string_6 *)(destination) = \
		*(fixed_string_6 const *)(source))
#define COPY_FIXED_STRING_7(destination, source) \
	(*(fixed_string_7 *)(destination) = \
		*(fixed_string_7 const *)(source))

#define PUT_ZERO_FOS(fos) \
	(fos)->exp = 0, \
	(fos)->sign = ' ', \
	(fos)->ManLen = 1, \
	(fos)->man[0] = '0', \
	(fos)->man[1] = 0;

#define SNAN_STR "1#SNAN"
#define SNAN_STR_LEN 6
#define QNAN_STR "1#QNAN"
#define QNAN_STR_LEN 6
#define INF_STR "1#INF"
#define INF_STR_LEN 5
#define IND_STR "1#IND"
#define IND_STR_LEN 5

void _CALLTYPE5 __multtenpow12(_LDBL12 *pld12, int pow, unsigned mult12);
void _CALLTYPE5 __ld12mul(_LDBL12 *px, _LDBL12 *py);
void _CALLTYPE5 __shl_12(_LDBL12 *ld12);
void _CALLTYPE5 __shr_12(_LDBL12 *ld12);
void _CALLTYPE5 __add_12(_LDBL12 *x, _LDBL12 *y);

int _CALLTYPE5 $I10_OUTPUT(
	_LDOUBLE ld,
	int ndigits,
	unsigned output_flags,
	FOS *fos)
{
	u_short expn;
	u_long manhi, manlo;
	u_short sign;
	u_short const log2hi = 0x4D10;
	u_short const log2lo = 0x4D;
	u_short const log4hi = 0x9A;
	u_long const c = 0x134312F4;
	_LDBL12 ld12_one_tenth =
	{
		{
			0xCC, 0xCC, 0xCC, 0xCC,
			0xCC, 0xCC, 0xCC, 0xCC,
			0xCC, 0xCC, 0xFB, 0x3F
		}
	};
	_LDBL12 ld12;
	_LDBL12 tmp12;
	u_short hh, ll;
	u_short mm;
	s_long r;
	s_short ir;
	int retval = 1;
	char round;
	char *p;
	int i;
	int ub_exp;
	int digcount;

	expn = *U_EXP_LD(&ld);
	manhi = *UL_MANHI_LD(&ld);
	manlo = *UL_MANLO_LD(&ld);
	sign = expn & MSB_USHORT;
	expn &= 0x7FFF;

	if (sign)
		fos->sign = '-';
	else
		fos->sign = ' ';

	if (expn == 0 && manhi == 0 && manlo == 0)
	{
		PUT_ZERO_FOS(fos);
		return 1;
	}

	if (expn == 0x7FFF)
	{
		fos->exp = 1;
		if (_IS_MAN_SNAN(sign, manhi, manlo))
		{
			COPY_FIXED_STRING_7(fos->man, SNAN_STR);
			fos->ManLen = SNAN_STR_LEN;
			retval = 0;
		}
		else if (_IS_MAN_IND(sign, manhi, manlo))
		{
			COPY_FIXED_STRING_6(fos->man, IND_STR);
			fos->ManLen = IND_STR_LEN;
			retval = 0;
		}
		else if (_IS_MAN_INF(sign, manhi, manlo))
		{
			COPY_FIXED_STRING_6(fos->man, INF_STR);
			fos->ManLen = INF_STR_LEN;
			retval = 0;
		}
		else
		{
			COPY_FIXED_STRING_7(fos->man, QNAN_STR);
			fos->ManLen = QNAN_STR_LEN;
			retval = 0;
		}
	}
	else
	{
		hh = expn >> 8;
		ll = expn & (u_short)0xFF;
		mm = (u_short)(manhi >> 24);
		r = (s_long)log2hi * (s_long)expn + log2lo * hh +
			log4hi * mm - c;
		ir = (s_short)(r >> 16);

		*U_EXP_12(&ld12) = expn;
		*UL_MANHI_12(&ld12) = manhi;
		*UL_MANLO_12(&ld12) = manlo;
		*U_XT_12(&ld12) = 0;

		__multtenpow12(&ld12, -ir, 1);

		if (*U_EXP_12(&ld12) >= 0x3FFF)
		{
			ir++;
			__ld12mul(&ld12, &ld12_one_tenth);
		}

		fos->exp = ir;
		if (output_flags & SO_FFORMAT)
		{
			ndigits += ir;
			if (ndigits <= 0)
			{
				PUT_ZERO_FOS(fos);
				return 1;
			}
		}
		if (ndigits > MAX_MAN_DIGITS)
			ndigits = MAX_MAN_DIGITS;

		ub_exp = *U_EXP_12(&ld12) - 0x3FFE;
		*U_EXP_12(&ld12) = 0;

		for (i = 0; i < 8; i++)
			__shl_12(&ld12);
		if (ub_exp < 0)
		{
			int shift_count = (-ub_exp) & 0xFF;
			for (; shift_count > 0; shift_count--)
				__shr_12(&ld12);
		}

		p = fos->man;
		for (digcount = ndigits + 1; digcount > 0; digcount--)
		{
			tmp12 = ld12;
			__shl_12(&ld12);
			__shl_12(&ld12);
			__add_12(&ld12, &tmp12);
			__shl_12(&ld12);

			*p++ = (char)(*UCHAR_12(&ld12, 11) + '0');
			*UCHAR_12(&ld12, 11) = 0;
		}

		round = *(--p);
		p--;
		if (round >= '5')
		{
			for (; p >= fos->man && *p == '9'; p--)
			{
				*p = '0';
			}
			if (p < fos->man)
			{
				p++;
				fos->exp++;
			}
			(*p)++;
		}
		else
		{
			for (; p >= fos->man && *p == '0'; p--)
			{
			}
			if (p < fos->man)
			{
				PUT_ZERO_FOS(fos);
				return 1;
			}
		}
		fos->ManLen = (char)(p - fos->man + 1);
		fos->man[fos->ManLen] = '\0';
	}
	return retval;
}
