/*
LONGSWORD_EXHAUST.C (test)

The real rasterizer_lens_flare_face_light_direction (rasterizer_lights.c),
longsword_exhaust_tags_loaded, _tags_unloaded and _flare_direction
(longsword_exhaust.c), and the draw's own fade of a reflection by its
light's direction to the camera (rasterizer_lens_flares_draw): the
Longswords' exhaust flare, which its tag fades by no angle, was drawn across
the black ships as a pale smear wherever its occlusion test cleared the top
of the hull, from ahead or abreast. It is made to fade by its light's
direction to the camera, its direction the ship's way back: whole from
behind, none from ahead or abreast. A flare with a fade of its own, or a
reflection scaled by anything, is left as it is.
test_longsword_exhaust.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"
#include <math.h>

#include "config.inc"

static struct lens_flare_reflection test_reflections[2][2];
static struct lens_flare_definition test_definitions[2];
static long test_flare_index = NONE;

/* the objects: the ship (0) and an engine on it (1), whose parent it is */
struct object_datum
{
	struct { real_vector3d forward; } object;
};
static struct object_datum test_objects[2];

static long tag_loaded(long group_tag, char const *name)
{
	(void)group_tag;
	return !strcmp(name, "vehicles\\fighterbomber\\fighterbomber exhaust") ? test_flare_index : NONE;
}

static struct object_datum *object_try_and_get(long object_index)
{
	return object_index >= 0 && object_index < 2 ? &test_objects[object_index] : NULL;
}

static long object_get_ultimate_parent(long object_index)
{
	(void)object_index;
	return 0;
}

static void normalize3d(real_vector3d *vector)
{
	real length = (real)sqrt(vector->i * vector->i + vector->j * vector->j + vector->k * vector->k);

	if (length > 0.f)
	{
		vector->i /= length;
		vector->j /= length;
		vector->k /= length;
	}
}

#include "under_test.inc"

static boolean near(real a, real b)
{
	return fabs(a - b) < 1e-4f;
}

/* the Xbox map's: no fade, one reflection scaled by nothing */
static void definition_reset(short index, real falloff, real cutoff, short scale_function)
{
	memset(&test_definitions[index], 0, sizeof(test_definitions[index]));
	memset(test_reflections[index], 0, sizeof(test_reflections[index]));
	test_definitions[index].falloff_angle = falloff;
	test_definitions[index].cutoff_angle = cutoff;
	test_definitions[index].runtime_cosine_falloff_angle = (real)cos(falloff);
	test_definitions[index].runtime_cosine_cutoff_angle = (real)cos(cutoff);
	test_definitions[index].reflections.count = 2;
	test_definitions[index].reflections.address = test_reflections[index];
	test_reflections[index][0].brightness_scale_function = _lens_flare_reflection_scale_function_none;
	test_reflections[index][1].brightness_scale_function = scale_function;
}

/* a10's opening: the ship flies along -y, its marker points along -x */
static void ship_reset(void)
{
	test_objects[0].object.forward.i = 0.f;
	test_objects[0].object.forward.j = -1.f;
	test_objects[0].object.forward.k = 0.f;
}

/* the fade the draw gives the flare at corona, seen from camera */
static real fade_from(short index, real_vector3d direction, real cx, real cy, real cz)
{
	real_vector3d eye_to_corona = { -cx, -cy, -cz };

	return light_to_camera_fade(&test_definitions[index], direction, eye_to_corona);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a flare that fades by no angle: made to, every reflection by its
	light's direction to the camera */
	CASE("faced")
	{
		definition_reset(1, 0.f, 0.f, _lens_flare_reflection_scale_function_none);
		CHECK(rasterizer_lens_flare_face_light_direction(1, 0.5f, 1.0f), "not changed");
		CHECK(near(test_definitions[1].falloff_angle, 0.5f) && near(test_definitions[1].cutoff_angle, 1.0f),
			"angles %f %f", test_definitions[1].falloff_angle, test_definitions[1].cutoff_angle);
		CHECK(near(test_definitions[1].runtime_cosine_falloff_angle, (real)cos(0.5)) &&
			near(test_definitions[1].runtime_cosine_cutoff_angle, (real)cos(1.0)), "cosines %f %f",
			test_definitions[1].runtime_cosine_falloff_angle, test_definitions[1].runtime_cosine_cutoff_angle);
		CHECK(test_reflections[1][0].brightness_scale_function == _lens_flare_reflection_scale_function_light_to_camera &&
			test_reflections[1][1].brightness_scale_function == _lens_flare_reflection_scale_function_light_to_camera,
			"reflections scaled by %d and %d", test_reflections[1][0].brightness_scale_function,
			test_reflections[1][1].brightness_scale_function);
		return 0;
	}
	/* a flare with a fade of its own, or a reflection scaled by anything:
	as it is */
	CASE("as it is")
	{
		definition_reset(1, 0.3f, 0.6f, _lens_flare_reflection_scale_function_none);
		CHECK(!rasterizer_lens_flare_face_light_direction(1, 0.5f, 1.0f), "a fade of its own changed");
		CHECK(near(test_definitions[1].falloff_angle, 0.3f), "its falloff %f", test_definitions[1].falloff_angle);
		definition_reset(1, 0.f, 0.f, _lens_flare_reflection_scale_function_camera_direction);
		CHECK(!rasterizer_lens_flare_face_light_direction(1, 0.5f, 1.0f), "a scaled reflection changed");
		CHECK(test_reflections[1][0].brightness_scale_function == _lens_flare_reflection_scale_function_none,
			"the first reflection scaled by %d", test_reflections[1][0].brightness_scale_function);
		CHECK(near(test_definitions[1].cutoff_angle, 0.f), "its cutoff %f", test_definitions[1].cutoff_angle);
		definition_reset(1, 0.f, 0.f, _lens_flare_reflection_scale_function_none);
		CHECK(!rasterizer_lens_flare_face_light_direction(1, 1.0f, 0.5f), "a cutoff short of the falloff taken");
		CHECK(!rasterizer_lens_flare_face_light_direction(NONE, 0.5f, 1.0f), "no flare changed");
		return 0;
	}
	/* the map's exhaust flare: its direction the ship's way back, until
	the map's tags unload */
	CASE("the ship's way back")
	{
		real_vector3d direction = { 7.f, 7.f, 7.f };

		definition_reset(1, 0.f, 0.f, _lens_flare_reflection_scale_function_none);
		ship_reset();
		test_flare_index = 1;
		longsword_exhaust_tags_loaded();
		CHECK(test_reflections[1][0].brightness_scale_function == _lens_flare_reflection_scale_function_light_to_camera,
			"the flare not changed");
		CHECK(longsword_exhaust_flare_direction(1, 1, &direction), "not the exhaust's");
		CHECK(near(direction.i, 0.f) && near(direction.j, 1.f) && near(direction.k, 0.f), "direction %f %f %f",
			direction.i, direction.j, direction.k);
		direction.i = direction.j = direction.k = 7.f;
		CHECK(!longsword_exhaust_flare_direction(0, 1, &direction) && direction.i == 7.f, "another flare turned");
		longsword_exhaust_tags_unloaded();
		CHECK(!longsword_exhaust_flare_direction(1, 1, &direction) && direction.i == 7.f,
			"turned after the map's tags unloaded");
		return 0;
	}
	/* a map with no such flare, or one whose flare fades: none turned */
	CASE("other maps")
	{
		real_vector3d direction = { 7.f, 7.f, 7.f };

		ship_reset();
		test_flare_index = NONE;
		longsword_exhaust_tags_loaded();
		CHECK(!longsword_exhaust_flare_direction(1, 1, &direction), "turned with no such flare");
		definition_reset(1, 0.3f, 0.6f, _lens_flare_reflection_scale_function_none);
		test_flare_index = 1;
		longsword_exhaust_tags_loaded();
		CHECK(!longsword_exhaust_flare_direction(1, 1, &direction) && direction.i == 7.f,
			"a flare with a fade of its own turned");
		return 0;
	}
	/* the draw's fade of the changed flare, as a10's camera sees the ship:
	whole from behind, none from ahead or abreast */
	CASE("behind only")
	{
		real_vector3d direction;

		definition_reset(1, 0.f, 0.f, _lens_flare_reflection_scale_function_none);
		ship_reset();
		test_flare_index = 1;
		longsword_exhaust_tags_loaded();
		CHECK(longsword_exhaust_flare_direction(1, 1, &direction), "not the exhaust's");
		CHECK(near(fade_from(1, direction, 0.f, 50.f, 5.f), 1.f), "from behind %f", fade_from(1, direction, 0.f, 50.f, 5.f));
		CHECK(near(fade_from(1, direction, 0.f, -50.f, 5.f), 0.f), "from ahead %f", fade_from(1, direction, 0.f, -50.f, 5.f));
		/* a10's opening, where it smeared: the camera ahead and to the side */
		CHECK(near(fade_from(1, direction, 46.f, -86.f, 21.f), 0.f), "from ahead and abreast %f",
			fade_from(1, direction, 46.f, -86.f, 21.f));
		CHECK(near(fade_from(1, direction, 50.f, 0.f, 0.f), 0.f), "abreast %f", fade_from(1, direction, 50.f, 0.f, 0.f));
		{
			real between = fade_from(1, direction, (real)sin(0.7854), (real)cos(0.7854), 0.f);

			CHECK(between > 0.05f && between < 0.95f, "45 degrees from behind %f", between);
		}
		return 0;
	}

	return 2;
}
