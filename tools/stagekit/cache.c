/*
CACHE.C

The map file StageKit writes: a Halo Custom Edition cache (version 609, which
the port converts itself: port/linux/game/cache_file_formats.c names each
offset used here). Its tags are built where the game reads a Custom Edition
map's tags, so the game's structures' pointers are written as they are; the
structure BSP goes at the top of that room, as every map puts it.
*/

/* ---------- headers */

#include "stagekit.h"
#include <sys/mman.h>
#include "tag_schema.h" /* port/linux/game/tag_schema.h */
#include "cache_file_formats.h" /* port/linux/game/cache_file_formats.h */

/* (the game's flags ask for strict ANSI, which hides these: Linux's values) */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif
#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

/* ---------- constants */

enum
{
	HEADER_BYTES = 0x800,
	TAG_INDEX_BYTES = 0x28,
	MAXIMUM_TAGS = 128,
	/* a tag handle's salt: any, the same in every reference */
	HANDLE_SALT = 0x0E00,
	/* the room kept at the top of the tag cache for the structure BSP */
	BSP_BYTES = 0x10000,
};

/* ---------- structures */

/* a cache's header (cache_files.c keeps it to itself) */
struct cache_header
{
	unsigned long signature;
	long version;
	unsigned long file_length;
	unsigned long unused0;
	unsigned long tag_data_offset;
	unsigned long tag_data_size;
	unsigned long unused1[2];
	char name[0x20];
	char build[0x20];
	short scenario_type;
	short pad;
	unsigned long checksum;
	unsigned char unused2[0x794];
	unsigned long footer_signature;
};
typedef char cache_header_size_check[sizeof(struct cache_header) == HEADER_BYTES ? 1 : -1];

struct tag_index
{
	void *instances;
	long scenario_handle;
	unsigned long checksum;
	long tag_count;
	long vertex_count;
	unsigned long vertex_data_offset;
	long index_count;
	unsigned long index_data_offset;
	unsigned long model_data_size;
	unsigned long signature;
};

struct tag_instance
{
	unsigned long group;
	unsigned long parent_groups[2];
	long handle;
	char *name;
	void *address;
	long in_resource_map;
	long pad;
};

/* ---------- globals */

static unsigned char *tag_cache;
static unsigned long tags_used;
static unsigned long bsp_used;
static struct tag_index *tag_index;
static struct tag_instance *instances;

/* ---------- private code */

/* every reference and tag index the generator left as it was made (zero, which
names the first tag) set to none, by the groups' own schemas (tag_schema_*.c),
so the map needs no correction (tools/map_validate.c) */
static void unset_references_to_none(
	struct tag_schema_definition const *definition,
	unsigned char *base,
	char *empty_name)
{
	struct tag_schema_field const *field;

	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short index;

		for (index = 0; index < field->count; index++)
		{
			unsigned char *at = base + field->offset + index * field->size;

			if (field->type == _tag_schema_reference)
			{
				struct tag_reference *reference = (struct tag_reference *)at;
				unsigned long const *groups = field->definition;

				if (!reference->group_tag && !reference->name && !reference->index)
				{
					reference->group_tag = groups ? groups[0] : NONE;
					reference->name = empty_name;
					reference->index = NONE;
				}
			}
			else if (field->type == _tag_schema_tag_index && TEST_FLAG(field->flags, _tag_schema_none_bit))
			{
				if (!*(long *)at)
					*(long *)at = NONE;
			}
			else if (field->type == _tag_schema_block)
			{
				struct tag_block *block = (struct tag_block *)at;
				struct tag_schema_definition const *element = field->definition;
				long element_index;

				for (element_index = 0; element_index < block->count; element_index++)
					unset_references_to_none(element, (unsigned char *)block->address + element_index * element->size,
						empty_name);
			}
			else if (field->type == _tag_schema_struct)
			{
				unset_references_to_none(field->definition, at, empty_name);
			}
		}
	}
}

/* each tag's references made none, where the schemas describe its group (the
structure BSP, written apart, is left as the generator made it) */
static void tags_finish(
	void)
{
	char *empty_name = cache_allocate(1);
	long index;

	for (index = 0; index < tag_index->tag_count; index++)
	{
		struct tag_instance const *instance = &instances[index];
		struct tag_schema_group const *const *list;

		for (list = tag_schema_group_lists; *list; list++)
		{
			struct tag_schema_group const *group;

			for (group = *list; group->group_tag; group++)
			{
				if (group->group_tag == instance->group && instance->address)
					unset_references_to_none(group->definition, instance->address, empty_name);
			}
		}
	}
}

/* ---------- public code */

void cache_open(
	void)
{
	tag_cache = mmap((void *)CUSTOM_EDITION_TAG_CACHE_ADDRESS, CUSTOM_EDITION_TAG_CACHE_BYTES,
		PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	if (tag_cache != (unsigned char *)CUSTOM_EDITION_TAG_CACHE_ADDRESS)
	{
		fprintf(stderr, "stagekit: the tag cache's address is taken\n");
		exit(1);
	}
	tag_index = cache_allocate(TAG_INDEX_BYTES);
	instances = cache_allocate(MAXIMUM_TAGS * sizeof(struct tag_instance));
	tag_index->instances = instances;
	tag_index->signature = FOURCC('t', 'a', 'g', 's');
}

void *cache_allocate(
	unsigned long size)
{
	void *result = tag_cache + tags_used;

	tags_used = (tags_used + size + 3) & ~3UL;
	if (tags_used > CUSTOM_EDITION_TAG_CACHE_BYTES - BSP_BYTES)
	{
		fprintf(stderr, "stagekit: the tags do not fit\n");
		exit(1);
	}
	return result;
}

void *cache_block(
	struct tag_block *block,
	long count,
	unsigned long element_size)
{
	block->count = count;
	block->address = count ? cache_allocate(count * element_size) : NULL;
	return block->address;
}

void *cache_block_add(
	struct tag_block *block,
	unsigned long element_size)
{
	unsigned char const *old = block->address;
	unsigned char *grown = cache_allocate((block->count + 1) * element_size);

	/* (the tags are allocated upwards: a copy, its old elements first) */
	memcpy(grown, old, block->count * element_size);
	block->address = grown;
	block->count++;
	return grown + (block->count - 1) * element_size;
}

void *cache_bsp_allocate(
	unsigned long size)
{
	void *result = tag_cache + CUSTOM_EDITION_TAG_CACHE_BYTES - BSP_BYTES + bsp_used;

	bsp_used = (bsp_used + size + 3) & ~3UL;
	if (bsp_used > BSP_BYTES)
	{
		fprintf(stderr, "stagekit: the structure BSP does not fit\n");
		exit(1);
	}
	return result;
}

void *cache_bsp_block(
	struct tag_block *block,
	long count,
	unsigned long element_size)
{
	block->count = count;
	block->address = count ? cache_bsp_allocate(count * element_size) : NULL;
	return block->address;
}

long cache_tag_new(
	unsigned long group,
	unsigned long parent,
	unsigned long grandparent,
	char const *name,
	unsigned long size,
	void **data)
{
	long index = tag_index->tag_count++;
	struct tag_instance *instance = &instances[index];
	char *name_copy = cache_allocate(strlen(name) + 1);

	if (index >= (long)MAXIMUM_TAGS)
	{
		fprintf(stderr, "stagekit: more than %d tags\n", MAXIMUM_TAGS);
		exit(1);
	}
	strcpy(name_copy, name);
	instance->group = group;
	instance->parent_groups[0] = parent;
	instance->parent_groups[1] = grandparent;
	instance->handle = ((HANDLE_SALT + index) << 16) | index;
	instance->name = name_copy;
	instance->address = *data = cache_allocate(size);
	return instance->handle;
}

/* (a structure BSP's instance has no data of its own: the scenario's
reference places it) */
void cache_tag_no_data(
	long handle)
{
	instances[handle & 0xFFFF].address = NULL;
}

void cache_reference(
	struct tag_reference *reference,
	long handle)
{
	struct tag_instance *instance = &instances[handle & 0xFFFF];

	reference->group_tag = instance->group;
	reference->name = instance->name;
	reference->name_length = strlen(instance->name);
	reference->index = handle;
}

void cache_no_reference(
	struct tag_reference *reference,
	unsigned long group)
{
	reference->group_tag = group;
	reference->name = cache_allocate(1);
	reference->name_length = 0;
	reference->index = NONE;
}

void cache_set_scenario(
	long handle)
{
	tag_index->scenario_handle = handle;
}

void cache_write(
	char const *path,
	long *tag_count)
{
	struct cache_header header = { 0 };
	unsigned long tag_data_offset = HEADER_BYTES + BSP_BYTES;
	FILE *file;

	tags_finish();
	/* no models' vertices or indices: none, before the tags */
	tag_index->vertex_data_offset = tag_data_offset;
	tag_index->index_data_offset = 0;
	tag_index->model_data_size = 0;
	header.signature = FOURCC('h', 'e', 'a', 'd');
	header.version = 609;
	header.file_length = tag_data_offset + tags_used;
	header.tag_data_offset = tag_data_offset;
	header.tag_data_size = tags_used;
	strcpy(header.name, "void");
	/* (the port does not check a Custom Edition cache's build: StageKit's own) */
	strcpy(header.build, "stagekit");
	header.scenario_type = _scenario_type_solo;
	header.footer_signature = FOURCC('f', 'o', 'o', 't');
	file = fopen(path, "wb");
	if (!file || fwrite(&header, sizeof(header), 1, file) != 1 ||
		fwrite(tag_cache + CUSTOM_EDITION_TAG_CACHE_BYTES - BSP_BYTES, BSP_BYTES, 1, file) != 1 ||
		fwrite(tag_cache, tags_used, 1, file) != 1 ||
		fclose(file))
	{
		fprintf(stderr, "stagekit: could not write %s\n", path);
		exit(1);
	}
	*tag_count = tag_index->tag_count;
}

/* where the structure BSP is in the file, how big, and where in memory */
void cache_bsp_place(
	struct scenario_structure_bsp_reference *reference)
{
	reference->file_offset = HEADER_BYTES;
	reference->file_size = BSP_BYTES;
	reference->base_address = tag_cache + CUSTOM_EDITION_TAG_CACHE_BYTES - BSP_BYTES;
}
