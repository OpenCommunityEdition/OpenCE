/*
PELICAN_LIGHTS.C (test)

The real pelican_lights_tags_loaded, _tags_unloaded and _zbias
(pelican_lights.c): of the Pelican's two light shaders the tags give
lights the decal flag and lights flashing none, so its lights were tested
against the hull with no depth bias; and the decals' bias, 8, left both cut
through where the hull curves over them as a Pelican flies by. As a map's
tags load lights flashing is given the flag and both are noted, and a
model's decal drawn with either takes a bias of 32; every other shader
keeps the one given, and nothing is noted once the tags unload. On a10
nothing is flagged or noted.
test_pelican_lights.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#define SET_FLAG(f, b, v) ((v) ? ((f) |= (unsigned)FLAG(b)) : ((f) &= (unsigned)~FLAG(b)))
#define NUMBEROF(array) (sizeof(array) / sizeof(array[0]))
#define csstrcasecmp _stricmp

struct shader
{
	short type;
};

#include "config.inc"

/* the map's chicago shaders: lights (0), lights flashing (1), another (2) */
static struct pelican_lights_chicago test_shaders[3];
static boolean test_map_has_lights = TRUE;

static long tag_loaded(long group_tag, char const *name)
{
	(void)group_tag;
	if (!test_map_has_lights)
		return NONE;
	if (!strcmp(name, "vehicles\\pelican\\shaders\\lights"))
		return 0;
	if (!strcmp(name, "vehicles\\pelican\\shaders\\lights flashing"))
		return 1;
	return NONE;
}

static void *tag_get(long group_tag, long index)
{
	(void)group_tag;
	return &test_shaders[index];
}

#include "under_test.inc"

/* as the Xbox maps have them: lights a decal, lights flashing not */
static void shaders_reset(void)
{
	memset(test_shaders, 0, sizeof(test_shaders));
	test_shaders[0].flags = FLAG(1);
	test_map_has_lights = TRUE;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* lights flashing made a decal, once */
	CASE("flagged")
	{
		shaders_reset();
		CHECK(pelican_lights_tags_loaded("a30") == 1, "not one changed");
		CHECK(test_shaders[0].flags == FLAG(1) && test_shaders[1].flags == FLAG(1), "flags %x and %x",
			test_shaders[0].flags, test_shaders[1].flags);
		CHECK(!test_shaders[2].flags, "another shader changed");
		CHECK(pelican_lights_tags_loaded("a30") == 0, "changed again");
		return 0;
	}
	/* both lights biased 32, any other shader as given */
	CASE("biased")
	{
		shaders_reset();
		pelican_lights_tags_loaded("a30");
		CHECK(pelican_lights_zbias(&test_shaders[0].shader, 8) == 32, "lights %ld",
			pelican_lights_zbias(&test_shaders[0].shader, 8));
		CHECK(pelican_lights_zbias(&test_shaders[1].shader, 8) == 32, "lights flashing %ld",
			pelican_lights_zbias(&test_shaders[1].shader, 8));
		CHECK(pelican_lights_zbias(&test_shaders[2].shader, 8) == 8, "another %ld",
			pelican_lights_zbias(&test_shaders[2].shader, 8));
		CHECK(pelican_lights_zbias(NULL, 8) == 8, "no shader");
		return 0;
	}
	/* once the tags unload, or on a map without them: as given */
	CASE("other maps")
	{
		shaders_reset();
		pelican_lights_tags_loaded("a30");
		pelican_lights_tags_unloaded();
		CHECK(pelican_lights_zbias(&test_shaders[1].shader, 8) == 8, "kept after the tags unloaded");
		shaders_reset();
		test_map_has_lights = FALSE;
		CHECK(pelican_lights_tags_loaded("a30") == 0, "changed with no such shaders");
		CHECK(pelican_lights_zbias(&test_shaders[0].shader, 8) == 8, "biased with no such shaders");
		return 0;
	}

	/* a10: as the tags have them */
	CASE("first level")
	{
		shaders_reset();
		CHECK(pelican_lights_tags_loaded("a10") == 0, "changed on a10");
		CHECK(!test_shaders[1].flags, "lights flashing flagged on a10");
		CHECK(pelican_lights_zbias(&test_shaders[0].shader, 8) == 8, "lights biased on a10");
		CHECK(pelican_lights_zbias(&test_shaders[1].shader, 8) == 8, "lights flashing biased on a10");
		shaders_reset();
		CHECK(pelican_lights_tags_loaded("b30") == 1 && pelican_lights_zbias(&test_shaders[1].shader, 8) == 32,
			"b30 unchanged");
		return 0;
	}

	return 2;
}
