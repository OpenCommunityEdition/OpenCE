/*
MAP_SHARE_NAMES.C (test)

The real names of map sharing (port/linux/game/map_share_files.c): the file
name of a second version of a map ends in _ and the first digits of its
hash, which the menus leave out, numbering the versions instead
(custom_edition_maps.c), and a map's version is those digits' number.
test_map_share_names.py takes the constants (config.inc) and the code
(under_test.inc).
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
#define csprintf sprintf

/* a map listed (custom_edition_maps.c's, its names alone) */
struct custom_edition_map
{
	char name[64];
	wchar_t display_name[64 + 8];
};

#include "under_test.inc"

/* the menus' name of a map file's name (custom_edition_maps.c) */
static char const *wide_text(wchar_t const *display_name)
{
	static char text[80];
	int index;

	for (index = 0; display_name[index] && index < 79; index++)
		text[index] = (char)display_name[index];
	text[index] = 0;
	return text;
}

static char const *menu_name(char const *name)
{
	static wchar_t display_name[64];

	display_name_make(name, display_name);
	return wide_text(display_name);
}

/* the maps listed, sorted by name as the map lists sort them, their menu
names made and the versions told apart */
static struct custom_edition_map fake_maps[8];

static short fake_list(char const *const *names, short count)
{
	short index;

	for (index = 0; index < count; index++)
	{
		strcpy(fake_maps[index].name, names[index]);
		display_name_make(fake_maps[index].name, fake_maps[index].display_name);
	}
	custom_edition_maps_tell_versions_apart(fake_maps, count);
	return count;
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
			/* Windows' devices, before the first dot, in any case */
			"CON.x_0e2b3ee4", "con.x_0e2b3ee4", "Nul.map", "PRN", "aux .x_0e2b3ee4", "COM1.x_0e2b3ee4", "lpt9.x", "com0.x",
			"CONIN$.x", "conout$",
		};
		static char const *const safe[] =
		{
			"con_0e2b3ee4", "console.x_0e2b3ee4", "com10.x", "com.x", "lptx.x", "a.con", "nul_0e2b3ee4", "auxiliary",
		};
		int index;

		CHECK(map_share_name_safe("hugeass_0e2b3ee4"), "a map's name was refused");
		CHECK(map_share_name_safe("Elite_Alpha_Siege (v2).final"), "a name with spaces and dots inside was refused");
		for (index = 0; index < (int)(sizeof(unsafe) / sizeof(unsafe[0])); index++)
			CHECK(!map_share_name_safe(unsafe[index]), "the name \"%s\" was taken", unsafe[index]);
		for (index = 0; index < (int)(sizeof(safe) / sizeof(safe[0])); index++)
			CHECK(map_share_name_safe(safe[index]), "the name \"%s\" was refused", safe[index]);
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
	/* the menus show a map without its hash (Halo PC's own maps by their
	names) */
	CASE("display-names")
	{
		CHECK(!strcmp(menu_name("the_bay_of_pigs_0c7e7075"), "The Bay Of Pigs"), "made %s",
			menu_name("the_bay_of_pigs_0c7e7075"));
		CHECK(!strcmp(menu_name("the_bay_of_pigs"), "The Bay Of Pigs"), "a name without a hash: %s",
			menu_name("the_bay_of_pigs"));
		CHECK(!strcmp(menu_name("icefields_0e2b3ee4"), "Ice Fields"), "Halo PC's map: %s",
			menu_name("icefields_0e2b3ee4"));
		return 0;
	}
	/* versions of a map: the one of the name alone first, the others (2),
	(3) after it; a map of a name of its own, or a longer name, is not one */
	CASE("versions-numbered")
	{
		static char const *const names[] =
		{
			"hugeass", "hugeass2", "hugeass_0e2b3ee4", "hugeass_ffff0000", "ratrace_12345678", "wizard",
		};
		short count = fake_list(names, NUMBEROF(names));

		CHECK(!strcmp(wide_text(fake_maps[0].display_name), "Hugeass"), "the first version: %s",
			wide_text(fake_maps[0].display_name));
		CHECK(!strcmp(wide_text(fake_maps[1].display_name), "Hugeass2"), "another map: %s",
			wide_text(fake_maps[1].display_name));
		CHECK(!strcmp(wide_text(fake_maps[2].display_name), "Hugeass (2)"), "the second version: %s",
			wide_text(fake_maps[2].display_name));
		CHECK(!strcmp(wide_text(fake_maps[3].display_name), "Hugeass (3)"), "the third version: %s",
			wide_text(fake_maps[3].display_name));
		CHECK(!strcmp(wide_text(fake_maps[4].display_name), "Ratrace"), "a map's one version: %s",
			wide_text(fake_maps[4].display_name));
		CHECK(!strcmp(wide_text(fake_maps[5].display_name), "Wizard"), "a map of its own: %s",
			wide_text(fake_maps[5].display_name));
		CHECK(count == 6, "listed %d", count);
		return 0;
	}
	/* versions without the one of the name alone: the first of them by its
	name, the next (2) */
	CASE("versions-without-the-first")
	{
		static char const *const names[] = { "hugeass_0e2b3ee4", "hugeass_ffff0000" };

		fake_list(names, NUMBEROF(names));
		CHECK(!strcmp(wide_text(fake_maps[0].display_name), "Hugeass"), "the first: %s",
			wide_text(fake_maps[0].display_name));
		CHECK(!strcmp(wide_text(fake_maps[1].display_name), "Hugeass (2)"), "the second: %s",
			wide_text(fake_maps[1].display_name));
		return 0;
	}
	/* a map's version, a game record's, is the number its digits are (the
	name of a second version ends in them), never 0 */
	CASE("versions")
	{
		byte hash[MAP_SHARE_HASH_BYTES] = { 0x0e, 0x2b, 0x3e, 0xe4, 0x55 };
		byte zero[MAP_SHARE_HASH_BYTES] = { 0 };
		char digits[MAP_SHARE_NAME_DIGITS + 1];

		map_share_hash_digits(hash, digits);
		CHECK(map_share_hash_version(hash) == strtoul(digits, NULL, 16), "version %08lx for the digits %s",
			map_share_hash_version(hash), digits);
		CHECK(map_share_hash_version(zero) != 0, "a version of 0, which a client checks nothing for");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
