/*
UPDATE_NAMES.C (test)

The names the self-updater takes from a release's zip
(port/linux/src/updater.c): the files of the release's tree (bin/opence,
lib/opence/...) and of the flat release, but none that would leave the
game's folder. test_update_names.py takes the code (under_test.inc).
*/

#include "harness.h"

#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* the release's files, in the tree and flat */
	CASE("release-names")
	{
		CHECK(updater_name_inside("bin/opence"), "bin/opence was refused");
		CHECK(updater_name_inside("lib/opence/libSDL3.so.0"), "lib/opence/libSDL3.so.0 was refused");
		CHECK(updater_name_inside("share/icons/hicolor/48x48/apps/opence.png"), "an icon was refused");
		CHECK(updater_name_inside("halo"), "the flat release's halo was refused");
		CHECK(updater_name_inside("share/opence/brokers.txt.old"), "an old file's name was refused");
		CHECK(updater_name_inside(".hidden"), "a name that starts with a dot was refused");
		return 0;
	}
	/* nothing outside the game's folder, nor a folder */
	CASE("leaving-names")
	{
		static char const *const leaving[] =
		{
			"", "/etc/passwd", "../halo", "bin/../../halo", "bin/..", "..", ".", "./bin/opence", "bin//opence",
			"bin/", "bin\\opence", "..\\halo", "c:/halo", "C:halo", "share/./opence",
		};
		int index;

		for (index = 0; index < (int)(sizeof(leaving) / sizeof(*leaving)); index++)
			CHECK(!updater_name_inside(leaving[index]), "a name that leaves the game's folder was taken");
		return 0;
	}
	return 2;
}
