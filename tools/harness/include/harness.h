/*
HARNESS.H

The asset-free tests' base (tools/harness): the game's basic types, data arrays
that refuse when full, CHECK (exits CHECK_FAILED, 1) and CASE.
*/

#ifndef HARNESS_H
#define HARNESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef float real;
typedef int boolean;
typedef unsigned char byte;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define UNSIGNED_LONG_MAX 0xFFFFFFFFUL
#define FLAG(b) (1<<(b))
#define TEST_FLAG(flags, bit) (((flags)&(unsigned)FLAG(bit))!=0)
#define match_assert(file, line, condition) ((void)0)
typedef struct { real x, y, z; } real_point3d;
typedef struct { real i, j, k; } real_vector3d;
#define DATUM_INDEX_TO_ABSOLUTE_INDEX(index) ((index) & 0xFFFF)

#define CHECK(condition, ...) \
	do \
	{ \
		if (!(condition)) \
		{ \
			fprintf(stderr, "%s:%d: check failed: %s: ", __FILE__, __LINE__, #condition); \
			fprintf(stderr, __VA_ARGS__); \
			fputc('\n', stderr); \
			exit(1); \
		} \
	} while (0)

/* data arrays (memory/data.h) of a fixed capacity */
struct datum_header { short identifier; };
struct data_array { const char *name; long maximum_count, size, count; short next_identifier; unsigned char *data; };

static struct data_array *game_state_data_new(const char *name, long maximum_count, long size)
{
	struct data_array *array = calloc(1, sizeof(*array));

	array->name = name;
	array->maximum_count = maximum_count;
	array->size = size;
	array->next_identifier = 0x1000;
	array->data = calloc((size_t)maximum_count, (size_t)size);
	return array;
}

static long datum_new(struct data_array *array)
{
	long index;

	for (index = 0; index < array->maximum_count; index++)
	{
		struct datum_header *header = (struct datum_header *)(array->data + index * array->size);

		if (!header->identifier)
		{
			memset(header, 0, (size_t)array->size);
			header->identifier = (short)(array->next_identifier++ | 0x8000);
			array->count++;
			return ((long)(unsigned short)header->identifier << 16) | index;
		}
	}
	return NONE;
}

static void *datum_get(struct data_array *array, long datum_index)
{
	return array->data + DATUM_INDEX_TO_ABSOLUTE_INDEX(datum_index) * array->size;
}

static void datum_delete(struct data_array *array, long datum_index)
{
	struct datum_header *header = datum_get(array, datum_index);

	header->identifier = 0;
	array->count--;
}

/* the case named on the command line */
#define CASE(name) if (!strcmp(case_name, name))

#endif
