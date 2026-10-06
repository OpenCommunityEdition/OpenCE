/*
FONT_GROUP.C

symbols in this file:
0018C840 0070:
	_font_get_character_by_ascii_code (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "font_group.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

struct font_character *font_get_character_by_ascii_code(
	struct font_header *font,
	word character_code)
{
	struct font_character_table *character_table;
	struct font_character *character = NULL;

	/* port: no glyph for a character past the font's tables, a table that
	isn't a whole 256 entries, or an entry past the font's characters (the
	map's font, and any string's characters, e.g. a player's name) */
	if (!VALID_INDEX(character_code >> 8, font->character_tables.count))
		return NULL;

	character_table = TAG_BLOCK_GET_ELEMENT(
		&font->character_tables,
		character_code >> 8,
		struct font_character_table);

	if (character_table->character_indices.count == 256)
	{
		short *character_index = TAG_BLOCK_GET_ELEMENT(&character_table->character_indices, character_code & 0xFF, short);

		if (VALID_INDEX(*character_index, font->characters.count))
		{
			character = tag_block_get_element_with_size(
				&font->characters,
				*character_index,
				FONT_CHARACTER_SIZE);
		}
	}

	return character;
}

/* ---------- private code */
