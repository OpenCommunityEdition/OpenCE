/*
TAG_WALK.C

A tag's fields walked through its group's schema (tag_walk.h).
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_walk.h"

/* ---------- private code */

static boolean walk_fields(
	byte *base,
	struct tag_schema_definition const *definition,
	short depth,
	tag_walk_proc proc,
	void *context)
{
	struct tag_schema_field const *field;

	if (depth >= MAXIMUM_TAG_WALK_DEPTH)
		return FALSE;
	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short index;

		for (index = 0; index < field->count; index++)
		{
			byte *address = base + field->offset + index * field->size;

			switch (field->type)
			{
			case _tag_schema_struct:
				if (!walk_fields(address, field->definition, depth, proc, context))
					return FALSE;
				break;

			case _tag_schema_block:
			{
				struct tag_block const *block = (struct tag_block const *)address;
				struct tag_schema_definition const *element = field->definition;
				long element_index;

				proc(context, field, address);
				for (element_index = 0; element_index < block->count; element_index++)
				{
					if (!walk_fields((byte *)block->address + element_index * element->size, element, depth + 1, proc,
						context))
					{
						return FALSE;
					}
				}
				break;
			}

			case _tag_schema_data:
			case _tag_schema_file_data:
			case _tag_schema_reference:
			case _tag_schema_tag_index:
				proc(context, field, address);
				break;

			default:
				break;
			}
		}
	}

	return TRUE;
}

/* ---------- public code */

struct tag_schema_group const *tag_walk_group(
	unsigned long group_tag,
	boolean custom_edition)
{
	struct tag_schema_group const *const *list;
	struct tag_schema_group const *group;

	/* (as the validator looks them up: schema_group_get) */
	if (custom_edition)
	{
		for (group = tag_schema_custom_edition_groups; group->group_tag; group++)
		{
			if (group->group_tag == group_tag)
				return group;
		}
	}
	for (list = tag_schema_group_lists; *list; list++)
	{
		for (group = *list; group->group_tag; group++)
		{
			if (group->group_tag == group_tag)
				return group;
		}
	}

	return NULL;
}

boolean tag_walk(
	void *root,
	unsigned long group_tag,
	boolean custom_edition,
	tag_walk_proc proc,
	void *context)
{
	struct tag_schema_group const *group = tag_walk_group(group_tag, custom_edition);

	return group && walk_fields(root, group->definition, 0, proc, context);
}
