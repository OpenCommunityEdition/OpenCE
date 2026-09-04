/*
RANDOM_MATH.H
*/

#ifndef __RANDOM_MATH_H
#define __RANDOM_MATH_H
#pragma once

/* ---------- prototypes/RANDOM_MATH.C */

void lock_global_random_seed(
	void);

void unlock_global_random_seed(
	void);

unsigned long get_random_seed(
	void);

void random_seed_debug_log(
	boolean log);

#endif /* __RANDOM_MATH_H */
