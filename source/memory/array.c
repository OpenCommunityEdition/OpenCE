/* ---------- headers */

#include "cseries/cseries.h"
#include "cseries/profile.h"
#include "array.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

static struct profile_section data_00308bc0[3] =
{
	{ "memory_dynamic_array_resize", NONE, TRUE },
	{ "memory_dynamic_array_add_element", NONE, TRUE },
	{ "memory_dynamic_array_delete_element", NONE, TRUE },
};

/* ---------- public code */

void dynamic_array_new(
	struct dynamic_array *array,
	long element_size)
{
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x10,
		array);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x11,
		element_size>0);

	array->element_size = element_size;
	array->count = 0;
	array->elements = NULL;

	return;
}

boolean dynamic_array_resize(
	struct dynamic_array *array,
	long count)
{
	boolean result = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x21,
		array);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x22,
		array->element_size>0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x23,
		array->count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x24,
		(array->count!=0)==(array->elements!=NULL));

	profile_enter(data_00308bc0[0]);
	if (count>=0 && count<=LONG_MAX)
	{
		if (count == array->count)
		{
			result = TRUE;
		}
		else
		{
			void *elements = debug_realloc(
				array->elements,
				count * array->element_size,
				"c:\\halo\\SOURCE\\memory\\array.c",
				0x2C);
			if ((count!=0)==(elements!=NULL))
			{
				if (count > array->count)
				{
					csmemset(
						(char *)elements + array->count * array->element_size,
						0,
						(count - array->count) * array->element_size);
				}
				array->count = count;
				array->elements = elements;
				result = TRUE;
			}
		}
	}
	profile_exit(data_00308bc0[0]);

	return result;
}

void dynamic_array_delete(
	struct dynamic_array *array)
{
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x49,
		array);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x4A,
		array->count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x4B,
		(array->count!=0)==(array->elements!=NULL));

	array->element_size = NONE;
	array->count = NONE;
	if (array->elements)
	{
		array->elements = debug_realloc(
			array->elements,
			0,
			"c:\\halo\\SOURCE\\memory\\array.c",
			0x50);
	}

	return;
}

long dynamic_array_add_element(
	struct dynamic_array *array)
{
	long index = NONE;

	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x5D,
		array);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x5E,
		array->element_size>0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x5F,
		array->count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x60,
		(array->count!=0)==(array->elements!=NULL));

	profile_enter(data_00308bc0[1]);
	if (array->count < LONG_MAX)
	{
		long new_count = array->count + 1;
		void *elements = debug_realloc(
			array->elements,
			new_count * array->element_size,
			"c:\\halo\\SOURCE\\memory\\array.c",
			0x67);
		if (elements)
		{
			index = array->count;
			csmemset(
				(char *)elements + index * array->element_size,
				0,
				array->element_size);
			array->count = new_count;
			array->elements = elements;
		}
	}
	profile_exit(data_00308bc0[1]);

	return index;
}

void *dynamic_array_get_element(
	struct dynamic_array *array,
	long index,
	long element_size)
{
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x7D,
		array);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x7E,
		array->element_size>0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x7F,
		array->element_size==element_size);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x80,
		array->count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x81,
		(array->count!=0)==(array->elements!=NULL));
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0x82,
		index>=0 && index<array->count);

	return (char *)array->elements + index * array->element_size;
}

void dynamic_array_delete_element(
	struct dynamic_array *array,
	long index)
{
	char *element;
	void *elements;

	match_assert("c:\\halo\\SOURCE\\memory\\array.c", 139, array);
	match_assert("c:\\halo\\SOURCE\\memory\\array.c", 140, array->element_size>0);
	match_assert("c:\\halo\\SOURCE\\memory\\array.c", 141, array->count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		142,
		(array->count!=0)==(array->elements!=NULL));
	match_assert("c:\\halo\\SOURCE\\memory\\array.c", 143, index>=0 && index<array->count);

	profile_enter(data_00308bc0[2]);
	array->count--;
	if (index<array->count)
	{
		element = (char *)array->elements+index*array->element_size;
		csmemmove(
			element,
			element+array->element_size,
			(array->count-index)*array->element_size);
	}
	elements = debug_realloc(
		array->elements,
		array->count*array->element_size,
		"c:\\halo\\SOURCE\\memory\\array.c",
		156);
	array->elements = elements;
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		158,
		(array->count!=0)==(array->elements!=NULL));
	profile_exit(data_00308bc0[2]);

	return;
}

void static_array_new(
	unsigned char *count,
	void *elements,
	short element_size,
	short maximum_count)
{
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xAB,
		count);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xAC,
		elements);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xAD,
		element_size>0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xAE,
		maximum_count<=UNSIGNED_CHAR_MAX);

	*count = 0;
	csmemset(elements, NONE, element_size * maximum_count);

	return;
}

boolean static_array_resize(
	unsigned char *count,
	void *elements,
	short element_size,
	short maximum_count,
	short new_count)
{
	boolean result = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xBF,
		count && *count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xC0,
		elements);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xC1,
		element_size>0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xC2,
		maximum_count<=UNSIGNED_CHAR_MAX);

	if (new_count>=0 && new_count<maximum_count)
	{
		if (*count != new_count)
		{
			char *old_end = (char *)elements + *count * element_size;
			char *new_end = (char *)elements + new_count * element_size;
			if (new_end > old_end)
				csmemset(old_end, 0, new_end - old_end);
			else
				csmemset(new_end, NONE, old_end - new_end);
			*count = (unsigned char)new_count;
		}

		result = TRUE;
	}

	return result;
}

short static_array_add_element(
	unsigned char *count,
	void *elements,
	short element_size,
	short maximum_count)
{
	short index = NONE;

	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xE6,
		count && *count>=0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xE7,
		elements);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xE8,
		element_size>0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xE9,
		maximum_count<=UNSIGNED_CHAR_MAX);

	if (*count < maximum_count)
	{
		index = *count;
		(*count)++;
		csmemset((char *)elements + index * element_size, 0, element_size);
	}

	return index;
}

void *static_array_get_element(
	unsigned char count,
	void *elements,
	short element_size,
	short index)
{
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xFB,
		count>0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xFC,
		elements);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xFD,
		element_size>0);
	match_assert(
		"c:\\halo\\SOURCE\\memory\\array.c",
		0xFE,
		index>=0 && index<count);

	return (char *)elements + index * element_size;
}

void static_array_delete_element(
	unsigned char *count,
	void *elements,
	short element_size,
	short index)
{
	char *element;

	match_assert("c:\\halo\\SOURCE\\memory\\array.c", 265, count && *count>0);
	match_assert("c:\\halo\\SOURCE\\memory\\array.c", 266, elements);
	match_assert("c:\\halo\\SOURCE\\memory\\array.c", 267, element_size>0);
	match_assert("c:\\halo\\SOURCE\\memory\\array.c", 268, index>=0 && index<*count);

	(*count)--;
	if (index<*count)
	{
		element = (char *)elements+index*element_size;
		csmemmove(
			element,
			element+element_size,
			(*count-index)*element_size);
	}
	csmemset((char *)elements+*count*element_size, NONE, element_size);

	return;
}


/* ---------- private code */
