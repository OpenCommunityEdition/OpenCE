/*
NEEDLER_RELOAD.C (test)

The real needler_reload_rounds_loaded (first_person_weapons.c): a needler's
reload draws its needles growing toward those it will hold when the reload
ends, as weapon_magazine_finish_reload loads them (weapons.c): those loaded
(none, for a magazine that wastes them) and as many of the rest as a reload
loads, up to a full magazine. It drew them growing toward the rest alone, so
that with fewer than 20 left the needles sank during the reload (from 15
loaded with 3 left, to 3) and popped up to 18 as it ended.
test_needler_reload.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* (what needler_reload_rounds_loaded reads of a magazine's definition) */
struct weapon_magazine_definition
{
	unsigned long flags;
	short rounds_loaded_maximum;
	short rounds_reloaded;
};

#include "under_test.inc"

/* the needles a reload ends with: the needler's magazine of 20, reloading 20 */
static short reloaded(short rounds_loaded, short rounds_total, short rounds_reloaded, unsigned long flags)
{
	struct weapon_magazine magazine;
	struct weapon_magazine_definition definition;

	memset(&magazine, 0, sizeof(magazine));
	magazine.rounds_loaded = rounds_loaded;
	magazine.rounds_total = rounds_total;
	definition.flags = flags;
	definition.rounds_loaded_maximum = 20;
	definition.rounds_reloaded = rounds_reloaded;
	return needler_reload_rounds_loaded(&magazine, &definition);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* 20 or more left: a full magazine */
	CASE("plenty left")
	{
		CHECK(reloaded(5, 100, 20, 0) == 20, "5 loaded, 100 left: %d", reloaded(5, 100, 20, 0));
		CHECK(reloaded(0, 20, 20, 0) == 20, "none loaded, 20 left: %d", reloaded(0, 20, 20, 0));
		return 0;
	}
	/* fewer left than the magazine has room for: those loaded and the rest */
	CASE("few left")
	{
		CHECK(reloaded(15, 3, 20, 0) == 18, "15 loaded, 3 left: %d", reloaded(15, 3, 20, 0));
		CHECK(reloaded(0, 7, 20, 0) == 7, "none loaded, 7 left: %d", reloaded(0, 7, 20, 0));
		CHECK(reloaded(19, 1, 20, 0) == 20, "19 loaded, 1 left: %d", reloaded(19, 1, 20, 0));
		return 0;
	}
	/* a magazine that wastes its rounds (its flag 0), and one that loads
	fewer a reload: as weapon_magazine_finish_reload loads them */
	CASE("magazine rules")
	{
		CHECK(reloaded(5, 3, 20, 1) == 3, "wasting, 5 loaded, 3 left: %d", reloaded(5, 3, 20, 1));
		CHECK(reloaded(10, 50, 1, 0) == 11, "1 a reload, 10 loaded: %d", reloaded(10, 50, 1, 0));
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
