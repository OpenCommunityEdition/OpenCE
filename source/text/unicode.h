/*
UNICODE.H

header included in hcex build.
*/

#ifndef __UNICODE_H
#define __UNICODE_H
#pragma once

/* ---------- headers */

#include <wchar.h>
#include <time.h>
#include <stdarg.h>

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/UNICODE.C */

/* ---------- globals */

/* ---------- public code */

int uisalpha(
	wchar_t character);
int uisupper(
	wchar_t character);
int uislower(
	wchar_t character);
int uisdigit(
	wchar_t character);
int uisxdigit(
	wchar_t character);
int uisspace(
	wchar_t character);
int uispunct(
	wchar_t character);
int uisalnum(
	wchar_t character);
int uisprint(
	wchar_t character);
int uisgraph(
	wchar_t character);
int uiscntrl(
	wchar_t character);
int utoupper(
	wchar_t character);
int utolower(
	wchar_t character);
void *umemchr(
	void const *buffer,
	int value,
	unsigned long count);
long umemcmp(
	void const *buffer1,
	void const *buffer2,
	unsigned long count);
void *umemmove(
	void *dest,
	void const *src,
	unsigned long count);
wint_t ufgetc(
	FILE *stream);
wint_t ufputc(
	wchar_t character,
	FILE *stream);
wint_t uungetc(
	wchar_t character,
	FILE *stream);
int ufclose(
	FILE *stream);
wchar_t *uctime(
	time_t const *timer);
wchar_t *uasctime(
	struct tm const *timeptr);
wchar_t *utmpnam(
	wchar_t *string);
unsigned long ustrlen(
	wchar_t const *string);
wchar_t *ustrchr(
	wchar_t const *string,
	wchar_t character);
wchar_t *ustrrchr(
	wchar_t const *string,
	wchar_t character);
wchar_t *ustrlwr(
	wchar_t *string);
wchar_t *ustrupr(
	wchar_t *string);
wchar_t *ugets(
	wchar_t *string);
int uputs(
	wchar_t const *string);
int uremove(
	wchar_t const *path);
long ustrtol(
	wchar_t const *nptr,
	wchar_t **endptr,
	int base);
int uatoi(
	wchar_t const *string);
void *umemset(
	void *buffer,
	int value,
	unsigned long count);
long ustrncmp(
	wchar_t const *string1,
	wchar_t const *string2,
	unsigned long count);
wchar_t *ustrncpy(
	wchar_t *dest,
	wchar_t const *src,
	unsigned long count);
wchar_t *ustrtok(
	wchar_t *string,
	wchar_t const *delimiters);
int uprintf(
	wchar_t const *format,
	...);
int uvprintf(
	wchar_t const *format,
	va_list arglist);
void uperror(
	wchar_t const *string);
unsigned long ustrtoul(
	wchar_t const *nptr,
	wchar_t **endptr,
	int base);
double ustrtod(
	wchar_t const *nptr,
	wchar_t **endptr);
unsigned long ustrnlen(
	wchar_t const *string,
	unsigned long maximum_length);

#endif // __UNICODE_H
