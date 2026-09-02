/*
CV.H

Definitions shared by the CRT floating-point conversion routines.
*/

#ifndef __LIBCMT_CV_H
#define __LIBCMT_CV_H
#pragma once

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

#define PTR_LD(x) ((u_char *)(&(x)->ld))
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

void _CALLTYPE5 __multtenpow12(
	_LDBL12 *value,
	int power,
	unsigned exact_multiplication);
void _CALLTYPE5 __ld12mul(
	_LDBL12 *first,
	_LDBL12 *second);
void _CALLTYPE5 __shl_12(
	_LDBL12 *value);
void _CALLTYPE5 __shr_12(
	_LDBL12 *value);
void _CALLTYPE5 __add_12(
	_LDBL12 *first,
	_LDBL12 *second);

int _CALLTYPE5 $I10_OUTPUT(
	_LDOUBLE value,
	int digit_count,
	unsigned output_flags,
	FOS *output);

#endif /* __LIBCMT_CV_H */
