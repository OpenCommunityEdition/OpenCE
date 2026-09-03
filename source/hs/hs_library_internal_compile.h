/*
HS_LIBRARY_INTERNAL_COMPILE.H

file has inline function assertions.
*/

#ifndef __HS_LIBRARY_INTERNAL_COMPILE_H
#define __HS_LIBRARY_INTERNAL_COMPILE_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

boolean hs_compile_postprocess(
	char const **error_message,
	char const **error_source);
boolean hs_parse_sleep(
	short function_index,
	long expression_index);

/* ---------- globals */

/* ---------- public code */

#endif // __HS_LIBRARY_INTERNAL_COMPILE_H
