/* The real particle animation step and draw gate, a particle stepped at a
frame rate: the plasma impact's bolt (one sprite, a span of one tick) must
never be drawn, and longer particles are drawn as they were at 30 Hz. */

#include "harness.h"
#include <math.h>

#define TICKS_PER_SECOND 30
#define PIN(n, floor, ceiling) ((n) < (floor) ? (floor) : ((n) > (ceiling) ? (ceiling) : (n)))
#define TAG_BLOCK_GET_ELEMENT(block, index, type) (&(block)->data[index])

#include "config.inc"

struct bitmap_group_sequence { struct { long count; } sprites; };
struct bitmap_group { struct { long count; struct bitmap_group_sequence *data; } sequences; };
struct particle_definition
{
	unsigned long flags;
	struct { long index; } bitmap;
	short first_sequence_index, initial_sequence_count, looping_sequence_count, final_sequence_count;
};
struct particle_datum
{
	unsigned short flags;
	long definition_index;
	byte state;
	real age, lifespan, frame_time, frame_span;
	short sequence_index, frame_index;
};

static struct bitmap_group_sequence sequence = { { 1 } };
static struct bitmap_group fake_bitmap = { { 1, &sequence } };
static struct particle_definition fake_definition;
static struct particle_datum fake_particle;
static long particles_update_ticks;
static real particles_leftover_ticks;
static real particles_last_update_dt;
static boolean double_speed;
static boolean dead;

#define particle_get(index) (&fake_particle)
#define particle_definition_get(index) (&fake_definition)
#define bitmap_group_get(index) (&fake_bitmap)
#define local_random_range(lower, upper) (lower)
#define particle_die(index) (dead = TRUE)
#define game_players_are_double_speed() double_speed

static boolean particle_next_sequence(long particle_index);

#include "under_test.inc"

/* particle_new's start, then particles_update's step and render_particles's
gate each frame for a second: the number of frames that draw it, the age at
the first. */
static int simulate_flags(unsigned long flags, real phase, real span, real const *dts, int dt_count, real tick_dt,
	real *first_age)
{
	int frame, draws = 0;

	memset(&fake_particle, 0, sizeof(fake_particle));
	fake_definition.flags = flags;
	particles_leftover_ticks = phase;
	double_speed = tick_dt < 1.0f / TICKS_PER_SECOND * 0.75f;
	fake_definition.initial_sequence_count = 1;
	fake_particle.state = _particle_state_next_sequence_initial;
	fake_particle.lifespan = 10.0f;
	fake_particle.frame_span = span;
	fake_particle.frame_time = -1.0f;
	dead = FALSE;
	particle_next_sequence(0);
	fake_particle.frame_index = NONE;
	*first_age = -1.0f;

	for (frame = 0; frame < 240 && !dead; frame++)
	{
		real dt = dts[frame % dt_count];

		/* particles_update's tick count, then its step */
		particles_leftover_ticks += dt * TICKS_PER_SECOND;
		particles_update_ticks = (long)particles_leftover_ticks;
		particles_leftover_ticks -= (real)particles_update_ticks;
		particles_last_update_dt = dt;
		fake_particle.age = dt + fake_particle.age;
		if (!particle_update_frame_time(0, dt))
			break;
		if (particle_drawn(0))
		{
			if (!draws)
				*first_age = fake_particle.age;
			draws++;
		}
	}
	return draws;
}

static int simulate(real span, real const *dts, int dt_count, real tick_dt, real *first_age)
{
	return simulate_flags(0, 0.0f, span, dts, dt_count, tick_dt, first_age);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	real const tick = 1.0f / TICKS_PER_SECOND;
	real first_age;
	int draws;

	CASE("bolt")
	{
		static real const rates[] = { 30, 31, 45, 60, 72, 90, 100, 120, 144, 165, 240, 1000 };
		int index;

		for (index = 0; index < (int)(sizeof(rates) / sizeof(rates[0])); index++)
		{
			real dt = 1.0f / rates[index];

			draws = simulate(tick, &dt, 1, tick, &first_age);
			CHECK(draws == 0, "%g Hz: one-tick particle drawn %d times, first at age %g", rates[index], draws,
				first_age);
		}
		return 0;
	}
	CASE("bolt-jitter")
	{
		/* a frame pacing that is not even */
		static real const dts[] = { 0.0081f, 0.0086f, 0.0079f, 0.0091f, 0.0083f, 0.0077f, 0.0088f };

		draws = simulate(tick, dts, (int)(sizeof(dts) / sizeof(dts[0])), tick, &first_age);
		CHECK(draws == 0, "drawn %d times, first at age %g", draws, first_age);
		return 0;
	}
	CASE("tick-paced")
	{
		/* a fixed timestep: drawn at its first frame, after a tick, as on the Xbox */
		draws = simulate(3 * tick, &tick, 1, tick, &first_age);
		CHECK(draws == 3 && first_age == tick, "drawn %d times, first at age %g", draws, first_age);
		return 0;
	}
	CASE("double-speed")
	{
		real const half_tick = 1.0f / (2 * TICKS_PER_SECOND);

		draws = simulate(3 * tick, &half_tick, 1, half_tick, &first_age);
		CHECK(draws == 6 && first_age == half_tick, "drawn %d times, first at age %g", draws, first_age);
		return 0;
	}
	CASE("frames")
	{
		/* longer particles: drawn from the first frame past a tick */
		static real const rates[] = { 60, 90, 120, 144, 240 };
		int index;

		for (index = 0; index < (int)(sizeof(rates) / sizeof(rates[0])); index++)
		{
			real dt = 1.0f / rates[index];

			draws = simulate(3 * tick, &dt, 1, tick, &first_age);
			CHECK(draws > 0 && first_age > tick && first_age <= tick + dt * 1.01f,
				"%g Hz: drawn %d times, first at age %g", rates[index], draws, first_age);
		}
		return 0;
	}
	CASE("once-a-tick")
	{
		/* a flash that animates once a tick, one sprite: drawn from its first step until it dies a tick later,
		whatever the phase of particles_update's ticks */
		static real const phases[] = { 0.0f, 0.3f, 0.6f, 0.9f, 0.99f };
		real dt = 1.0f / 120;
		int index;

		for (index = 0; index < (int)(sizeof(phases) / sizeof(phases[0])); index++)
		{
			draws = simulate_flags(FLAG(_particle_definition_animate_once_per_frame_bit), phases[index], tick, &dt, 1,
				tick, &first_age);
			CHECK(draws >= 3 && draws <= 5, "phase %g: drawn %d times, first at age %g", phases[index], draws, first_age);
		}
		draws = simulate_flags(FLAG(_particle_definition_animate_once_per_frame_bit), 0.0f, tick, &tick, 1, tick,
			&first_age);
		CHECK(draws == 1 && first_age == tick, "tick-paced: drawn %d times, first at age %g", draws, first_age);
		return 0;
	}
	CASE("created-after-update")
	{
		/* a particle created after a tick's update is drawn at age 0, as it was */
		CHECK(particle_age_drawn(0.0f, tick, tick), "not drawn");
		CHECK(!particle_age_drawn(0.0f, tick / 4, tick), "drawn in a frame between the ticks");
		return 0;
	}
	fprintf(stderr, "unknown case %s\n", case_name);
	return 2;
}
