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

short default_hash_function(
	const void *key,
	unsigned long key_size);
static boolean hashtable_search(
	struct hashtable *table,
	const void *key,
	short *element_index);
static void *hashtable_capacious_put(
	struct hashtable *table,
	const void *key);

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

short default_hash_function(
	const void *key,
	unsigned long key_size)
{
	short hash = 0;
	short polynomial_index = 0;
	const byte *key_byte;

	if (polynomial_index < key_size)
	{
		key_byte = key;
		do
		{
			if (polynomial_index == NUMBEROF(default_hash_polynomial))
			{
				polynomial_index = 0;
				key_size -= NUMBEROF(default_hash_polynomial);
			}

			hash += default_hash_polynomial[polynomial_index] * (short)*key_byte;
			polynomial_index++;
			key_byte++;
		}
		while (polynomial_index < key_size);
	}

	return hash;
}

void *hashtable_get(
	struct hashtable *table,
	const void *key)
{
	void *result = NULL;
	short element_index;

	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 77, hashtable_valid(table));
	if (table->count && hashtable_search(table, key, &element_index))
	{
		result = (byte *)dynamic_array_get_element(
			&table->elements,
			element_index,
			table->element_size) + table->key_size;
	}

	return result;
}

void hashtable_remove(
	struct hashtable *table,
	const void *key)
{
	short empty_index;
	short element_index;
	short home_index;
	long hash;
	void *element;

	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 195, hashtable_valid(table));
	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 196, key);

	if (hashtable_search(table, key, &empty_index))
	{
		element_index = (empty_index + 1) & (table->elements.count - 1);
		while (BIT_VECTOR_TEST_FLAG(table->used_slots, element_index))
		{
			element = dynamic_array_get_element(
				&table->elements,
				element_index,
				table->element_size);
			if (table->hash_function)
			{
				hash = table->hash_function(table->user_data, element);
			}
			else
			{
				hash = default_hash_function(element, table->key_size);
			}

			home_index = hash & (table->elements.count - 1);
			if ((home_index < element_index &&
				empty_index >= home_index &&
				empty_index < element_index) ||
				(home_index > element_index &&
				(empty_index >= home_index || empty_index < element_index)))
			{
				csmemcpy(
					dynamic_array_get_element(
						&table->elements,
						empty_index,
						table->element_size),
					dynamic_array_get_element(
						&table->elements,
						element_index,
						table->element_size),
					table->elements.element_size);
				empty_index = element_index;
			}

			element_index = (element_index + 1) & (table->elements.count - 1);
		}

		BIT_VECTOR_SET_FLAG(table->used_slots, empty_index, FALSE);
	}
	else
	{
		match_vassert(
			"c:\\halo\\SOURCE\\memory\\hashtable.c",
			225,
			FALSE,
			"removing key not in hashtable");
	}

	return;
}

boolean hashtable_grow(
	struct hashtable *table,
	short growth_bits)
{
	short old_capacity_bits = table->capacity_bits;
	short old_count = table->count;
	unsigned long *old_used_slots = table->used_slots;
	struct dynamic_array old_elements = table->elements;
	short old_element_index;
	short new_count;
	void *old_element;

	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 134, hashtable_valid(table));
	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 135, growth_bits>0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\hashtable.c",
		136,
		table->capacity_bits+growth_bits<SHORT_BITS);

	table->capacity_bits += growth_bits;
	new_count = 1<<table->capacity_bits;
	table->count = 0;
	table->used_slots = match_malloc(
		"c:\\halo\\SOURCE\\memory\\hashtable.c",
		143,
		BIT_VECTOR_SIZE_IN_BYTES(new_count));
	if (table->used_slots)
	{
		dynamic_array_new(&table->elements, table->elements.element_size);
		if (dynamic_array_resize(&table->elements, new_count))
		{
			csmemset(table->used_slots, 0, BIT_VECTOR_SIZE_IN_BYTES(new_count));
			for (old_element_index = 0;
				old_element_index<old_elements.count;
				old_element_index++)
			{
				if (BIT_VECTOR_TEST_FLAG(old_used_slots, old_element_index))
				{
					old_element = dynamic_array_get_element(
						&old_elements,
						old_element_index,
						old_elements.element_size);
					csmemcpy(
						hashtable_capacious_put(table, old_element),
						(byte *)old_element+table->key_size,
						table->element_size);
				}
			}

			if (old_used_slots)
			{
				match_free(
					"c:\\halo\\SOURCE\\memory\\hashtable.c",
					168,
					old_used_slots);
			}
			dynamic_array_delete(&old_elements);

			return TRUE;
		}

		match_free(
			"c:\\halo\\SOURCE\\memory\\hashtable.c",
			176,
			table->used_slots);
	}

	table->capacity_bits = old_capacity_bits;
	table->count = old_count;
	table->used_slots = old_used_slots;
	table->elements = old_elements;

	return FALSE;
}

void *hashtable_put(
	struct hashtable *table,
	const void *key)
{
	match_assert("c:\\halo\\SOURCE\\memory\\hashtable.c", 93, hashtable_valid(table));
	if ((table->capacity_bits == NONE ||
		table->count >= table->elements.count * table->load_factor) &&
		!hashtable_grow(table, 1 + (table->capacity_bits == NONE)))
	{
		return NULL;
	}

	return hashtable_capacious_put(table, key);
}

/* ---------- private code */

static boolean hashtable_search(
	struct hashtable *table,
	const void *key,
	short *element_index_reference)
{
	short element_index;
	short probe_count = 0;
	long hash;
	long equal;
	boolean result = FALSE;

	if (table->hash_function)
	{
		hash = table->hash_function(table->user_data, key);
	}
	else
	{
		hash = default_hash_function(key, table->key_size);
	}

	element_index = hash & (table->elements.count - 1);
	while (BIT_VECTOR_TEST_FLAG(table->used_slots, element_index))
	{
		if (probe_count < table->count)
		{
			if (table->compare_function)
			{
				equal = table->compare_function(
					table->user_data,
					dynamic_array_get_element(
						&table->elements,
						element_index,
						table->element_size),
					key);
			}
			else
			{
				equal = !csmemcmp(
					dynamic_array_get_element(
						&table->elements,
						element_index,
						table->element_size),
					key,
					table->key_size);
			}

			if (equal)
			{
				result = TRUE;
				goto done;
			}

			element_index = (element_index + 1) & (table->elements.count - 1);
			probe_count++;
		}
		else
		{
			goto done;
		}
	}

done:
	*element_index_reference = element_index;
	return result;
}

static void *hashtable_capacious_put(
	struct hashtable *table,
	const void *key)
{
	short element_index;
	void *element;
	void *result = NULL;

	if (hashtable_search(table, key, &element_index))
	{
		match_vassert(
			"c:\\halo\\SOURCE\\memory\\hashtable.c",
			241,
			FALSE,
			"putting key already in hashtable");
	}
	else
	{
		element = dynamic_array_get_element(
			&table->elements,
			element_index,
			table->element_size);
		csmemcpy(element, key, table->key_size);
		BIT_VECTOR_SET_FLAG(table->used_slots, element_index, TRUE);
		result = (byte *)element + table->key_size;
		table->count++;
	}

	return result;
}
