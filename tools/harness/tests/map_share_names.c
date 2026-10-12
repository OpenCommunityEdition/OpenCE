/*
MAP_SHARE_NAMES.C (test)

The real names of map sharing (port/linux/game/map_share_files.c): a map's
file name ends in _ and the first digits of its hash, which the menus leave
out and a download is checked against. test_map_share_names.py takes the
constants (config.inc) and the code (under_test.inc).
*/

#include "harness.h"
#include "config.inc"

#include <strings.h>
#include <wchar.h>

#define csstrlen(string) strlen(string)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define NUMBEROF(array) ((short)(sizeof(array) / sizeof((array)[0])))
#define ustrcpy(destination, source) wcscpy(destination, source)
#define _strnicmp(first, second, count) strncasecmp(first, second, count)

#include "under_test.inc"

/* the menus' name of a map file's name (custom_edition_maps.c) */
static char const *menu_name(char const *name, boolean hash)
{
	static wchar_t display_name[64];
	static char text[64];
	int index;

	display_name_make(name, hash, display_name);
	for (index = 0; display_name[index] && index < 63; index++)
		text[index] = (char)display_name[index];
	text[index] = 0;
	return text;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a name ends in _ and the hash's digits, lowercase hexadecimal */
	CASE("hashed-names")
	{
		CHECK(map_share_name_hashed("hugeass_0e2b3ee4"), "a hashed name was not taken for one");
		CHECK(map_share_name_hashed("a_00000000"), "a short hashed name was not taken for one");
		CHECK(!map_share_name_hashed("hugeass"), "a name without a hash was taken for one");
		CHECK(!map_share_name_hashed("hugeass_0E2B3EE4"), "uppercase digits were taken (the hash's are lowercase)");
		CHECK(!map_share_name_hashed("hugeass_0e2b3ee"), "seven digits were taken");
		CHECK(!map_share_name_hashed("hugeass0e2b3ee4"), "digits without _ were taken");
		CHECK(!map_share_name_hashed("hugeass_0e2b3eg4"), "a letter past f was taken");
		CHECK(!map_share_name_hashed("_0e2b3ee4"), "a hash with no name before it was taken");
		CHECK(!map_share_name_hashed(""), "nothing was taken");
		return 0;
	}
	/* a host's level name names a file of custom_maps alone */
	CASE("safe-names")
	{
		static char const *const unsafe[] =
		{
			"", ".", "..", ".x_0e2b3ee4", "../x_0e2b3ee4", "..\\x", "a/b", "c:x", "x.", " x", "x ", "a*b", "a?b", "a\"b", "a<b",
			"a>b", "a|b", "a\tb", "caf\xE9",
		};
		int index;

		CHECK(map_share_name_safe("hugeass_0e2b3ee4"), "a map's name was refused");
		CHECK(map_share_name_safe("Elite_Alpha_Siege (v2).final"), "a name with spaces and dots inside was refused");
		for (index = 0; index < (int)(sizeof(unsafe) / sizeof(unsafe[0])); index++)
			CHECK(!map_share_name_safe(unsafe[index]), "the name \"%s\" was taken", unsafe[index]);
		return 0;
	}
	/* the menus' name leaves the hash out */
	CASE("base-length")
	{
		CHECK(map_share_name_base_length("hugeass_0e2b3ee4") == 7, "gave %d", (int)map_share_name_base_length("hugeass_0e2b3ee4"));
		CHECK(map_share_name_base_length("the_bay_of_pigs") == 15, "a name without a hash was cut");
		CHECK(map_share_name_base_length("the_bay_of_pigs_12345678") == 15, "a name with _ in it lost more than its hash");
		return 0;
	}
	/* the digits are the hash's first bytes, as a name ends in them */
	CASE("digits")
	{
		static unsigned char const hash[32] = { 0x0E, 0x2B, 0x3E, 0xE4, 0xFF };
		char digits[MAP_SHARE_NAME_DIGITS + 1];
		char name[64];

		memset(digits, 'x', sizeof(digits));
		map_share_hash_digits(hash, digits);
		CHECK(!strcmp(digits, "0e2b3ee4"), "made %s", digits);
		snprintf(name, sizeof(name), "hugeass_%s", digits);
		CHECK(map_share_name_hashed(name), "a name made with the digits is not a hashed one");
		return 0;
	}
	/* a file's pieces: the last one shorter */
	CASE("pieces")
	{
		CHECK(map_share_piece_count(0) == 0, "an empty file has pieces");
		CHECK(map_share_piece_count(1) == 1, "one byte is not one piece");
		CHECK(map_share_piece_count(MAP_SHARE_PIECE_BYTES) == 1, "a piece's bytes are not one piece");
		CHECK(map_share_piece_count(MAP_SHARE_PIECE_BYTES + 1) == 2, "a byte more is not a second piece");
		CHECK(map_share_piece_count(59221172UL) == 226, "hugeass.map's pieces: %ld", map_share_piece_count(59221172UL));
		return 0;
	}
	/* the menus show a map without its hash, unless asked to tell versions
	apart (Halo PC's own maps by their names) */
	CASE("display-names")
	{
		CHECK(!strcmp(menu_name("the_bay_of_pigs_0c7e7075", FALSE), "The Bay Of Pigs"), "made %s",
			menu_name("the_bay_of_pigs_0c7e7075", FALSE));
		CHECK(!strcmp(menu_name("the_bay_of_pigs", FALSE), "The Bay Of Pigs"), "a name without a hash: %s",
			menu_name("the_bay_of_pigs", FALSE));
		CHECK(!strcmp(menu_name("icefields_0e2b3ee4", FALSE), "Ice Fields"), "Halo PC's map: %s",
			menu_name("icefields_0e2b3ee4", FALSE));
		CHECK(!strcmp(menu_name("hugeass_0e2b3ee4", TRUE), "Hugeass 0e2b3ee4"), "two versions: %s",
			menu_name("hugeass_0e2b3ee4", TRUE));
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
