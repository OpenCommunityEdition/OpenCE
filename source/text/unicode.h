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

#endif // __UNICODE_H
