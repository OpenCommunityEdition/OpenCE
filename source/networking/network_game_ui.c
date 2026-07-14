/*
NETWORK_GAME_UI.C

symbols in this file:
0011AE30 0060:
	_network_game_get_random_player_name (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

wchar_t const *unicode_string_list_get_string(long tag_index, short string_index);

/* ---------- globals */

/* ---------- public code */

wchar_t const *network_game_get_random_player_name(
	void)
{
	wchar_t const *player_name = L"";
	long string_list_index = tag_loaded('ustr', "ui\\random_player_names");

	if (string_list_index != NONE)
	{
		struct tag_block *string_list = tag_get('ustr', string_list_index);

		if (string_list)
		{
			short string_index = seed_random_range(
				get_global_local_random_seed_address(),
				0,
				string_list->count - 1);
			player_name = unicode_string_list_get_string(string_list_index, string_index);
		}
	}

	return player_name;
}

/* ---------- private code */
