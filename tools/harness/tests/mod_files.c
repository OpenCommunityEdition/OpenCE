/*
MOD_FILES.C (test)

The real checks that stand between a mod's files and the game: the keys a
map's tag names are looked up by in the mods' files
(port/linux/src/port_mods.c), mod.toml's channel_order (port_mods.c), the
bounds of a tag file's counts and offsets
(port/linux/game/mods.c), and the comparison that keeps a mod's string from
being printed as a format that reads arguments the game does not pass
(port/linux/game/mod_strings.c): test_mod_files.py takes the code
(under_test.inc).
*/

#include "harness.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* (mod_strings.c's struct conversion and MAXIMUM_CONVERSIONS) */
#include "config.inc"

#include "under_test.inc"

/* a string's bytes, as a tag file has them: UTF-16 little-endian or not */
static unsigned long string_bytes(char const *text, boolean unicode, byte *bytes)
{
	unsigned long length = (unsigned long)strlen(text) + 1;
	unsigned long index;

	for (index = 0; index < length; index++)
	{
		if (unicode)
		{
			bytes[2 * index] = (byte)text[index];
			bytes[2 * index + 1] = 0;
		}
		else
		{
			bytes[index] = (byte)text[index];
		}
	}
	return unicode ? 2 * length : length;
}

static boolean same_formats(char const *first, char const *second, boolean unicode)
{
	byte first_bytes[512];
	byte second_bytes[512];
	unsigned long first_size = string_bytes(first, unicode, first_bytes);
	unsigned long second_size = string_bytes(second, unicode, second_bytes);

	return formats_match(first_bytes, first_size, second_bytes, second_size, unicode);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a map's tag name and a file's path make the same key, whatever their
	case and slashes; one that does not fit is none */
	CASE("keys")
	{
		char key[64];
		char small[8];

		CHECK(key_make("Sound/SFX\\Weapons/fire.Sound", key, sizeof(key)), "a name was refused");
		CHECK(!strcmp(key, "sound\\sfx\\weapons\\fire.sound"), "made %s", key);
		CHECK(key_hash("a\\b.sound") == key_hash(key_make("A/B.SOUND", key, sizeof(key)) ? key : ""),
			"the same key hashed otherwise");
		CHECK(!key_make("12345678", small, sizeof(small)), "a key longer than its room was taken");
		CHECK(key_make("1234567", small, sizeof(small)), "the longest key that fits was refused");
		CHECK(names_equal("HD_Audio", "hd_audio"), "a folder named in another case was not found");
		CHECK(!names_equal("hd_audio", "hd_audio2"), "a folder was found by its start");
		return 0;
	}
	/* a count and offset from a file lie within it, however large */
	CASE("bounds")
	{
		CHECK(mods_in_file(10, 5, 15), "the last bytes of the file were refused");
		CHECK(mods_in_file(15, 0, 15), "nothing at the end was refused");
		CHECK(!mods_in_file(10, 6, 15), "a byte past the end was taken");
		CHECK(!mods_in_file(16, 0, 15), "an offset past the end was taken");
		CHECK(!mods_in_file(0xFFFFFFF0UL, 0x20, 0x100), "an offset that wraps was taken");
		CHECK(!mods_in_file(0x10, 0xFFFFFFF8UL, 0x100), "a size that wraps was taken");
		return 0;
	}
	/* a string replaces the map's only if it reads the same arguments */
	CASE("formats")
	{
		boolean unicode;

		for (unicode = FALSE; unicode <= TRUE; unicode++)
		{
			CHECK(same_formats("%s was killed by %s", "%s a tue %s", unicode), "a translation was refused");
			CHECK(same_formats("Score", "Punkte", unicode), "a string with no conversions was refused");
			CHECK(same_formats("%2d kills", "%d Abschuesse", unicode), "a width was taken for an argument");
			CHECK(!same_formats("%s died", "%s died %s", unicode), "a string reading another argument was taken");
			CHECK(!same_formats("%d lives", "%s lives", unicode), "a string reading a number as a string was taken");
			CHECK(!same_formats("%d", "%ld", unicode), "a string reading a longer number was taken");
			CHECK(!same_formats("Score", "%n", unicode), "a string writing through an argument was taken");
			CHECK(!same_formats("%d", "%*d", unicode), "a string reading a width argument was taken");
			CHECK(same_formats("100%%", "100 %%", unicode), "a percent sign was taken for a conversion");
			CHECK(same_formats("Score: 100%%", "Punkte: 100", unicode), "a percent sign was taken for a conversion");
			CHECK(!same_formats("100%%", "100%", unicode), "a lone percent sign at the end was missed");
		}
		return 0;
	}
	/* however many conversions or characters, the comparison ends */
	CASE("long-formats")
	{
		char many[200];
		byte bytes[64];
		long index;

		/* (one more conversion than are compared) */
		for (index = 0; index < 2 * (MAXIMUM_CONVERSIONS + 1); index++)
			many[index] = index % 2 ? 'd' : '%';
		many[index] = 0;
		CHECK(!same_formats(many, many, FALSE), "more conversions than are compared were taken");
		/* (a string that is not terminated ends at its size: "%%%%" then a
		'%' with nothing after it) */
		memset(bytes, '%', sizeof(bytes));
		CHECK(format_conversions(bytes, 5, FALSE, (struct conversion[MAXIMUM_CONVERSIONS]){ { 0 } }) == 1,
			"a string was read past its size");
		/* (a character past ASCII is no conversion's: U+016C after '%' is not
		the 'l' of its low byte) */
		{
			byte wide[8] = { '%', 0, 0x6C, 0x01, 'd', 0, 0, 0 };
			byte modifier[8] = { '%', 0, 'l', 0, 'd', 0, 0, 0 };

			CHECK(!formats_match(wide, sizeof(wide), modifier, sizeof(modifier), TRUE),
				"a wide character was read as its low byte");
		}
		return 0;
	}
	/* mod.toml's channel_order: "xbox" or "pc", whatever their case; else
	none (the mod's bitmaps taken as Halo PC's) */
	CASE("channel-order")
	{
		CHECK(channel_order_xbox("xbox") == 1, "\"xbox\" was not the Xbox's order");
		CHECK(channel_order_xbox("XBOX") == 1, "\"XBOX\" was not the Xbox's order");
		CHECK(channel_order_xbox("pc") == 0, "\"pc\" was not Halo PC's order");
		CHECK(channel_order_xbox("Pc") == 0, "\"Pc\" was not Halo PC's order");
		CHECK(channel_order_xbox("gearbox") == -1, "an unknown value was taken");
		CHECK(channel_order_xbox("") == -1, "an empty value was taken");
		CHECK(channel_order_xbox("xbox1") == -1, "a longer value was taken");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
