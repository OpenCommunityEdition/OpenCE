/*
MOD_STRINGS.C

The text of mods (mods.c): each of a map's string lists (unicode string
lists, the text the game shows, and string lists) that a mod has a tag file
of (tags\...\name.unicode_string_list, name.string_list) shows that file's
strings instead of the map's, a translation or other words, and
mods_reload reads them again.

A tag file is Halo PC's (big-endian, version 1): the list, its strings, then
each string's characters (UTF-16 for a unicode string list, as the tool
writes them, else bytes). Every count and size is checked against the file,
which must end where its last string does. The game finds a string by its
index and reads many as the format of what it prints ("%s was killed by
%s"), so a list keeps the map's number of strings, and a string replaces
the map's only where it has the same conversions in the same order (any
other would read what printf was not given): the map's own string stays
where the file's has not got them, or has none at that index. Each is
terminated, and a unicode one ended where the game's text drawing would read
past it (tag_schema_unicode_string_end), as the validator does a map's.

The map's list takes the file's strings in place, so everything that holds
the tag keeps holding it, and its own come back when the file goes. A list
made is kept until the map goes, as what is shown may hold its strings.
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "cache/cache_files.h"
#include "main/console.h"
#include "tag_files/tag_groups.h"
#include "text/text_group.h"
#include "tag_schema.h"
#include "mods.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* the game's (port) */
char *tag_get_name(long tag_index);

/* ---------- constants */

/* (text_group.c's string_list and unicode_string_list groups) */
#define STRING_LIST_TAG_VERSION 1
#define UNICODE_STRING_LIST_TAG_VERSION 1

/* the sizes of a tag file's list and string (text_group.h's structures, as
the file has them) */
#define STRING_LIST_SIZE 0x0C
#define STRING_LIST_ENTRY_SIZE 0x14

/* the validator's limits (tag_schema_effects.c) */
#define MAXIMUM_STRINGS SHORT_MAX
#define MAXIMUM_STRING_BYTES (2 * SHORT_MAX)

/* the most conversions a string's format is compared by */
#define MAXIMUM_CONVERSIONS 32

typedef char verify_string_list_size[sizeof(struct string_list) == STRING_LIST_SIZE ? 1 : -1];
typedef char verify_string_list_entry_size[sizeof(struct string_list_entry) == STRING_LIST_ENTRY_SIZE ? 1 : -1];

/* ---------- structures */

/* a group of lists: its tag, the extension of its files, and whether its
characters are UTF-16 */
struct string_group
{
	unsigned long group_tag;
	char const *extension;
	short version;
	boolean unicode;
};

/* a list's tag file read, and what it makes */
struct mod_string_list
{
	struct mod_string_list *next;
	long tag_index;
	/* the file, kept to tell whether it changed */
	byte *file;
	unsigned long file_size;
	/* the list it makes: the map's number of strings, the file's where they
	may be, and the characters of the file's */
	struct string_list list;
	struct string_list_entry *entries;
	byte *characters;
	long replaced_count;
	unsigned long kept_bytes;
};

/* a list of the map's shown from a file: the map's own, given back when it
goes */
struct mod_string_override
{
	struct mod_string_list *list;
	struct string_list map_list;
};

/* a conversion of a format, as it reads its arguments */
struct conversion
{
	/* the conversion, its length modifier, and how many '*' (each an
	argument) it has */
	char conversion;
	char modifier[3];
	short stars;
};

/* ---------- globals */

static struct string_group const string_groups[] =
{
	{ UNICODE_STRING_LIST_TAG, ".unicode_string_list", UNICODE_STRING_LIST_TAG_VERSION, TRUE },
	{ STRING_LIST_TAG, ".string_list", STRING_LIST_TAG_VERSION, FALSE },
};

static struct
{
	/* by the map's tag, its override (none: the map's own) */
	struct mod_string_override *overrides;
	long override_count;
	/* every list made while the map is loaded */
	struct mod_string_list *lists;
} mod_strings_globals;

/* ---------- private code */

static void mod_string_list_free(
	struct mod_string_list *list)
{
	/* (the game's free, debug_free, does not take NULL) */
	if (list->file)
		free(list->file);
	if (list->entries)
		free(list->entries);
	if (list->characters)
		free(list->characters);
	free(list);

	return;
}

/* the character at `index` of a string of `size` bytes */
static unsigned short character_get(
	byte const *string,
	unsigned long size,
	unsigned long index,
	boolean unicode)
{
	if (unicode)
	{
		return 2 * index + 1 < size ? (unsigned short)(string[2 * index] | string[2 * index + 1] << 8) : 0;
	}

	return index < size ? string[index] : 0;
}

/* whether a character is one of the ASCII characters of `set` */
static boolean character_in(
	unsigned short character,
	char const *set)
{
	return character && character < 0x80 && strchr(set, (char)character) != NULL;
}

/* the conversions of a printf format (a string's of `size` bytes) up to its
terminator: how many, or NONE for more than *conversions holds */
static long format_conversions(
	byte const *string,
	unsigned long size,
	boolean unicode,
	struct conversion *conversions)
{
	unsigned long length = unicode ? size / 2 : size;
	unsigned long index = 0;
	long count = 0;

	while (index < length && character_get(string, size, index, unicode))
	{
		struct conversion *conversion;
		unsigned short character;
		short modifier_length = 0;

		if (character_get(string, size, index, unicode) != '%')
		{
			index++;
			continue;
		}
		index++;
		if (character_get(string, size, index, unicode) == '%')
		{
			index++;
			continue;
		}
		if (count == MAXIMUM_CONVERSIONS)
		{
			return NONE;
		}
		conversion = &conversions[count++];
		memset(conversion, 0, sizeof(*conversion));
		/* (flags, width and precision, then the length modifier) */
		while (character_in(character = character_get(string, size, index, unicode), "-+ #0123456789.*hlLqjzt"))
		{
			if (character == '*')
			{
				conversion->stars++;
			}
			else if (character_in(character, "hlLqjzt") && modifier_length < (short)sizeof(conversion->modifier) - 1)
			{
				conversion->modifier[modifier_length++] = (char)character;
			}
			index++;
		}
		conversion->conversion = (char)MIN(character, 0x7F);
		if (character)
		{
			index++;
		}
	}

	return count;
}

/* whether two strings read the same arguments as formats */
static boolean formats_match(
	byte const *string,
	unsigned long size,
	byte const *map_string,
	unsigned long map_size,
	boolean unicode)
{
	struct conversion conversions[MAXIMUM_CONVERSIONS];
	struct conversion map_conversions[MAXIMUM_CONVERSIONS];
	long count = format_conversions(string, size, unicode, conversions);
	long map_count = format_conversions(map_string, map_size, unicode, map_conversions);
	long index;

	if (count == NONE || count != map_count)
	{
		return FALSE;
	}
	for (index = 0; index < count; index++)
	{
		if (conversions[index].conversion != map_conversions[index].conversion ||
			conversions[index].stars != map_conversions[index].stars ||
			strcmp(conversions[index].modifier, map_conversions[index].modifier))
		{
			return FALSE;
		}
	}

	return TRUE;
}

/* The list the tag file `file` (`file_size` bytes) of group `group` makes
for the map's list `tag_index`, whose strings are `map_list`'s, or NULL with
*reason saying why not. */
static struct mod_string_list *mod_string_list_new(
	long tag_index,
	struct string_group const *group,
	byte *file,
	unsigned long file_size,
	struct string_list const *map_list,
	char const **reason)
{
	struct mod_string_list *list;
	unsigned long cursor = MOD_TAG_FILE_HEADER_SIZE + STRING_LIST_SIZE;
	unsigned long string_count;
	unsigned long data_cursor;
	unsigned long characters_size = 0;
	unsigned long character_cursor = 0;
	unsigned long terminator = group->unicode ? 2 : 1;
	unsigned long index;

	*reason = "not a string list tag file of this version";
	if (file_size < cursor || !mods_tag_file_header_valid(file, group->group_tag, group->version))
	{
		return NULL;
	}
	list = malloc(sizeof(*list));
	if (!list)
	{
		*reason = "out of memory";
		return NULL;
	}
	memset(list, 0, sizeof(*list));
	list->tag_index = tag_index;
	string_count = mods_read_be32(file + MOD_TAG_FILE_HEADER_SIZE + offsetof(struct string_list, strings.count));
	*reason = "its strings are not in the file";
	if (string_count > MAXIMUM_STRINGS || !mods_in_file(cursor, string_count * STRING_LIST_ENTRY_SIZE, file_size))
	{
		goto refused;
	}

	/* each string's characters, which must end the file */
	data_cursor = cursor + string_count * STRING_LIST_ENTRY_SIZE;
	for (index = 0; index < string_count; index++)
	{
		unsigned long size = mods_read_be32(file + cursor + index * STRING_LIST_ENTRY_SIZE +
			offsetof(struct string_list_entry, string.size));

		if (size > MAXIMUM_STRING_BYTES || (group->unicode && size % 2) || !mods_in_file(data_cursor, size, file_size))
		{
			goto refused;
		}
		data_cursor += size;
		characters_size += size + terminator;
	}
	*reason = "the file goes on past its last string";
	if (data_cursor != file_size)
	{
		goto refused;
	}

	/* the map's number of strings: the file's where they read what the
	map's do */
	*reason = "out of memory";
	list->entries = malloc(MAX(map_list->strings.count, 1) * sizeof(*list->entries));
	list->characters = malloc(MAX(characters_size, 1));
	if (!list->entries || !list->characters)
	{
		goto refused;
	}
	data_cursor = cursor + string_count * STRING_LIST_ENTRY_SIZE;
	for (index = 0; index < (unsigned long)map_list->strings.count; index++)
	{
		struct string_list_entry const *map_entry =
			TAG_BLOCK_GET_ELEMENT(&map_list->strings, index, struct string_list_entry);
		struct string_list_entry *entry = &list->entries[index];

		*entry = *map_entry;
		if (index < string_count)
		{
			unsigned long size = mods_read_be32(file + cursor + index * STRING_LIST_ENTRY_SIZE +
				offsetof(struct string_list_entry, string.size));
			byte const *string = file + data_cursor;

			data_cursor += size;
			if (map_entry->string.size > 0 && map_entry->string.address &&
				formats_match(string, size, map_entry->string.address, (unsigned long)map_entry->string.size,
					group->unicode))
			{
				byte *characters = list->characters + character_cursor;

				memcpy(characters, string, size);
				memset(characters + size, 0, terminator);
				character_cursor += size + terminator;
				if (group->unicode)
				{
					tag_schema_unicode_string_end((wchar_t *)characters, (long)((size + terminator) / 2));
				}
				entry->string.size = (long)(size + terminator);
				entry->string.address = characters;
				entry->string.file_offset = 0;
				entry->string.definition = NULL;
				list->replaced_count++;
			}
		}
	}
	list->list.strings.count = map_list->strings.count;
	list->list.strings.address = map_list->strings.count ? list->entries : NULL;
	list->list.strings.definition = NULL;
	list->file = file;
	list->file_size = file_size;
	list->kept_bytes = sizeof(*list) + file_size + MAX(map_list->strings.count, 1) * sizeof(*list->entries) +
		MAX(characters_size, 1);
	if ((long)string_count > map_list->strings.count || list->replaced_count < MIN((long)string_count, map_list->strings.count))
	{
		error(_error_silent, "mods: %ld of the %ld strings of %s replace the map's %ld; the rest are past them, or read other arguments",
			list->replaced_count, (long)string_count, tag_get_name(tag_index), map_list->strings.count);
	}

	return list;

refused:
	list->file = NULL;
	mod_string_list_free(list);

	return NULL;
}

/* a list made before for the tag `tag_index` from a file of these bytes,
shown or not, or NULL */
static struct mod_string_list *list_kept(
	long tag_index,
	byte const *file,
	unsigned long file_size)
{
	struct mod_string_list *list;

	for (list = mod_strings_globals.lists; list; list = list->next)
	{
		if (list->tag_index == tag_index && list->file_size == file_size && !memcmp(list->file, file, file_size))
			return list;
	}

	return NULL;
}

/* the override of the tag `tag_index` */
static struct mod_string_override *override_get(
	long tag_index)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index);

	return absolute_index >= 0 && absolute_index < mod_strings_globals.override_count ?
		&mod_strings_globals.overrides[absolute_index] : NULL;
}

/* Makes the table of overrides, one for each of the map's tags up to its
last string list; FALSE when it cannot be. */
static boolean overrides_new(
	void)
{
	long count = 0;
	short group_index;

	if (mod_strings_globals.overrides)
	{
		return TRUE;
	}
	for (group_index = 0; group_index < (short)NUMBEROF(string_groups); group_index++)
	{
		struct tag_iterator iterator;
		long tag_index;

		tag_iterator_new(&iterator, string_groups[group_index].group_tag);
		while ((tag_index = tag_iterator_next(&iterator)) != NONE)
		{
			count = MAX(count, DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index) + 1);
		}
	}
	if (count <= 0 || !(mod_strings_globals.overrides = malloc((size_t)count * sizeof(struct mod_string_override))))
	{
		return FALSE;
	}
	memset(mod_strings_globals.overrides, 0, (size_t)count * sizeof(struct mod_string_override));
	mod_strings_globals.override_count = count;

	return TRUE;
}

/* ---------- public code */

long mod_strings_update(
	boolean remove_all,
	boolean at_load)
{
	long loaded_count = 0;
	long removed_count = 0;
	long refused_count = 0;
	long played_count = 0;
	short group_index;
	long tag_index;

	if (!mods_map_loaded() || !overrides_new())
	{
		return 0;
	}

	for (group_index = 0; group_index < (short)NUMBEROF(string_groups); group_index++)
	{
		struct string_group const *group = &string_groups[group_index];
		struct tag_iterator iterator;

		tag_iterator_new(&iterator, group->group_tag);
		while ((tag_index = tag_iterator_next(&iterator)) != NONE)
		{
			struct mod_string_override *override = override_get(tag_index);
			struct string_list *definition = tag_get(group->group_tag, tag_index);
			long file_index = remove_all ? NONE : mods_tag_file_find(tag_index, group->extension);
			struct mod_string_list *made = NULL;
			byte *file = NULL;
			unsigned long file_size = 0;
			char const *reason = NULL;

			/* (a tag of the other group, as tag_get gives it) */
			if (!override || !definition || tag_get_group_tag(tag_index) != group->group_tag)
			{
				continue;
			}
			if (file_index != NONE)
			{
				file = mods_tag_file_read(file_index, &file_size, &reason);
				/* (one made before from the same file, shown or not) */
				made = file ? list_kept(tag_index, file, file_size) : NULL;
				if (made)
				{
					free(file);
					if (made == override->list)
						continue;
				}
				else if (file)
				{
					made = mod_string_list_new(tag_index, group, file, file_size,
						override->list ? &override->map_list : definition, &reason);
					if (made && !mods_memory_take(made->kept_bytes))
					{
						reason = "the mods' tags already hold too much memory";
						made->file = NULL;
						mod_string_list_free(made);
						made = NULL;
					}
					if (!made)
					{
						free(file);
					}
				}
				if (!made)
				{
					error(_error_silent, "mods: %s left out: %s", port_mods_file_path(file_index), reason);
					console_warning("mods: %s%s of %s left out: %s", tag_get_name(tag_index), group->extension,
						port_mods_file_mod(file_index), reason);
					refused_count++;
					/* (one refused leaves what was shown before) */
					continue;
				}
			}
			else if (!override->list)
			{
				continue;
			}

			if (override->list)
			{
				memcpy(definition, &override->map_list, sizeof(*definition));
				override->list = NULL;
				if (!made)
					removed_count++;
			}
			if (made)
			{
				memcpy(&override->map_list, definition, sizeof(*definition));
				memcpy(definition, &made->list, sizeof(*definition));
				override->list = made;
				if (!list_kept(tag_index, made->file, made->file_size))
				{
					made->next = mod_strings_globals.lists;
					mod_strings_globals.lists = made;
				}
				loaded_count++;
			}
		}
	}

	for (tag_index = 0; tag_index < mod_strings_globals.override_count; tag_index++)
	{
		if (mod_strings_globals.overrides[tag_index].list)
			played_count++;
	}
	if (!at_load || loaded_count || refused_count)
	{
		error(_error_silent, "mods: %ld string lists from their files (%ld new), %ld given back, %ld left out",
			played_count, loaded_count, removed_count, refused_count);
		if (!at_load && mods_say())
		{
			console_printf(FALSE, "mods: %ld string lists from their files (%ld new), %ld given back, %ld left out",
				played_count, loaded_count, removed_count, refused_count);
		}
	}

	return played_count;
}

void mod_strings_tags_unloaded(
	void)
{
	struct mod_string_list *list = mod_strings_globals.lists;

	while (list)
	{
		struct mod_string_list *next = list->next;

		mod_string_list_free(list);
		list = next;
	}
	if (mod_strings_globals.overrides)
		free(mod_strings_globals.overrides);
	memset(&mod_strings_globals, 0, sizeof(mod_strings_globals));

	return;
}
