/*
HS_LIBRARY_INTERNAL.H

header included in hcex build.
*/

#ifndef __HS_LIBRARY_INTERNAL_H
#define __HS_LIBRARY_INTERNAL_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/EXAMPLE.C */

/* ---------- prototypes/HS_RUNTIME.C */

void hs_evaluate_begin(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_equality(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_inequality(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_logical(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_if(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_set(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_inspect(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_arithmetic(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_object_cast_up(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_begin_random(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_debug_string(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_sleep_until(
	short function_index,
	long thread_index,
	boolean initialize);
void hs_evaluate_sleep(
	short function_index,
	long thread_index,
	boolean initialize);

/* ---------- prototypes/HS_COMPILE.C */

boolean hs_macro_function_parse(
	short function_index,
	long expression_index);
boolean hs_parse_begin(
	short function_index,
	long expression_index);
boolean hs_parse_if(
	short function_index,
	long expression_index);
boolean hs_parse_cond(
	short function_index,
	long expression_index);
boolean hs_parse_set(
	short function_index,
	long expression_index);
boolean hs_parse_logical(
	short function_index,
	long expression_index);
boolean hs_parse_arithmetic(
	short function_index,
	long expression_index);
boolean hs_parse_equality(
	short function_index,
	long expression_index);
boolean hs_parse_inequality(
	short function_index,
	long expression_index);
boolean hs_parse_sleep(
	short function_index,
	long expression_index);
boolean hs_parse_sleep_until(
	short function_index,
	long expression_index);
boolean hs_parse_wake(
	short function_index,
	long expression_index);
boolean hs_parse_inspect(
	short function_index,
	long expression_index);
boolean hs_parse_object_cast_up(
	short function_index,
	long expression_index);
boolean hs_parse_debug_string(
	short function_index,
	long expression_index);

/* ---------- globals */

/* ---------- public code */

#endif // __HS_LIBRARY_INTERNAL_H
