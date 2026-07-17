/*
HASHTABLE.C

symbols in this file:
0010B070 0100:
	_hashtable_new (0000)
0010B170 0010:
	_hashtable_set_user_data (0000)
0010B180 00a0:
	_hashtable_delete (0000)
0010B220 0050:
	_default_hash_function (0000)
0010B270 0120:
	_code_0010b270 (0000)
0010B390 00b0:
	_hashtable_get (0000)
0010B440 01f0:
	_hashtable_remove (0000)
0010B630 00a0:
	_code_0010b630 (0000)
0010B6D0 0240:
	_hashtable_grow (0000)
0010B910 00c0:
	_hashtable_put (0000)
0027DB74 0020:
	??_C@_0CA@EAIKHNMM@load_factor?$DO0?5?$CG?$CG?5load_factor?$DM?$DN1?$AA@ (0000)
0027DB94 000b:
	??_C@_0L@LMBHJBKN@key_size?$DO0?$AA@ (0000)
0027DBA0 0006:
	??_C@_05LCLENNFI@table?$AA@ (0000)
0027DBA8 0022:
	??_C@_0CC@GCDECDDH@c?3?2halo?2SOURCE?2memory?2hashtable?4@ (0000)
0027DBCC 0017:
	??_C@_0BH@CIIOJKNJ@hashtable_valid?$CItable?$CJ?$AA@ (0000)
0027DBE4 001e:
	??_C@_0BO@PNMEFFDL@removing?5key?5not?5in?5hashtable?$AA@ (0000)
0027DC04 0021:
	??_C@_0CB@FCCGJANJ@putting?5key?5already?5in?5hashtable@ (0000)
0027DC28 002c:
	??_C@_0CM@FCICEOBE@table?9?$DOcapacity_bits?$CLgrowth_bits@ (0000)
0027DC54 000e:
	??_C@_0O@LDIFINGJ@growth_bits?$DO0?$AA@ (0000)
00309E5C 001e:
	_default_hash_polynomial (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "memory/hashtable.h"

/* ---------- constants */

/* ---------- macros */

#define hashtable_valid(table) \
	((table) && \
	(table)->key_size>0 && \
	(table)->element_size>0 && \
	(table)->load_factor>0 && \
	(table)->load_factor<=1 && \
	((table)->capacity_bits==NONE || \
	(1<<(table)->capacity_bits)==(table)->elements.count))

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

short default_hash_polynomial[] =
{
	3, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59
};

/* ---------- public code */

void hashtable_new(
	struct hashtable *table,
	short key_size,
	short element_size,
	real load_factor,
	hashtable_hash_function hash_function,
	hashtable_compare_function compare_function)
{
	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 41, table);
	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 42, key_size>0);
	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 43, element_size>0);
	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 44, load_factor>0 && load_factor<=1);

	table->key_size = key_size;
	table->element_size = element_size;
	table->count = 0;
	table->capacity_bits = NONE;
	table->load_factor = load_factor;
	table->hash_function = hash_function;
	table->compare_function = compare_function;
	dynamic_array_new(&table->elements, key_size + element_size);
	table->used_slots = NULL;

	return;
}

void hashtable_set_user_data(
	struct hashtable *table,
	void *user_data)
{
	table->user_data = user_data;

	return;
}

void hashtable_delete(
	struct hashtable *table)
{
	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 110, hashtable_valid(table));

	dynamic_array_delete(&table->elements);
	if (table->used_slots)
		match_free("c:\\halo\\SOURCE\\memory\\hashtable.c", 116, table->used_slots);

	return;
}

/* ---------- private code */
