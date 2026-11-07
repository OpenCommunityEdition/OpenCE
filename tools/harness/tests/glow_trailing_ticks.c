/*
GLOW_TRAILING_TICKS.C (test)

The real glow_update and the trailing particles' updates (glow.c), for a glow
on one marker that makes trailing particles: they are made and aged by the
ticks that have passed since the last frame, so a glow that makes 15 a second
makes one every other tick, a frame that runs no tick (the native builds draw
several a tick) makes and ages none, and a particle lives its lifetime; they
were made and aged by the game's whole time, so that every frame made them up
to the limit and aged each past its lifetime by the next.
test_glow_trailing_ticks.py takes the enums, structures and code from the
sources (config.inc, under_test.inc).
*/

#include "harness.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PIN(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))
#define match_vassert(file, line, condition, ...) ((void)(condition))

typedef struct { real alpha, red, green, blue; } real_argb_color;
typedef struct { real scale; real_vector3d forward, left, up; real_point3d position; } real_matrix4x3;
struct object_marker { real_matrix4x3 matrix; };
struct tag_reference { long group_tag; char *name; long name_length; long index; };

#include "config.inc"

/* a glow on one marker (the trailing particles' path; a glow on several also
moves its own particles along them) */
static struct glow_definition glow_tag;
#define glow_definition_get(index) (&glow_tag)
static struct object_marker marker = { { 1.f, { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }, { 3.f, 4.f, 5.f } } };

static short object_get_marker_by_name(long object_index, char const *name, struct object_marker *markers, short count)
{
	markers[0] = marker;
	return 1;
}
static boolean object_get_function_value(long object_index, short index, real *value) { return FALSE; }
static void vector_from_points3d(real_point3d const *a, real_point3d const *b, real_vector3d *v) { }
static void normalize3d(real_vector3d *v) { }
static real distance3d(real_point3d const *a, real_point3d const *b) { return 0.f; }

struct glow_datum;
struct glow_particle;
static void glow_particles_initialize(struct glow_datum *glow) { }
static void glow_normal_particle_update_position(long object_index, struct glow_datum *glow,
	struct glow_particle *particle, real elapsed_time, real rotation_per_unit_t) { }
static void glow_normal_particle_update_color(long object_index, struct glow_datum *glow, struct glow_particle *particle) { }
static void glow_normal_particle_update_size(struct glow_particle *particle) { }

/* the game's time, five minutes in, and the ticks the last frame ran */
static long game_time = 9000;
static short elapsed;
static long game_time_get(void) { return game_time; }
static short game_time_get_elapsed(void) { return elapsed; }

static struct { real time_delta_since_tick_sec; } render;

/* the particles, as many as fit (the map's limit) */
#define PARTICLE_LIMIT 64
static struct { struct data_array *glow_particle_data; } glow_globals;

static struct glow_particle *glow_trailing_particle_new(struct glow_datum *glow)
{
	long index = datum_new(glow_globals.glow_particle_data);
	struct glow_particle *particle;

	if (index == NONE)
		return NULL;
	particle = datum_get(glow_globals.glow_particle_data, index);
	particle->index = index;
	particle->position = glow->markers[0].matrix.position;
	particle->lifetime = (short)(long)(glow_tag.trailing_particle_lifetime * 30.0f);
	SET_FLAG(particle->flags, _glow_particle_trailing_bit, TRUE);
	return particle;
}

#include "under_test.inc"

static struct glow_datum glow;

/* a frame that ran some ticks (none: a frame drawn between ticks) */
static void frame(short ticks)
{
	game_time += ticks;
	elapsed = ticks;
	render.time_delta_since_tick_sec = ticks / 30.0f;
	glow_update(&glow, 0);
}

static short alive(void)
{
	return (short)glow_globals.glow_particle_data->count;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short i;

	glow_globals.glow_particle_data = game_state_data_new("glow particles", PARTICLE_LIMIT, sizeof(struct glow_particle));
	glow_tag.trailing_particle_generation_frequency = 15.f; /* (one every other tick) */
	glow_tag.trailing_particle_lifetime = 1.f;              /* (30 ticks) */

	/* a second of ticks makes a second's particles, one at a time */
	CASE("generation")
	{
		for (i = 0; i < 30; i++)
		{
			short before = alive();

			frame(1);
			CHECK(alive() <= before + 1, "a tick made %d particles", alive() - before);
		}
		CHECK(alive() >= 13 && alive() <= 15, "a second at 15 a second made %d particles", alive());
		return 0;
	}
	/* frames between ticks make and age nothing */
	CASE("between")
	{
		short ticks;

		for (i = 0; i < 3; i++)
			frame(1);
		CHECK(alive() == 1, "three ticks made %d particles, not one", alive());
		ticks = glow.head_particle->ticks_in_existence;
		for (i = 0; i < 10; i++)
			frame(0);
		CHECK(alive() == 1, "frames between ticks left %d particles, not one", alive());
		CHECK(glow.head_particle->ticks_in_existence == ticks, "frames between ticks aged a particle %d ticks",
			glow.head_particle->ticks_in_existence - ticks);
		return 0;
	}
	/* a particle lives its lifetime */
	CASE("lifetime")
	{
		for (i = 0; i < 3; i++)
			frame(1);
		CHECK(alive() == 1, "three ticks made %d particles, not one", alive());
		glow_tag.trailing_particle_generation_frequency = 0.f; /* (no more) */
		for (i = 0; i < 30; i++)
			frame(1);
		CHECK(alive() == 1, "a particle with 30 ticks to live is gone after %d", 30);
		frame(1);
		CHECK(alive() == 0, "a particle with 30 ticks to live is there after 31");
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
