/*
FIRST_PERSON_PARTICLES.C (test)

The real render_particles (render_particles.c), with its
compare_rendered_particles, over a view and a sprite builder that keep which
particles are drawn with the first-person weapon: its pass draws them away
from the weapon without the depth test (rasterizer_xbox_transparent_geometry.c).
A particle stuck to the first-person weapon (a charge) or shown only in first
person is drawn with it; a particle the first-person weapon threw off, free
in the world (a pistol's, an assault rifle's or a sniper rifle's casing,
shown in both views), is drawn in the world, as on the Xbox, though it keeps
the flag of the weapon it came from: drawn with the weapon, a casing on the
ground showed through floors and walls. test_first_person_particles.py takes
the flags, the structures and the code from the sources (config.inc,
under_test.inc).
*/

#include <stdlib.h>
#include "harness.h"

typedef unsigned short word;
typedef struct { real alpha, red, green, blue; } real_argb_color;
typedef struct { real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
struct location { long cluster_reference; short leaf_index; short cluster_index; };
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))
#define __cdecl
#define MAXIMUM_LOCAL_PLAYERS 4

#include "config.inc"

/* the particles: one a slot, each drawn where its x is its slot */
enum { PARTICLE_SLOTS = 8 };
static struct data_array *particle_data;
#define particle_get(index) ((struct particle_datum *)datum_get(particle_data, (index)))

static long data_next_index(struct data_array *array, long previous)
{
	long index;

	for (index = previous == NONE ? 0 : DATUM_INDEX_TO_ABSOLUTE_INDEX(previous) + 1;
		index < array->maximum_count;
		index++)
	{
		struct datum_header *header = (struct datum_header *)(array->data + index * array->size);

		if (header->identifier)
			return ((long)(unsigned short)header->identifier << 16) | index;
	}
	return NONE;
}

static boolean particle_drawn(long particle_index) { return TRUE; }
static boolean render_location_visible(struct location const *location) { return TRUE; }
static boolean viewmodel_draws_geometry(boolean first_person_weapon) { return TRUE; }
static real particle_get_radius(long particle_index) { return 0.05f; }
static void particle_delete(long particle_index) { datum_delete(particle_data, particle_index); }

/* the particles' definition (a casing's) */
struct shader_effect_definition { real secondary_map_radius; };
struct particle_definition
{
	struct { long index; } bitmap;
	struct shader_effect_definition shader;
	real lod_cutoff, runtime_oo_width, minimum_pixels, fade_in_time, fade_out_time;
	short sprite_orientation;
};
static struct particle_definition casing = { { 0 }, { 0.0f }, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0 };
#define particle_definition_get(index) (&casing)

/* the view: local player 0's, in first person or not */
struct render_frustum { long unused; };
static struct { short local_player_index; long frame_index; struct render_frustum frustum; } render;
static boolean first_person;
static boolean local_player_is_first_person(short local_player_index) { return first_person; }
static boolean render_particles_enabled = TRUE;
#define profile_enter(section) ((void)0)
#define profile_exit(section) ((void)0)
static real render_frustum_sphere_diameter_in_pixels(struct render_frustum const *frustum,
	real_point3d const *position, real radius)
{
	return 10.0f;
}

/* the nodes, where the particles' own positions are */
static real_matrix4x3 identity = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 0, 0, 0 } };
static real_matrix4x3 const *first_person_weapon_get_node_matrix(short local_player_index, short node_index)
{
	return &identity;
}
static void *object_try_and_get(long object_index) { return &identity; }
static real_matrix4x3 const *object_get_node_matrix(long object_index, short node_index) { return &identity; }
static void matrix4x3_transform_point(real_matrix4x3 const *matrix, real_point3d const *in, real_point3d *out)
{
	*out = *in;
}
static void matrix4x3_transform_normal(real_matrix4x3 const *matrix, real_vector3d const *in, real_vector3d *out)
{
	*out = *in;
}

/* the sprites: each particle drawn, and whether with the first-person weapon */
struct build_sprite_data { unsigned long flags; struct shader_effect_definition const *shader; };
static boolean drawn[PARTICLE_SLOTS];
static boolean drawn_with_weapon[PARTICLE_SLOTS];

static void build_sprites_begin(struct build_sprite_data *data, short maximum_sprite_count, long bitmap_group_index,
	struct shader_effect_definition const *shader, unsigned long flags)
{
	data->flags = flags;
	data->shader = shader;
}

static void build_sprite(struct build_sprite_data *data, short mode, short sequence_index, short sprite_index,
	real_point3d const *position, real_vector3d const *direction, real rotation, real scale,
	real_argb_color const *color, real fade, unsigned long flags)
{
	long slot = (long)position->x;

	CHECK(slot >= 0 && slot < PARTICLE_SLOTS, "a sprite at %g", position->x);
	CHECK(!drawn[slot], "particle %ld drawn twice", slot);
	drawn[slot] = TRUE;
	drawn_with_weapon[slot] = TEST_FLAG(data->flags, _build_sprites_first_person_bit);
}

static void build_sprites_end(struct build_sprite_data *data) { }

#include "under_test.inc"

/* the particles, one of each */
enum
{
	_thrown_casing,			/* thrown off local player 0's first-person weapon, free */
	_stuck_charge,			/* stuck to it */
	_first_person_only,		/* shown only in first person, free */
	_another_players,		/* stuck to local player 1's */
	_nobodys,				/* the world's */
	NUMBER_OF_TEST_PARTICLES
};

static void add_particle(short slot, unsigned short flags, long object_index, byte local_player_index)
{
	long particle_index = datum_new(particle_data);
	struct particle_datum *particle = particle_get(particle_index);

	CHECK(DATUM_INDEX_TO_ABSOLUTE_INDEX(particle_index) == slot, "particle %d in slot %ld",
		slot, DATUM_INDEX_TO_ABSOLUTE_INDEX(particle_index));
	particle->flags = flags;
	particle->object_index = object_index;
	particle->local_player_index = local_player_index;
	particle->definition_index = 0;
	particle->lifespan = 2.0f;
	particle->position.x = (real)slot;
}

static void draw(boolean in_first_person)
{
	particle_data = game_state_data_new("particle", PARTICLE_SLOTS, sizeof(struct particle_datum));
	add_particle(_thrown_casing, FLAG(_particle_datum_attached_to_local_player_bit), NONE, 0);
	add_particle(_stuck_charge, FLAG(_particle_datum_attached_to_local_player_bit), 7, 0);
	add_particle(_first_person_only, FLAG(_particle_datum_dont_draw_third_person_bit), NONE, 0);
	add_particle(_another_players, FLAG(_particle_datum_attached_to_local_player_bit), 9, 1);
	add_particle(_nobodys, 0, NONE, (byte)NONE);
	render.local_player_index = 0;
	first_person = in_first_person;
	render_particles();
}

static void in_the_world(short slot, const char *what)
{
	CHECK(drawn[slot], "%s is not drawn", what);
	CHECK(!drawn_with_weapon[slot], "%s is drawn with the first-person weapon, not in the world", what);
}

static void with_the_weapon(short slot, const char *what)
{
	CHECK(drawn[slot], "%s is not drawn", what);
	CHECK(drawn_with_weapon[slot], "%s is drawn in the world, not with the first-person weapon", what);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* a casing the first-person weapon threw off, free in the world */
	CASE("thrown-casing")
	{
		draw(TRUE);
		in_the_world(_thrown_casing, "a casing thrown off the first-person weapon");
		return 0;
	}
	/* a charge stuck to the first-person weapon */
	CASE("stuck-charge")
	{
		draw(TRUE);
		with_the_weapon(_stuck_charge, "a charge stuck to the first-person weapon");
		return 0;
	}
	/* a particle shown only in first person */
	CASE("first-person-only")
	{
		draw(TRUE);
		with_the_weapon(_first_person_only, "a particle shown only in first person");
		return 0;
	}
	/* in third person, none of the player's own is the first-person
	weapon's, and one shown only in first person is not drawn */
	CASE("third-person")
	{
		draw(FALSE);
		in_the_world(_thrown_casing, "in third person, a thrown casing");
		in_the_world(_stuck_charge, "in third person, a stuck charge");
		CHECK(!drawn[_first_person_only], "in third person, a particle shown only in first person is drawn");
		return 0;
	}
	/* another player's and the world's are in the world */
	CASE("another-player")
	{
		draw(TRUE);
		in_the_world(_another_players, "another player's charge");
		in_the_world(_nobodys, "the world's particle");
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
