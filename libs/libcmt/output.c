/*
OUTPUT.C

The printf formatting engine.  Compiled twice: directly as output.obj (narrow),
and via woutput.c which defines _UNICODE and includes this file (wide).
*/

#ifndef _X86_
#define _X86_
#endif
#include <stdarg.h>
#include <excpt.h>
#include <windef.h>
#include <winbase.h>

/* ---------- types */

typedef struct _iobuf {
	char *_ptr;
	int   _cnt;
	char *_base;
	int   _flag;
	int   _file;
	int   _charbuf;
	int   _bufsiz;
	char *_tmpfname;
} FILE;

typedef unsigned short wint_t;

#ifdef _UNICODE
#define _TCHAR      wchar_t
#define _TUCHAR     wchar_t
#define _TINT       wint_t
#define _T(x)       L##x
#define _TEOF       ((wint_t)0xffff)
#define _PUTC_LK    _putwc_lk
#define _OUTPUT     _woutput
#else
#define _TCHAR      char
#define _TUCHAR     unsigned char
#define _TINT       int
#define _T(x)       x
#define _TEOF       (-1)
#define _PUTC_LK    _putc_lk
#define _OUTPUT     _output
#endif

/* ---------- constants */

#define BUFFERSIZE      512
#define MAXPRECISION    BUFFERSIZE
#define CVTBUFSIZE      (309+40)
#define INT_MAX         0x7fffffff

#define _IOSTRG         0x0040

/* flags */
#define FL_SIGN         0x0001
#define FL_SIGNSP       0x0002
#define FL_LEFT         0x0004
#define FL_LEADZERO     0x0008
#define FL_LONG         0x0010
#define FL_SHORT        0x0020
#define FL_SIGNED       0x0040
#define FL_ALTERNATE    0x0080
#define FL_NEGATIVE     0x0100
#define FL_FORCEOCTAL   0x0200
#define FL_LONGDOUBLE   0x0400
#define FL_WIDECHAR     0x0800
#define FL_I64          0x8000

/* state machine */
#define ST_NORMAL       0
#define ST_PERCENT      1
#define ST_FLAG         2
#define ST_WIDTH        3
#define ST_DOT          4
#define ST_PRECIS       5
#define ST_SIZE         6
#define ST_TYPE         7
#define NUMSTATES       (ST_TYPE + 1)

#define FIND_CHAR_CLASS(lookuptbl, c)                                       \
	((c) < _T(' ') || (c) > _T('x') ?                                       \
		0 : lookuptbl[(c) - _T(' ')] & 0xF)

#define FIND_NEXT_STATE(lookuptbl, class, state)                            \
	(lookuptbl[(class) * NUMSTATES + (state)] >> 4)

#define WRITE_CHAR(ch, pnw)             write_char(ch, stream, pnw)
#define WRITE_MULTI_CHAR(ch, num, pnw)  write_multi_char(ch, num, stream, pnw)
#define WRITE_STRING(s, len, pnw)       write_string(s, len, stream, pnw)

/* ---------- imports */

extern const char __lookuptable[];
extern const unsigned short *_pctype;
extern int __mb_cur_max;
extern char __nullstring[];
extern wchar_t __wnullstring[];
extern void *_cfltcvt_tab[];

typedef struct { double x; } _CRT_DOUBLE;

#define _CFLTCVT(a,b,c,d,e) \
	(*((void (__cdecl *)(double *, char *, int, int, int))_cfltcvt_tab[0]))(a,b,c,d,e)
#define _CROPZEROS(a) \
	(*((void (__cdecl *)(char *))_cfltcvt_tab[1]))(a)
#define _FORCDECPT(a) \
	(*((void (__cdecl *)(char *))_cfltcvt_tab[3]))(a)

unsigned int __cdecl strlen(const char *);
#pragma intrinsic(strlen)

_TINT __cdecl _PUTC_LK(_TCHAR, FILE *);
int __cdecl mbtowc(wchar_t *, const char *, unsigned int);

/* ---------- prototypes */

static void __cdecl write_char(_TCHAR ch, FILE *f, int *pnumwritten);
static void __cdecl write_multi_char(_TCHAR ch, int num, FILE *f, int *pnumwritten);
static void __cdecl write_string(_TCHAR *string, int len, FILE *f, int *numwritten);
static int __cdecl get_int_arg(va_list *pargptr);
static __int64 __cdecl get_int64_arg(va_list *pargptr);

int __cdecl _OUTPUT(FILE *stream, const _TCHAR *format, va_list argptr);

/* ---------- public code */

int __cdecl _OUTPUT(
	FILE *stream,
	const _TCHAR *format,
	va_list argptr)
{
	int flags;
	int textlen;
	union { char *sz; wchar_t *wz; } text;
	int precision;
	int charsout;
	int prefixlen;
	int bufferiswide;
	int fldwidth;
	_TCHAR prefix[2];
	int hexadd;
	int no_output;
	int state;
	int capexp;
	_TCHAR ch;
	int charclass;
	int radix;
	wchar_t wchar;
	_CRT_DOUBLE dbl;
	_TCHAR buffer[BUFFERSIZE];

	textlen = 0;
	charsout = 0;
	state = ST_NORMAL;

	while ((ch = *format++) != _T('\0') && charsout >= 0)
	{
		charclass = FIND_CHAR_CLASS(__lookuptable, ch);
		state = FIND_NEXT_STATE(__lookuptable, charclass, state);

		switch (state)
		{
		case ST_NORMAL:
NORMAL_STATE:
			bufferiswide = 1;
			WRITE_CHAR(ch, &charsout);
			break;

		case ST_PERCENT:
			prefixlen = fldwidth = no_output = capexp = 0;
			flags = 0;
			precision = -1;
			bufferiswide = 0;
			break;

		case ST_FLAG:
			switch (ch)
			{
			case _T('-'):
				flags |= FL_LEFT;
				break;
			case _T('+'):
				flags |= FL_SIGN;
				break;
			case _T(' '):
				flags |= FL_SIGNSP;
				break;
			case _T('#'):
				flags |= FL_ALTERNATE;
				break;
			case _T('0'):
				flags |= FL_LEADZERO;
				break;
			}
			break;

		case ST_WIDTH:
			if (ch == _T('*'))
			{
				fldwidth = get_int_arg(&argptr);
				if (fldwidth < 0)
				{
					flags |= FL_LEFT;
					fldwidth = -fldwidth;
				}
			}
			else
			{
				fldwidth = fldwidth * 10 + (ch - _T('0'));
			}
			break;

		case ST_DOT:
			precision = 0;
			break;

		case ST_PRECIS:
			if (ch == _T('*'))
			{
				precision = get_int_arg(&argptr);
				if (precision < 0)
					precision = -1;
			}
			else
			{
				precision = precision * 10 + (ch - _T('0'));
			}
			break;

		case ST_SIZE:
			switch (ch)
			{
			case _T('l'):
				flags |= FL_LONG;
				break;
			case _T('I'):
				if (*format == _T('6') && *(format + 1) == _T('4'))
				{
					format += 2;
					flags |= FL_I64;
				}
				else if (*format == _T('3') && *(format + 1) == _T('2'))
				{
					format += 2;
					flags &= ~FL_I64;
				}
				else if (*format == _T('d') || *format == _T('i') ||
					*format == _T('o') || *format == _T('u') ||
					*format == _T('x') || *format == _T('X'))
				{
					/* nothing to do */
				}
				else
				{
					state = ST_NORMAL;
					goto NORMAL_STATE;
				}
				break;
			case _T('h'):
				flags |= FL_SHORT;
				break;
			case _T('w'):
				flags |= FL_WIDECHAR;
				break;
			}
			break;

		case ST_TYPE:
			switch (ch)
			{
			case _T('C'):
				if (!(flags & (FL_SHORT | FL_LONG | FL_WIDECHAR)))
#ifdef _UNICODE
					flags |= FL_SHORT;
#else
					flags |= FL_WIDECHAR;
#endif
				/* fall through */
			case _T('c'):
				{
					bufferiswide = 1;
					wchar = (wchar_t)get_int_arg(&argptr);
					if (flags & FL_SHORT)
					{
						char tempchar[2];

						tempchar[0] = (char)(wchar & 0x00ff);
						tempchar[1] = '\0';
						if (mbtowc(buffer, tempchar, __mb_cur_max) < 0)
							no_output = 1;
					}
					else
					{
						buffer[0] = wchar;
					}
					text.wz = buffer;
					textlen = 1;
				}
				break;

			case _T('Z'):
				{
					struct _count_string {
						short Length;
						short MaximumLength;
						char *Buffer;
					} *pstr;

					pstr = (struct _count_string *)get_int_arg(&argptr);
					if (pstr == NULL || pstr->Buffer == NULL)
					{
						text.sz = __nullstring;
						textlen = (int)strlen(text.sz);
					}
					else
					{
						text.sz = pstr->Buffer;
						if (flags & FL_WIDECHAR)
						{
							textlen = pstr->Length / (int)sizeof(wchar_t);
							bufferiswide = 1;
						}
						else
						{
							bufferiswide = 0;
							textlen = pstr->Length;
						}
					}
				}
				break;

			case _T('S'):
				if (!(flags & (FL_SHORT | FL_LONG | FL_WIDECHAR)))
					flags |= FL_SHORT;
				/* fall through */
			case _T('s'):
				{
					int i;
					char *p;
					wchar_t *pw;

					i = (precision == -1) ? INT_MAX : precision;
					text.sz = (char *)get_int_arg(&argptr);
					if (flags & FL_SHORT)
					{
						if (text.sz == NULL)
							text.sz = __nullstring;
						p = text.sz;
						for (textlen = 0; textlen < i && *p; textlen++)
						{
							if (_pctype[(unsigned char)*p] & 0x8000)
								p++;
							p++;
						}
					}
					else
					{
						if (text.wz == NULL)
							text.wz = __wnullstring;
						bufferiswide = 1;
						pw = text.wz;
						while (i-- && *pw)
							pw++;
						textlen = (int)(pw - text.wz);
					}
				}
				break;

			case _T('n'):
				{
					int *p;
					p = (int *)get_int_arg(&argptr);
					if (flags & FL_SHORT)
						*(short *)p = (short)charsout;
					else
						*p = charsout;
					no_output = 1;
				}
				break;

			case _T('E'):
			case _T('G'):
				capexp = 1;
				ch += _T('a') - _T('A');
				/* fall through */
			case _T('e'):
			case _T('f'):
			case _T('g'):
				{
					flags |= FL_SIGNED;
					text.sz = (char *)buffer;

					if (precision < 0)
						precision = 6;
					else if (precision == 0 && ch == _T('g'))
						precision = 1;

					dbl = va_arg(argptr, _CRT_DOUBLE);
					_CFLTCVT(&dbl.x, text.sz, (char)ch, precision, capexp);

					if ((flags & FL_ALTERNATE) && (precision == 0))
						_FORCDECPT(text.sz);

					if (ch == _T('g') && !(flags & FL_ALTERNATE))
						_CROPZEROS(text.sz);

					if (*text.sz == '-')
					{
						flags |= FL_NEGATIVE;
						text.sz++;
					}

					textlen = (int)strlen(text.sz);
				}
				break;

			case _T('d'):
			case _T('i'):
				flags |= FL_SIGNED;
				radix = 10;
				goto COMMON_INT;

			case _T('u'):
				radix = 10;
				goto COMMON_INT;

			case _T('p'):
				precision = 8;
				/* fall through */
			case _T('X'):
				hexadd = _T('A') - _T('9') - 1;
				goto COMMON_HEX;

			case _T('x'):
				hexadd = _T('a') - _T('9') - 1;
			COMMON_HEX:
				radix = 16;
				if (flags & FL_ALTERNATE)
				{
					prefix[0] = _T('0');
					prefix[1] = (_TCHAR)(_T('x') - (_T('a') - _T('9') - 1) + hexadd);
					prefixlen = 2;
				}
				goto COMMON_INT;

			case _T('o'):
				radix = 8;
				if (flags & FL_ALTERNATE)
					flags |= FL_FORCEOCTAL;
			COMMON_INT:
				{
					__int64 l;
					unsigned __int64 number;
					int digit;
					char *charbuffer;

					if (flags & FL_I64)
					{
						l = get_int64_arg(&argptr);
					}
					else if (flags & FL_SHORT)
					{
						if (flags & FL_SIGNED)
							l = (__int64)(short)get_int_arg(&argptr);
						else
							l = (__int64)(unsigned short)get_int_arg(&argptr);
					}
					else
					{
						if (flags & FL_SIGNED)
							l = (__int64)get_int_arg(&argptr);
						else
							l = (__int64)(unsigned int)get_int_arg(&argptr);
					}

					if ((flags & FL_SIGNED) && l < 0)
					{
						l = -l;
						flags |= FL_NEGATIVE;
					}

					if (flags & FL_I64)
						number = (unsigned __int64)l;
					else
						number = (unsigned long)l;

					if (precision < 0)
						precision = 1;
					else
						flags &= ~FL_LEADZERO;

					if (number == 0)
						prefixlen = 0;

					charbuffer = (char *)buffer;
					{
						char *tp;
						tp = &charbuffer[BUFFERSIZE - 1];
						while (precision-- > 0 || number != 0)
						{
							digit = (int)(number % radix) + '0';
							number /= radix;
							if (digit > '9')
								digit += hexadd;
							*tp-- = (char)digit;
						}
						textlen = (int)(&charbuffer[BUFFERSIZE - 1] - tp);
						text.sz = tp + 1;

						if (flags & FL_FORCEOCTAL)
						{
							if (*text.sz != '0' || textlen == 0)
							{
								*--text.sz = '0';
								textlen++;
							}
						}
					}
				}
				break;
			}

			if (no_output == 0)
			{
				int padding;

				if (flags & FL_SIGNED)
				{
					if (flags & FL_NEGATIVE)
					{
						prefix[0] = _T('-');
						prefixlen = 1;
					}
					else if (flags & FL_SIGN)
					{
						prefix[0] = _T('+');
						prefixlen = 1;
					}
					else if (flags & FL_SIGNSP)
					{
						prefix[0] = _T(' ');
						prefixlen = 1;
					}
				}

				padding = fldwidth - textlen - prefixlen;

				if (!(flags & (FL_LEFT | FL_LEADZERO)))
					WRITE_MULTI_CHAR(_T(' '), padding, &charsout);

				WRITE_STRING(prefix, prefixlen, &charsout);

				if ((flags & FL_LEADZERO) && !(flags & FL_LEFT))
					WRITE_MULTI_CHAR(_T('0'), padding, &charsout);

				if (bufferiswide == 0 && textlen > 0)
				{
					char *p;
					int retval;
					int count;
					p = text.sz;
					count = textlen;
					do
					{
						count--;
						retval = mbtowc(&wchar, p, __mb_cur_max);
						if (retval <= 0)
							break;
						WRITE_CHAR(wchar, &charsout);
						p += retval;
					}
					while (count > 0);
				}
				else
				{
					WRITE_STRING(text.wz, textlen, &charsout);
				}

				if (flags & FL_LEFT)
					WRITE_MULTI_CHAR(_T(' '), padding, &charsout);
			}
			break;
		}
	}

	return charsout;
}

/* ---------- private code */

static void __cdecl write_char(
	_TCHAR ch,
	FILE *f,
	int *pnumwritten)
{
	if ((f->_flag & _IOSTRG) && f->_base == NULL)
	{
		++(*pnumwritten);
		return;
	}

	if (_PUTC_LK(ch, f) == _TEOF)
		*pnumwritten = -1;
	else
		++(*pnumwritten);

	return;
}

static void __cdecl write_multi_char(
	_TCHAR ch,
	int num,
	FILE *f,
	int *pnumwritten)
{
	while (num-- > 0)
	{
		write_char(ch, f, pnumwritten);
		if (*pnumwritten == -1)
			break;
	}

	return;
}

static void __cdecl write_string(
	_TCHAR *string,
	int len,
	FILE *f,
	int *pnumwritten)
{
	if ((f->_flag & _IOSTRG) && f->_base == NULL)
	{
		*pnumwritten += len;
		return;
	}

	while (len-- > 0)
	{
		write_char(*string++, f, pnumwritten);
		if (*pnumwritten == -1)
			break;
	}

	return;
}

static int __cdecl get_int_arg(
	va_list *pargptr)
{
	return va_arg(*pargptr, int);
}

static __int64 __cdecl get_int64_arg(
	va_list *pargptr)
{
	return va_arg(*pargptr, __int64);
}
