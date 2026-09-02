/*
X10FOUT.C

symbols in this file:
	_$I10_OUTPUT (.text, 0x277)
*/

#include <string.h>
#include "cv.h"

#define STRCPY strcpy

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
			STRCPY(fos->man, SNAN_STR);
			fos->ManLen = SNAN_STR_LEN;
			retval = 0;
		}
		else if (_IS_MAN_IND(sign, manhi, manlo))
		{
			STRCPY(fos->man, IND_STR);
			fos->ManLen = IND_STR_LEN;
			retval = 0;
		}
		else if (_IS_MAN_INF(sign, manhi, manlo))
		{
			STRCPY(fos->man, INF_STR);
			fos->ManLen = INF_STR_LEN;
			retval = 0;
		}
		else
		{
			STRCPY(fos->man, QNAN_STR);
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
