/*
LOOSE_SOUND_PATHS.C (test)

The real checks of port/linux/game/loose_sounds.c that stand between a map's
sound tag names (a Custom Edition map's are anyone's) and the files opened,
and between a tag file's counts and offsets and the memory read:
test_loose_sound_paths.py takes the folder and extension (config.inc) and the
code (under_test.inc).
*/

#include "harness.h"
#include "config.inc"

#include "under_test.inc"

static char const *path_of(char const *name)
{
	static char path[256];

	return tag_file_path(name, path, sizeof(path)) ? path : NULL;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a tag's name is a file of the tags folder */
	CASE("accepts-tag-names")
	{
		char const *path = path_of("sound\\sfx\\weapons\\assault rifle\\fire");

		CHECK(path != NULL, "a stock sound's name was refused");
		CHECK(!strcmp(path, TAG_FILE_FOLDER "sound\\sfx\\weapons\\assault rifle\\fire" TAG_FILE_EXTENSION),
			"made %s", path);
		CHECK(path_of("sound\\dialog\\a..b") != NULL, "two dots inside a name were refused");
		CHECK(path_of("sound/sfx/x") != NULL, "a name with forward slashes was refused");
		return 0;
	}
	/* no name reaches outside it */
	CASE("refuses-escapes")
	{
		static char const *const names[] =
		{
			"", "..\\x", "../x", "sound\\..\\..\\config", "sound/../../x", "..",
			"c:\\x", "d:x", "\\x", "/x", "sound\\\\x", "sound//x", "sound\\", "sound/",
		};
		int index;

		for (index = 0; index < (int)(sizeof(names) / sizeof(names[0])); index++)
		{
			CHECK(path_of(names[index]) == NULL, "the name \"%s\" was taken", names[index]);
		}
		CHECK(!tag_file_path(NULL, (char[16]){ 0 }, 16), "no name was taken");
		return 0;
	}
	/* nor past the path's room */
	CASE("refuses-long-names")
	{
		char name[300];

		memset(name, 'a', sizeof(name) - 1);
		name[sizeof(name) - 1] = 0;
		CHECK(path_of(name) == NULL, "a name longer than the path was taken");
		name[256 - sizeof(TAG_FILE_FOLDER TAG_FILE_EXTENSION)] = 0;
		CHECK(path_of(name) != NULL, "the longest name that fits was refused");
		return 0;
	}
	/* a count and offset from a file lie within it, however large */
	CASE("bounds")
	{
		CHECK(in_file(10, 5, 15), "the last bytes of the file were refused");
		CHECK(in_file(15, 0, 15), "nothing at the end was refused");
		CHECK(!in_file(10, 6, 15), "a byte past the end was taken");
		CHECK(!in_file(16, 0, 15), "an offset past the end was taken");
		CHECK(!in_file(0xFFFFFFF0UL, 0x20, 0x100), "an offset that wraps was taken");
		CHECK(!in_file(0x10, 0xFFFFFFF8UL, 0x100), "a size that wraps was taken");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
