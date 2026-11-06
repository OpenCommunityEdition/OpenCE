/*
TAG_WALK.H

A tag's fields walked through its group's schema (tag_schema.h): every
block, data, data in a file, tag reference and tag index in it, and in each
of its blocks' elements. For tags the validator (tag_validate.c) has passed,
whose pointers can be followed: the add-ons' builder finds what a tag uses
and what in it points (tools/addon_build.c), and the game makes an
add-on's tags its own (port/linux/game/addons.c).
*/

#ifndef __TAG_WALK_H
#define __TAG_WALK_H
#pragma once

/* ---------- headers */

#include "tag_schema.h"

/* ---------- constants */

enum
{
	/* how deep blocks go in a schema (as the validator's) */
	MAXIMUM_TAG_WALK_DEPTH = 32
};

/* ---------- structures */

/* called for each field of type _tag_schema_block (before its elements are
walked), _tag_schema_data, _tag_schema_file_data, _tag_schema_reference or
_tag_schema_tag_index, once for each in an array: address is the field's */
typedef void (*tag_walk_proc)(
	void *context,
	struct tag_schema_field const *field,
	void *address);

/* ---------- prototypes/TAG_WALK.C */

/* the schema of group_tag (custom_edition: a Custom Edition map's groups laid
out otherwise first: tag_schema_custom_edition_groups), or NULL */
struct tag_schema_group const *tag_walk_group(
	unsigned long group_tag,
	boolean custom_edition);

/* walks the tag at root, of group_tag: FALSE if the group has no schema or
its blocks go deeper than MAXIMUM_TAG_WALK_DEPTH */
boolean tag_walk(
	void *root,
	unsigned long group_tag,
	boolean custom_edition,
	tag_walk_proc proc,
	void *context);

#endif // __TAG_WALK_H
