/*
TEXT_GROUP.C

symbols in this file:
0018CC10 0060:
	_string_list_get_string (0000)
0018CC70 0060:
	_unicode_string_list_get_string (0000)
002A2A90 0011:
	??_C@_0BB@DGIKDNEK@?$DMmissing?5string?$DO?$AA@ (0000)
002A2AA4 0022:
	??_C@_1CC@IMCEGIAL@?$AA?$DM?$AAm?$AAi?$AAs?$AAs?$AAi?$AAn?$AAg?$AA?5?$AAs?$AAt?$AAr?$AAi?$AAn?$AAg?$AA?$DO?$AA?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "text/text_group.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

char *string_list_get_string(long tag_index, short string_index)
{
	char *result = "<missing string>";

	if (tag_index != NONE)
	{
		struct string_list *list = string_list_definition_get(tag_index);

		if (string_index >= 0 && string_index < list->strings.count)
		{
			struct string_list_entry *entry = TAG_BLOCK_GET_ELEMENT(
				&list->strings,
				string_index,
				struct string_list_entry);

			if (entry->string.size > 0)
			{
				result = entry->string.address;
				result[entry->string.size - 1] = '\0';
			}
		}
	}

	return result;
}

wchar_t *unicode_string_list_get_string(long tag_index, short string_index)
{
	wchar_t *result = L"<missing string>";

	if (tag_index != NONE)
	{
		struct string_list *list = unicode_string_list_definition_get(tag_index);

		if (string_index >= 0 && string_index < list->strings.count)
		{
			struct string_list_entry *entry = TAG_BLOCK_GET_ELEMENT(
				&list->strings,
				string_index,
				struct string_list_entry);

			if (entry->string.size > 0)
			{
				result = entry->string.address;
				result[entry->string.size / sizeof(wchar_t) - 1] = L'\0';
			}
		}
	}

	return result;
}

/* ---------- private code */
