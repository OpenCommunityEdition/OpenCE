/*
UNICODE.H

header included in hcex build.
*/

#ifndef __UNICODE_H
#define __UNICODE_H
#pragma once

/* ---------- headers */

#include <wchar.h>

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

#endif // __UNICODE_H
