/*
CAMO_VIEW_BATCHES.C (test)

The real active_camouflage_next_part and active_camouflage_transparent_part
(source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c): a cloak's
parts are drawn together, and its transparent parts (the assault rifle's
display) after the last of them, once, in the view the cloak is drawn in.
The player's own legs (display.first_person_legs) are cloaked by the unit
that cloaks the first-person weapon and arms: their cloak ends in the world,
with their own transparent parts, and the weapon's display is drawn only
with the weapon, whatever comes between the legs and the weapon (the
weapon's muzzle flash as it fires). Another cloaked object's parts are drawn
together, with its display after them, as on the Xbox.

draw_sorted walks the groups as rasterizer_transparent_geometry_group_draw
does, in their order: a cloaked part ends its cloak unless the next group
goes on with it, and the end draws the transparent parts of the cloak.
test_camo_view_batches.py takes the structures, the enumerations and the
code from the sources (types.inc, under_test.inc).
*/

#include "harness.h"
#include <stddef.h>

typedef unsigned short word;
typedef struct { short x, y; } point2d;
typedef struct { real x, y; } real_point2d;
typedef struct { real i, j; } real_vector2d;
typedef struct { real red, green, blue; } real_rgb_color;
typedef struct { real alpha; real_rgb_color rgb; } real_argb_color;
typedef struct { real_vector3d n; real d; } real_plane3d;
#define TAG_STRING_LENGTH 31

#include "types.inc"
#include "under_test.inc"

enum
{
	PLAYER = 7,
	ENEMY = 9,
	MAXIMUM_GROUPS = 8,
	MAXIMUM_DRAWS = 16,
};

static struct shader model_shader, effect_shader, chicago_shader;

/* the group's flags: in the world, or with the first-person weapon */
enum { WORLD = 0, FIRST_PERSON = 1 };

static struct transparent_geometry_group cloaked(long source, int view)
{
	struct transparent_geometry_group group;

	memset(&group, 0, sizeof(group));
	group.geometry_flags = view == FIRST_PERSON ? FLAG(_rasterizer_geometry_first_person_bit) : 0;
	group.source_object_index = source;
	group.shader = &model_shader;
	group.effect_type = _render_model_effect_type_active_camouflage;
	return group;
}

static struct transparent_geometry_group other(int view)
{
	struct transparent_geometry_group group;

	memset(&group, 0, sizeof(group));
	group.geometry_flags = view == FIRST_PERSON ? FLAG(_rasterizer_geometry_first_person_bit) : 0;
	group.shader = &effect_shader;
	group.effect_type = _render_model_effect_type_none;
	return group;
}

/* a transparent part of a cloaked model, as rasterizer_xbox_models.c queues
it (rasterizer_transparent_geometry_new_group2) */
static struct transparent_geometry_group transparent_part(long source, int view)
{
	struct transparent_geometry_group group;

	memset(&group, 0, sizeof(group));
	group.geometry_flags = view == FIRST_PERSON ? FLAG(_rasterizer_geometry_first_person_bit) : 0;
	group.source_object_index = source;
	group.shader = &chicago_shader;
	group.effect_type = _render_model_effect_type_active_camouflage;
	group.active_camouflage_transparent_source_object_index = source;
	return group;
}

/* each transparent part drawn: the part, the group whose cloak ended, and
whether the first-person weapon's view had begun */
static struct { int part; int at; boolean first_person_view; } draws[MAXIMUM_DRAWS];
static int draw_count;

static void draw_sorted(struct transparent_geometry_group *sorted, int count,
	struct transparent_geometry_group *parts, int part_count)
{
	boolean first_person_view = FALSE;
	int index, part;

	draw_count = 0;
	for (index = 0; index < count; index++)
	{
		struct transparent_geometry_group *group = &sorted[index];

		/* (the first-person groups come last: rasterizer_transparent_geometry.c) */
		if (TEST_FLAG(group->geometry_flags, _rasterizer_geometry_first_person_bit))
			first_person_view = TRUE;
		if (group->effect_type != _render_model_effect_type_active_camouflage ||
			!group->shader || group->shader->base.type != _shader_type_model)
		{
			continue;
		}
		if (active_camouflage_next_part(group, index + 1 < count ? &sorted[index + 1] : NULL))
			continue;
		for (part = 0; part < part_count; part++)
		{
			if (active_camouflage_transparent_part(group, &parts[part]))
			{
				CHECK(draw_count < MAXIMUM_DRAWS, "too many draws");
				draws[draw_count].part = part;
				draws[draw_count].at = index;
				draws[draw_count].first_person_view = first_person_view;
				draw_count++;
			}
		}
	}
}

/* the part drawn once, at the end of the cloak of group at, in a view */
static void drawn_once(int part, int at, boolean first_person_view, const char *what)
{
	int index, found = 0;

	for (index = 0; index < draw_count; index++)
	{
		if (draws[index].part != part)
			continue;
		found++;
		CHECK(draws[index].at == at, "%s: drawn at the end of group %d's cloak, not %d's", what, draws[index].at, at);
		CHECK(draws[index].first_person_view == first_person_view, "%s: drawn %s", what,
			draws[index].first_person_view ? "with the first-person weapon" : "in the world");
	}
	CHECK(found == 1, "%s: drawn %d times", what, found);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	struct transparent_geometry_group sorted[MAXIMUM_GROUPS], parts[MAXIMUM_GROUPS];

	model_shader.base.type = _shader_type_model;
	effect_shader.base.type = _shader_type_effect;
	chicago_shader.base.type = _shader_type_transparent_chicago;

	/* the legs, the weapon's muzzle flash as it fires, then the cloaked
	weapon and arms: the display once, with the weapon, the legs' own part
	once, in the world */
	CASE("fired")
	{
		sorted[0] = other(WORLD);
		sorted[1] = cloaked(PLAYER, WORLD);
		sorted[2] = other(FIRST_PERSON);
		sorted[3] = cloaked(PLAYER, FIRST_PERSON);
		sorted[4] = cloaked(PLAYER, FIRST_PERSON);
		parts[0] = transparent_part(PLAYER, FIRST_PERSON);
		parts[1] = transparent_part(PLAYER, WORLD);
		draw_sorted(sorted, 5, parts, 2);
		drawn_once(0, 4, TRUE, "the weapon's display");
		drawn_once(1, 1, FALSE, "the legs' part");
		return 0;
	}
	/* nothing between the legs and the weapon */
	CASE("legs-then-weapon")
	{
		sorted[0] = cloaked(PLAYER, WORLD);
		sorted[1] = cloaked(PLAYER, WORLD);
		sorted[2] = cloaked(PLAYER, FIRST_PERSON);
		sorted[3] = cloaked(PLAYER, FIRST_PERSON);
		parts[0] = transparent_part(PLAYER, FIRST_PERSON);
		parts[1] = transparent_part(PLAYER, WORLD);
		draw_sorted(sorted, 4, parts, 2);
		drawn_once(0, 3, TRUE, "the weapon's display");
		drawn_once(1, 1, FALSE, "the legs' part");
		return 0;
	}
	/* no legs (display.first_person_legs off), firing */
	CASE("no-legs")
	{
		sorted[0] = other(WORLD);
		sorted[1] = other(FIRST_PERSON);
		sorted[2] = cloaked(PLAYER, FIRST_PERSON);
		sorted[3] = cloaked(PLAYER, FIRST_PERSON);
		parts[0] = transparent_part(PLAYER, FIRST_PERSON);
		draw_sorted(sorted, 4, parts, 1);
		drawn_once(0, 3, TRUE, "the weapon's display");
		return 0;
	}
	/* another cloaked player's weapon, then the legs and the weapon: each
	display with its own cloak, as on the Xbox */
	CASE("another-cloak")
	{
		sorted[0] = cloaked(ENEMY, WORLD);
		sorted[1] = cloaked(ENEMY, WORLD);
		sorted[2] = other(WORLD);
		sorted[3] = cloaked(PLAYER, WORLD);
		sorted[4] = cloaked(PLAYER, FIRST_PERSON);
		parts[0] = transparent_part(ENEMY, WORLD);
		parts[1] = transparent_part(PLAYER, FIRST_PERSON);
		parts[2] = transparent_part(PLAYER, WORLD);
		draw_sorted(sorted, 5, parts, 3);
		drawn_once(0, 1, FALSE, "the other player's display");
		drawn_once(1, 4, TRUE, "the weapon's display");
		drawn_once(2, 3, FALSE, "the legs' part");
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
