/*
TAG_NAMES.C

Names for the tags of a Custom Edition map that have none of their own
(tag_names.h). A protected map's tags keep their data, but not their paths:
each is "<protected>", nothing, or a path many tags of its group share,
and the Tag Injector lists tags, keeps them in profiles and adds them to a
level by their group and path (tag_injector.c). The tags read from a
resource map keep theirs (the map's loader finds them by their paths), and
so does every tag whose path is one: printable, without the characters a
file's name cannot have, and no other tag of its group's.

Every name made is under the map's name, so that none is a path another
map's tags have (the campaign's, which a tag of that path would be taken
for in a level), and is made, in this order, for:
- a weapon, equipment and the other objects a level places (bipeds,
  vehicles, scenery, devices): "<map>\<kind>\<name>\<name>", its name its
  pickup text (an item's, as the HUD says it: "battle rifle"), else that of
  the stock folder its tags with names have most of (a biped whose bitmaps
  are characters\elite\...'s: "elite"), else its kind ("biped");
- an actor variant: in its unit's folder, its unit's name and its weapon's
  ("<map>\characters\elite\elite plasma rifle");
- the scenario and the globals, if they have none: "<map>\<map>" and
  "<map>\globals";
- every other tag, first found from those (the objects first, the scenario
  last): in the folder of the tag that names it, by the field it is named
  by ("<map>\weapons\battle rifle\projectile");
- and what nothing names: "<map>\unnamed\<group>".
A name another tag of its group has (any case) is numbered ("battle rifle
2"). The same map gives the same names, however often it is loaded.
*/

/* ---------- headers */

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif
#include "cseries.h"
#include "interface/hud_definitions.h"
#include "items/item_definitions.h"
#include "text/text_group.h"
#include "tag_files/tag_groups.h"
#include "tag_schema.h"
#include "tag_names.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef malloc
#undef free
#undef realloc

/* ---------- constants */

enum
{
	HUD_GLOBALS_GROUP_TAG = 'hudg',
	UNICODE_STRING_LIST_GROUP_TAG = 'ustr',
	ACTOR_VARIANT_GROUP_TAG = 'actv',
	SCENARIO_GROUP_TAG = 'scnr',
	GLOBALS_GROUP_TAG = 'matg',
	ITEM_GROUP_TAG = 'item',

	/* the longest path, with its terminator (the game's buffers' size) */
	MAXIMUM_TAG_NAME_LENGTH = 256,
	/* the longest part of a name made, and the folder it is put in (a
	longer one is the map's) */
	MAXIMUM_LEAF_LENGTH = 40,
	MAXIMUM_PREFIX_LENGTH = 160,
	/* how many of a name are numbered */
	MAXIMUM_NAME_NUMBER = 9999,
	/* how deep a tag's blocks go, and how far an object's tags are looked
	through for the stock folder they are of */
	MAXIMUM_WALK_DEPTH = 32,
	STOCK_FOLDER_DEPTH = 3,
	MAXIMUM_STOCK_CANDIDATES = 16,
};

/* a tag's name: its own, made, or none yet */
enum
{
	_name_none,
	_name_kept,
	_name_made,
};

/* ---------- structures */

/* a tag of a Custom Edition map's table (tag_validate.c's, tag_bundle.c's) */
struct names_instance
{
	unsigned long group_tag;
	unsigned long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	/* (read from a resource map) */
	unsigned long in_resource_map;
	unsigned long unused;
};

struct names_header
{
	struct names_instance *instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
};

/* a name taken, in the names' table: of a tag of this group */
struct names_entry
{
	unsigned long group_tag;
	char const *name;
	long tag;
};

/* the stock folders a tag's tags are of, and how many of them */
struct stock_candidate
{
	char name[MAXIMUM_LEAF_LENGTH + 1];
	long count;
};

struct names_state
{
	struct names_instance *instances;
	long tag_count;
	char map[MAXIMUM_LEAF_LENGTH + 1];
	byte *status;
	/* the names taken (a table of table_size, a power of two) */
	struct names_entry *table;
	unsigned long table_size;
	/* the names made */
	char *pool;
	unsigned long pool_size;
	unsigned long pool_used;
	/* the tags whose tags are named next, and those queued */
	long *queue;
	long queue_count;
	byte *queued;
	/* the walk looking for an object's stock folder: the tags it has been
	through (this walk's stamp), and what it found */
	long *stamps;
	long stamp;
	char const *stock_folder;
	struct stock_candidate candidates[MAXIMUM_STOCK_CANDIDATES];
	short candidate_count;
	/* the HUD's item texts (an item's pickup text is one of them) */
	struct string_list const *item_texts;
	/* what the walk of a tag's references is for */
	long parent;
	long found;
	char const *found_field;
};

typedef void (*names_reference_proc)(struct names_state *state, long child, char const *field_name, short depth);

/* ---------- globals */

/* the objects a level places, in the order their names are made (and their
tags named from them): their folder, what one is called, and the stock
folder a tag of theirs is looked for in */
static struct
{
	unsigned long group_tag;
	char const *folder;
	char const *kind;
	char const *stock_folder;
} const names_kinds[] =
{
	{ 'weap', "weapons", "weapon", "weapons" },
	{ 'eqip', "powerups", "equipment", "powerups" },
	{ 'garb', "items", "item", NULL },
	{ 'bipd', "characters", "biped", "characters" },
	{ 'vehi', "vehicles", "vehicle", "vehicles" },
	{ 'scen', "scenery", "scenery", "scenery" },
	{ 'ssce', "scenery", "sound scenery", "scenery" },
	{ 'mach', "devices", "machine", NULL },
	{ 'ctrl', "devices", "control", NULL },
	{ 'lifi', "devices", "light fixture", NULL },
	{ 'plac', "objects", "placeholder", NULL },
};

/* the names the last tag_names_restore made */
static char *tag_names_pool = NULL;

/* ---------- private code */

static char names_lower(
	char character)
{
	return character >= 'A' && character <= 'Z' ? (char)(character - 'A' + 'a') : character;
}

static boolean names_equal(
	char const *first,
	char const *second)
{
	for (; *first && names_lower(*first) == names_lower(*second); first++, second++)
	{
	}

	return !*first && !*second;
}

/* whether `name` starts with the folder `folder` (any case), then a
separator */
static boolean names_in_folder(
	char const *name,
	char const *folder)
{
	for (; *folder && names_lower(*folder) == names_lower(*name); folder++, name++)
	{
	}

	return !*folder && *name == '\\';
}

static unsigned long names_hash(
	unsigned long group_tag,
	char const *name)
{
	unsigned long hash = 2166136261UL ^ group_tag;

	for (; *name; name++)
		hash = (hash ^ (unsigned char)names_lower(*name)) * 16777619UL;

	return hash;
}

/* the table's entry of a group's name, or the empty one it would be in */
static struct names_entry *names_slot(
	struct names_state *state,
	unsigned long group_tag,
	char const *name)
{
	unsigned long index = names_hash(group_tag, name) & (state->table_size - 1);

	while (state->table[index].name &&
		(state->table[index].group_tag != group_tag || !names_equal(state->table[index].name, name)))
	{
		index = (index + 1) & (state->table_size - 1);
	}

	return &state->table[index];
}

static boolean names_taken(
	struct names_state *state,
	unsigned long group_tag,
	char const *name)
{
	return names_slot(state, group_tag, name)->name != NULL;
}

static void names_take(
	struct names_state *state,
	unsigned long group_tag,
	char const *name,
	long tag)
{
	struct names_entry *entry = names_slot(state, group_tag, name);

	if (!entry->name)
	{
		entry->group_tag = group_tag;
		entry->name = name;
		entry->tag = tag;
	}
}

/* the absolute index of the tag a tag index names, or NONE */
static long names_tag(
	struct names_state const *state,
	long tag_index)
{
	long absolute = (long)(unsigned short)tag_index;

	return tag_index != NONE && absolute < state->tag_count && state->instances[absolute].tag_index == tag_index ?
		absolute : NONE;
}

static char const *names_name(
	struct names_state const *state,
	long tag)
{
	return state->status[tag] != _name_none && state->instances[tag].name ? state->instances[tag].name : "";
}

/* a path of the map's own: printable, none of the characters a file's name
cannot have, not starting or ending with a space or a separator */
static boolean name_usable(
	char const *name)
{
	long length;

	if (!name || !name[0] || name[0] == ' ' || name[0] == '\\')
		return FALSE;
	for (length = 0; name[length]; length++)
	{
		unsigned char character = (unsigned char)name[length];

		if (character < 0x20 || character > 0x7E || strchr("<>:\"|?*", character) ||
			length >= MAXIMUM_TAG_NAME_LENGTH - 1)
			return FALSE;
	}

	return name[length - 1] != ' ' && name[length - 1] != '\\';
}

/* text made a part of a name: its letters (lower case) and digits, and a
space for each run of anything else (but -), a format's (%d) left out, at
most `limit` */
static void leaf_append(
	char *leaf,
	unsigned short character,
	unsigned short next,
	boolean *skip,
	long limit)
{
	long length = (long)strlen(leaf);
	char ascii = character < 0x80 ? names_lower((char)character) : ' ';

	if (*skip)
	{
		*skip = FALSE;
		return;
	}
	if (ascii == '%' && next)
	{
		*skip = TRUE;
		return;
	}
	if (!((ascii >= 'a' && ascii <= 'z') || (ascii >= '0' && ascii <= '9') || ascii == '-'))
		ascii = ' ';
	if (length >= limit || (ascii == ' ' && (!length || leaf[length - 1] == ' ')))
		return;
	leaf[length] = ascii;
	leaf[length + 1] = 0;
}

static void leaf_trim(
	char *leaf)
{
	long length = (long)strlen(leaf);

	while (length > 0 && leaf[length - 1] == ' ')
		leaf[--length] = 0;
}

static void leaf_clean(
	char *leaf,
	char const *text,
	long length)
{
	boolean skip = FALSE;
	long index;

	leaf[0] = 0;
	for (index = 0; index < length && text[index]; index++)
	{
		leaf_append(leaf, (unsigned char)text[index], index + 1 < length ? (unsigned char)text[index + 1] : 0, &skip,
			MAXIMUM_LEAF_LENGTH);
	}
	leaf_trim(leaf);
}

/* `text` without `start` at its start (TRUE), or as it was */
static boolean text_strip(
	char *text,
	char const *start)
{
	long length;
	long index;

	for (length = 0; start[length] && text[length] == start[length]; length++)
	{
	}
	if (start[length])
		return FALSE;
	for (index = 0; text[length + index]; index++)
		text[index] = text[length + index];
	text[index] = 0;

	return TRUE;
}

/* the last part of a name, made a part of another */
static void leaf_of(
	char *leaf,
	char const *name)
{
	char const *slash = strrchr(name, '\\');

	leaf_clean(leaf, slash ? slash + 1 : name, MAXIMUM_TAG_NAME_LENGTH);
}

/* what a field is called, as a part of a name: its last part (its
structure's before it for one called reference), without _reference
("object.collision_model": "collision model") */
static void field_leaf(
	char *leaf,
	char const *field_name)
{
	char text[MAXIMUM_TAG_NAME_LENGTH];
	char *part;
	long length;

	snprintf(text, sizeof(text), "%s", field_name ? field_name : "");
	length = (long)strlen(text);
	if (length >= 10 && !strcmp(text + length - 10, "_reference"))
		text[length - 10] = 0;
	part = strrchr(text, '.');
	if (part && (!strcmp(part + 1, "reference") || !part[1]))
	{
		*part = 0;
		part = strrchr(text, '.');
	}
	leaf_clean(leaf, part ? part + 1 : text, MAXIMUM_TAG_NAME_LENGTH);
}

/* the folder new names are made in for a tag of the path `name`: its
folder, under the map's name (the map's alone when it would be too long) */
static void prefix_of(
	struct names_state const *state,
	char *prefix,
	char const *name)
{
	char const *slash = strrchr(name, '\\');
	long map_length = (long)strlen(state->map);
	long length = slash ? (long)(slash - name) : 0;
	long same;

	if (length > MAXIMUM_PREFIX_LENGTH)
		length = 0;
	for (same = 0; same < map_length && name[same] == state->map[same]; same++)
	{
	}
	/* (a made name's folder is the map's already) */
	if (length >= map_length && same == map_length && (name[map_length] == '\\' || length == map_length))
	{
		snprintf(prefix, MAXIMUM_TAG_NAME_LENGTH, "%.*s", (int)length, name);
		return;
	}
	if (length)
		snprintf(prefix, MAXIMUM_TAG_NAME_LENGTH, "%s\\%.*s", state->map, (int)length, name);
	else
		snprintf(prefix, MAXIMUM_TAG_NAME_LENGTH, "%s", state->map);
}

/* Gives a tag a name: prefix\leaf (an object a level places
prefix\leaf\leaf), its leaf numbered while another tag of its group has the
name. */
static void name_make(
	struct names_state *state,
	long tag,
	char const *prefix,
	char const *leaf)
{
	unsigned long group_tag = state->instances[tag].group_tag;
	char candidate[MAXIMUM_TAG_NAME_LENGTH];
	char numbered[MAXIMUM_LEAF_LENGTH + 8];
	boolean doubled = FALSE;
	long number;
	long kind;

	/* (an object of a kind is a folder of its own, as the stock tags are:
	weapons\plasma rifle\plasma rifle) */
	for (kind = 0; kind < (long)NUMBEROF(names_kinds); kind++)
		doubled = doubled || names_kinds[kind].group_tag == group_tag;
	for (number = 1; number <= MAXIMUM_NAME_NUMBER; number++)
	{
		if (number == 1)
			snprintf(numbered, sizeof(numbered), "%s", leaf[0] ? leaf : "tag");
		else
			snprintf(numbered, sizeof(numbered), "%s %ld", leaf[0] ? leaf : "tag", number);
		if (doubled)
			snprintf(candidate, sizeof(candidate), "%s\\%s\\%s", prefix, numbered, numbered);
		else
			snprintf(candidate, sizeof(candidate), "%s\\%s", prefix, numbered);
		if (!names_taken(state, group_tag, candidate))
			break;
	}
	if (number > MAXIMUM_NAME_NUMBER)
		return;
	/* (room for one name a tag: tag_names_restore) */
	if (state->pool_used + strlen(candidate) + 1 > state->pool_size)
		return;
	memcpy(state->pool + state->pool_used, candidate, strlen(candidate) + 1);
	state->instances[tag].name = state->pool + state->pool_used;
	state->pool_used += (unsigned long)strlen(candidate) + 1;
	state->status[tag] = _name_made;
	names_take(state, group_tag, state->instances[tag].name, tag);
}

/* ---------- the walk of a tag's references */

static struct tag_schema_group const *names_schema_group(
	unsigned long group_tag)
{
	struct tag_schema_group const *const *list;
	struct tag_schema_group const *group;

	/* (a Custom Edition map's groups laid out otherwise first, as the
	validator takes them: tag_bundle.c) */
	for (group = tag_schema_custom_edition_groups; group->group_tag; group++)
	{
		if (group->group_tag == group_tag)
			return group;
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

/* each tag an element's references and tag indices name, in its schema's
order, to `proc` (the validator checked every block, count and index) */
static void names_walk_fields(
	struct names_state *state,
	byte const *base,
	struct tag_schema_definition const *definition,
	short depth,
	short reference_depth,
	names_reference_proc proc)
{
	struct tag_schema_field const *field;

	if (!base || !definition || depth >= MAXIMUM_WALK_DEPTH)
		return;
	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short item;

		for (item = 0; item < field->count; item++)
		{
			byte const *place = base + field->offset + item * field->size;

			switch (field->type)
			{
			case _tag_schema_struct:
				names_walk_fields(state, place, field->definition, (short)(depth + 1), reference_depth, proc);
				break;

			case _tag_schema_block:
			{
				struct tag_block const *block = (struct tag_block const *)place;
				struct tag_schema_definition const *element = field->definition;
				long index;

				for (index = 0; element && block->address && index < block->count; index++)
				{
					names_walk_fields(state, (byte const *)block->address + index * element->size, element,
						(short)(depth + 1), reference_depth, proc);
				}
				break;
			}

			case _tag_schema_reference:
			{
				long child = names_tag(state, ((struct tag_reference const *)place)->index);

				if (child != NONE)
					proc(state, child, field->name, reference_depth);
				break;
			}

			case _tag_schema_tag_index:
			{
				long child = names_tag(state, *(long const *)place);

				if (child != NONE)
					proc(state, child, field->name, reference_depth);
				break;
			}

			default:
				break;
			}
		}
	}
}

static void names_walk(
	struct names_state *state,
	long tag,
	short reference_depth,
	names_reference_proc proc)
{
	struct names_instance const *instance = &state->instances[tag];
	struct tag_schema_group const *group = names_schema_group(instance->group_tag);

	if (group && instance->base_address)
		names_walk_fields(state, instance->base_address, group->definition, 0, reference_depth, proc);
}

/* (the reference whose field state->found_field is: state->found) */
static void reference_find_proc(
	struct names_state *state,
	long child,
	char const *field_name,
	short depth)
{
	(void)depth;
	if (state->found == NONE && field_name && !strcmp(field_name, state->found_field))
		state->found = child;
}

/* the tag a tag's reference of the field `field_name` names, or NONE */
static long reference_find(
	struct names_state *state,
	long tag,
	char const *field_name)
{
	state->found = NONE;
	state->found_field = field_name;
	names_walk(state, tag, 0, reference_find_proc);

	return state->found;
}

/* (each tag of its own name under the stock folder looked for counted by
the folder's next part, and those it names looked through, as far as
STOCK_FOLDER_DEPTH) */
static void stock_folder_proc(
	struct names_state *state,
	long child,
	char const *field_name,
	short depth)
{
	char const *name = state->instances[child].name;

	(void)field_name;
	if (state->stamps[child] == state->stamp)
		return;
	state->stamps[child] = state->stamp;
	if (state->status[child] == _name_kept && name && names_in_folder(name, state->stock_folder))
	{
		char const *part = name + strlen(state->stock_folder) + 1;
		char const *end = strchr(part, '\\');
		char leaf[MAXIMUM_LEAF_LENGTH + 1];
		short candidate;

		leaf_clean(leaf, part, end ? (long)(end - part) : (long)strlen(part));
		for (candidate = 0; leaf[0] && candidate < state->candidate_count &&
			strcmp(state->candidates[candidate].name, leaf); candidate++)
		{
		}
		if (leaf[0] && candidate < state->candidate_count)
			state->candidates[candidate].count++;
		else if (leaf[0] && candidate < MAXIMUM_STOCK_CANDIDATES)
		{
			snprintf(state->candidates[candidate].name, sizeof(state->candidates[candidate].name), "%s", leaf);
			state->candidates[candidate].count = 1;
			state->candidate_count++;
		}
	}
	if (depth + 1 < STOCK_FOLDER_DEPTH)
		names_walk(state, child, (short)(depth + 1), stock_folder_proc);
}

/* the stock folder an object's tags with names of their own are most of
(under `folder`: characters\elite\...'s "elite"), or "" */
static void stock_folder_leaf(
	struct names_state *state,
	long tag,
	char const *folder,
	char *leaf)
{
	short best = NONE;
	short candidate;

	leaf[0] = 0;
	if (!folder)
		return;
	state->stamp++;
	state->stamps[tag] = state->stamp;
	state->stock_folder = folder;
	state->candidate_count = 0;
	names_walk(state, tag, 0, stock_folder_proc);
	for (candidate = 0; candidate < state->candidate_count; candidate++)
	{
		if (best == NONE || state->candidates[candidate].count > state->candidates[best].count)
			best = candidate;
	}
	if (best != NONE)
		snprintf(leaf, MAXIMUM_LEAF_LENGTH + 1, "%s", state->candidates[best].name);
}

/* an item's pickup text, as the HUD says it, made its name: "Picked up a
Battle Rifle" and "Picked up %d rounds for Battle Rifle" are "battle rifle";
"" for none, or a text that is no item's (a placeholder, <Something Should
Go Here>, or a message, Checkpoint...) */
static void item_text_leaf(
	struct names_state const *state,
	long tag,
	char *leaf)
{
	static char const *const articles[] = { "a ", "an ", "the ", "some " };
	struct names_instance const *instance = &state->instances[tag];
	struct item_definition const *item = instance->base_address;
	struct string_list_entry const *entry;
	unsigned short const *text;
	char words[MAXIMUM_TAG_NAME_LENGTH];
	char const *after;
	boolean skip = FALSE;
	short dots = 0;
	long count;
	long index;

	leaf[0] = 0;
	if (!state->item_texts || !item || (instance->group_tag != ITEM_GROUP_TAG &&
		instance->parent_group_tags[0] != ITEM_GROUP_TAG && instance->parent_group_tags[1] != ITEM_GROUP_TAG))
		return;
	index = item->item.hud_message_index;
	if (index < 0 || index >= state->item_texts->strings.count || !state->item_texts->strings.address)
		return;
	entry = (struct string_list_entry const *)state->item_texts->strings.address + index;
	text = entry->string.address;
	count = entry->string.size / (long)sizeof(unsigned short);
	words[0] = 0;
	for (index = 0; text && index < count && text[index]; index++)
	{
		dots = (short)(text[index] == '.' ? dots + 1 : 0);
		if (text[index] == '<' || dots == 3)
			return;
		leaf_append(words, text[index], index + 1 < count ? text[index + 1] : 0, &skip, MAXIMUM_TAG_NAME_LENGTH - 1);
	}
	leaf_trim(words);
	/* (Halo PC's are "Picked up ...", a map's misspelt too; its ammunition's
	"... rounds for" the weapon) */
	if (!text_strip(words, "picked up "))
		text_strip(words, "piked up ");
	for (after = strstr(words, " for "); after && strstr(after + 1, " for "); after = strstr(after + 1, " for "))
	{
	}
	if (after)
	{
		after += 5;
		for (index = 0; after[index]; index++)
			words[index] = after[index];
		words[index] = 0;
	}
	for (index = 0; index < (long)NUMBEROF(articles); index++)
	{
		if (text_strip(words, articles[index]))
			break;
	}
	leaf_clean(leaf, words, MAXIMUM_TAG_NAME_LENGTH);
}

/* the HUD's item texts: the string list its globals name, or NULL */
static struct string_list const *item_texts_find(
	struct names_state const *state)
{
	long tag;

	for (tag = 0; tag < state->tag_count; tag++)
	{
		struct names_instance const *instance = &state->instances[tag];
		struct hud_globals_definition const *globals = instance->base_address;
		long texts;

		if (instance->group_tag != HUD_GLOBALS_GROUP_TAG || !globals)
			continue;
		texts = names_tag(state, globals->messaging.hud_item_messages.index);
		if (texts != NONE && state->instances[texts].group_tag == UNICODE_STRING_LIST_GROUP_TAG)
			return state->instances[texts].base_address;
	}

	return NULL;
}

/* ---------- the names made */

/* the objects a level places, of a kind, in the table's order */
static void objects_name(
	struct names_state *state,
	long kind)
{
	char prefix[MAXIMUM_TAG_NAME_LENGTH];
	long tag;

	snprintf(prefix, sizeof(prefix), "%s\\%s", state->map, names_kinds[kind].folder);
	for (tag = 0; tag < state->tag_count; tag++)
	{
		char leaf[MAXIMUM_LEAF_LENGTH + 1];

		if (state->status[tag] != _name_none || state->instances[tag].group_tag != names_kinds[kind].group_tag)
			continue;
		item_text_leaf(state, tag, leaf);
		if (!leaf[0])
			stock_folder_leaf(state, tag, names_kinds[kind].stock_folder, leaf);
		if (!leaf[0])
			snprintf(leaf, sizeof(leaf), "%s", names_kinds[kind].kind);
		name_make(state, tag, prefix, leaf);
	}
}

/* the actor variants: in their units' folders, by their units' names and
their weapons' */
static void actor_variants_name(
	struct names_state *state)
{
	long tag;

	for (tag = 0; tag < state->tag_count; tag++)
	{
		char prefix[MAXIMUM_TAG_NAME_LENGTH];
		char leaf[MAXIMUM_LEAF_LENGTH + 1];
		char weapon_leaf[MAXIMUM_LEAF_LENGTH + 1];
		long unit;
		long weapon;

		if (state->status[tag] != _name_none || state->instances[tag].group_tag != ACTOR_VARIANT_GROUP_TAG)
			continue;
		unit = reference_find(state, tag, "unit_reference");
		weapon = reference_find(state, tag, "ranged_combat.reference");
		weapon_leaf[0] = 0;
		if (weapon != NONE && names_name(state, weapon)[0])
			leaf_of(weapon_leaf, names_name(state, weapon));
		if (unit != NONE && names_name(state, unit)[0])
		{
			char unit_leaf[MAXIMUM_LEAF_LENGTH + 1];

			prefix_of(state, prefix, names_name(state, unit));
			leaf_of(unit_leaf, names_name(state, unit));
			snprintf(leaf, sizeof(leaf), "%.*s%s%.*s", MAXIMUM_LEAF_LENGTH / 2, unit_leaf, weapon_leaf[0] ? " " : "",
				MAXIMUM_LEAF_LENGTH / 2 - 1, weapon_leaf);
		}
		else
		{
			snprintf(prefix, sizeof(prefix), "%s\\characters", state->map);
			snprintf(leaf, sizeof(leaf), "%s", weapon_leaf[0] ? weapon_leaf : "actor variant");
		}
		leaf_trim(leaf);
		name_make(state, tag, prefix, leaf);
	}
}

static void queue_add(
	struct names_state *state,
	long tag)
{
	if (state->queued[tag] || state->status[tag] == _name_none)
		return;
	state->queued[tag] = TRUE;
	state->queue[state->queue_count++] = tag;
}

/* (a tag without a name, named in the folder of the one naming it) */
static void child_name_proc(
	struct names_state *state,
	long child,
	char const *field_name,
	short depth)
{
	char prefix[MAXIMUM_TAG_NAME_LENGTH];
	char leaf[MAXIMUM_LEAF_LENGTH + 1];

	(void)depth;
	if (state->status[child] != _name_none)
		return;
	prefix_of(state, prefix, names_name(state, state->parent));
	field_leaf(leaf, field_name);
	name_make(state, child, prefix, leaf);
	queue_add(state, child);
}

/* every tag named from those with names: the objects', the actor
variants', then the rest's, in the table's order, each tag's before the
tags they name */
static void children_name(
	struct names_state *state)
{
	long head;
	long kind;
	long tag;

	for (kind = 0; kind < (long)NUMBEROF(names_kinds); kind++)
	{
		for (tag = 0; tag < state->tag_count; tag++)
		{
			if (state->instances[tag].group_tag == names_kinds[kind].group_tag)
				queue_add(state, tag);
		}
		/* (the actor variants after the equipment their AI carry, before
		their units) */
		if (names_kinds[kind].group_tag == 'garb')
		{
			for (tag = 0; tag < state->tag_count; tag++)
			{
				if (state->instances[tag].group_tag == ACTOR_VARIANT_GROUP_TAG)
					queue_add(state, tag);
			}
		}
	}
	for (tag = 0; tag < state->tag_count; tag++)
		queue_add(state, tag);
	for (head = 0; head < state->queue_count; head++)
	{
		state->parent = state->queue[head];
		names_walk(state, state->parent, 0, child_name_proc);
	}
}

static void state_dispose(
	struct names_state *state)
{
	/* (the game's free takes no NULL: cseries.h) */
	if (state->status)
		free(state->status);
	if (state->table)
		free(state->table);
	if (state->queue)
		free(state->queue);
	if (state->queued)
		free(state->queued);
	if (state->stamps)
		free(state->stamps);
}

/* ---------- public code */

long tag_names_restore(
	void *tag_header,
	char const *map_name,
	unsigned char **made)
{
	struct names_header *header = tag_header;
	struct names_state state;
	long unnamed = 0;
	long count = 0;
	long kind;
	long tag;

	if (made)
		*made = NULL;
	/* (the names made before are let go once others take their place: the
	table may still name them, a table of its own names alone) */
	if (!header || header->tag_count <= 0)
		return 0;
	memset(&state, 0, sizeof(state));
	state.instances = header->instances;
	state.tag_count = header->tag_count;
	leaf_clean(state.map, map_name ? map_name : "", MAXIMUM_TAG_NAME_LENGTH);
	if (!state.map[0])
		snprintf(state.map, sizeof(state.map), "map");
	for (state.table_size = 16; state.table_size < (unsigned long)state.tag_count * 2; state.table_size *= 2)
	{
	}
	state.status = malloc((unsigned long)state.tag_count);
	state.table = malloc(state.table_size * sizeof(*state.table));
	state.queue = malloc((unsigned long)state.tag_count * sizeof(long));
	state.queued = malloc((unsigned long)state.tag_count);
	state.stamps = malloc((unsigned long)state.tag_count * sizeof(long));
	if (!state.status || !state.table || !state.queue || !state.queued || !state.stamps)
	{
		state_dispose(&state);
		return NONE;
	}
	memset(state.queued, 0, (unsigned long)state.tag_count);
	memset(state.stamps, 0, (unsigned long)state.tag_count * sizeof(long));

	/* the names of their own: a resource map's tags', and each path no other
	tag of its group has */
	memset(state.table, 0, state.table_size * sizeof(*state.table));
	for (tag = 0; tag < state.tag_count; tag++)
	{
		struct names_instance const *instance = &state.instances[tag];
		struct names_entry *entry;

		state.status[tag] = instance->in_resource_map || name_usable(instance->name) ? _name_kept : _name_none;
		if (state.status[tag] == _name_none)
			continue;
		entry = names_slot(&state, instance->group_tag, instance->name);
		if (!entry->name)
		{
			names_take(&state, instance->group_tag, instance->name, tag);
			continue;
		}
		/* (a resource map's keeps it: the other is none's) */
		if (!state.instances[entry->tag].in_resource_map)
		{
			state.status[entry->tag] = _name_none;
			entry->tag = tag;
		}
		if (!instance->in_resource_map)
			state.status[tag] = _name_none;
	}
	memset(state.table, 0, state.table_size * sizeof(*state.table));
	for (tag = 0; tag < state.tag_count; tag++)
	{
		if (state.status[tag] == _name_kept)
			names_take(&state, state.instances[tag].group_tag, state.instances[tag].name, tag);
		else
			unnamed++;
	}
	if (!unnamed)
	{
		state_dispose(&state);
		return 0;
	}
	state.pool_size = (unsigned long)unnamed * MAXIMUM_TAG_NAME_LENGTH;
	state.pool = malloc(state.pool_size);
	if (!state.pool)
	{
		state_dispose(&state);
		return NONE;
	}
	state.item_texts = item_texts_find(&state);

	for (kind = 0; kind < (long)NUMBEROF(names_kinds); kind++)
		objects_name(&state, kind);
	actor_variants_name(&state);
	tag = names_tag(&state, header->scenario_tag_index);
	if (tag != NONE && state.status[tag] == _name_none && state.instances[tag].group_tag == SCENARIO_GROUP_TAG)
		name_make(&state, tag, state.map, state.map);
	for (tag = 0; tag < state.tag_count; tag++)
	{
		if (state.status[tag] == _name_none && state.instances[tag].group_tag == GLOBALS_GROUP_TAG)
			name_make(&state, tag, state.map, "globals");
	}
	children_name(&state);
	for (tag = 0; tag < state.tag_count; tag++)
	{
		char prefix[MAXIMUM_TAG_NAME_LENGTH];
		char group[5];
		char leaf[MAXIMUM_LEAF_LENGTH + 1];
		short character;

		if (state.status[tag] != _name_none)
			continue;
		for (character = 0; character < 4; character++)
			group[character] = (char)(state.instances[tag].group_tag >> (24 - 8 * character));
		group[4] = 0;
		snprintf(prefix, sizeof(prefix), "%s\\unnamed", state.map);
		leaf_clean(leaf, group, 4);
		name_make(&state, tag, prefix, leaf);
	}

	for (tag = 0; tag < state.tag_count; tag++)
		count += state.status[tag] == _name_made;
	if (made)
	{
		*made = malloc((unsigned long)state.tag_count);
		for (tag = 0; *made && tag < state.tag_count; tag++)
			(*made)[tag] = state.status[tag] == _name_made;
	}
	tag_names_dispose();
	tag_names_pool = state.pool;
	state_dispose(&state);

	return count;
}

void tag_names_dispose(
	void)
{
	if (tag_names_pool)
		free(tag_names_pool);
	tag_names_pool = NULL;
}
