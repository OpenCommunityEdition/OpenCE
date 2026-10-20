#include "harness.h"
#include <math.h>
#include <string.h>

/* The host's bodies drawn as wireframes (physics_draw.c), with the game's
debug drawing recorded instead of drawn. A fake world of two ragdolls (one
awake with its second part torn off, one asleep), a third not yet posed,
and two pieces of debris, one awake and one asleep; every other debris id
has no body. */
typedef struct { real_vector3d v; real w; } real_quaternion;
typedef union { real n[4]; struct { real alpha, red, green, blue; }; } real_argb_color;

#include "config.inc"

#define DRAWN_MAX 256

/* each debug shape drawn: 's' a sphere, 'p' a pill, 'l' a line */
static struct drawn
{
	char kind;
	real_point3d a;
	real_vector3d b;
	real size;
	real_argb_color color;
} drawn[DRAWN_MAX];
static long drawn_count;

static void record(char kind, real_point3d const *a, real_vector3d const *b, real size, real_argb_color const *color)
{
	struct drawn *d;

	CHECK(drawn_count < DRAWN_MAX, "drew too much");
	d = &drawn[drawn_count++];
	memset(d, 0, sizeof(*d));
	d->kind = kind;
	d->a = *a;
	if (b)
		d->b = *b;
	d->size = size;
	d->color = *color;
}

static void render_debug_sphere(boolean immediate, real_point3d const *center, real radius, real_argb_color const *color)
{
	CHECK(immediate, "sphere: cached, not drawn at once");
	record('s', center, NULL, radius, color);
}

static void render_debug_pill(boolean immediate, real_point3d const *base, real_vector3d const *height, real width,
	real_argb_color const *color)
{
	CHECK(immediate, "pill: cached, not drawn at once");
	record('p', base, height, width, color);
}

static void render_debug_line(boolean immediate, real_point3d const *p0, real_point3d const *p1,
	real_argb_color const *color)
{
	real_vector3d d;

	CHECK(immediate, "line: cached, not drawn at once");
	d.i = p1->x - p0->x;
	d.j = p1->y - p0->y;
	d.k = p1->z - p0->z;
	record('l', p0, &d, 0.0f, color);
}

static boolean setting;
static boolean host;
static unsigned long changes = 1;
static int config_boolean(const char *name)
{
	CHECK(strcmp(name, "debug.physics_draw") == 0, "asked for %s", name);
	return setting;
}
static unsigned long config_changes(void) { return changes; }
static boolean physics_jolt_enabled(void) { return host; }

static struct ragdoll_body body;
static struct ragdoll ragdolls[3];

static struct ragdoll *ragdolls_get(long id)
{
	return id >= 1 && id <= 3 && ragdolls[id - 1].used ? &ragdolls[id - 1] : NULL;
}

/* (part 1's joint torn: it is its own piece) */
static short ragdoll_part_piece(struct ragdoll const *ragdoll, short part)
{
	return ragdoll->broken_joints & (1u << part) ? part : 0;
}

static long debris_asked_first = -1, debris_asked_last = -1, debris_asked;
static struct physics_debris_body debris[2];

static boolean physics_debris_posed_body(long id, struct physics_debris_body *result, boolean *awake)
{
	if (debris_asked_first < 0)
		debris_asked_first = id;
	debris_asked_last = id;
	debris_asked++;
	if (id != PHYSICS_DEBRIS_FIRST_ID + 3 && id != PHYSICS_DEBRIS_FIRST_ID + 40)
		return FALSE;
	*result = debris[id == PHYSICS_DEBRIS_FIRST_ID + 3 ? 0 : 1];
	*awake = id == PHYSICS_DEBRIS_FIRST_ID + 3;
	return TRUE;
}

#include "under_test.inc"

static boolean near(real a, real b) { return fabsf(a - b) < 0.0001f; }
static boolean point_near(real_point3d const *p, real x, real y, real z) { return near(p->x, x) && near(p->y, y) && near(p->z, z); }
static boolean same_color(real_argb_color const *a, real_argb_color const *b)
{
	return a->alpha == b->alpha && a->red == b->red && a->green == b->green && a->blue == b->blue;
}

/* a pose at a point, turned a quarter about z (x to y) */
static struct ragdoll_pose quarter_turn(real x, real y, real z)
{
	struct ragdoll_pose pose;

	pose.position.x = x;
	pose.position.y = y;
	pose.position.z = z;
	pose.rotation.v.i = 0.0f;
	pose.rotation.v.j = 0.0f;
	pose.rotation.v.k = 0.70710678f;
	pose.rotation.w = 0.70710678f;
	return pose;
}

static void world(void)
{
	struct ragdoll_part *part;

	memset(&body, 0, sizeof(body));
	body.part_count = 2;
	part = &body.parts[0];
	part->shape = _ragdoll_shape_capsule;
	part->size[0] = 0.1f;
	part->size[1] = 0.5f;
	part->center.x = 0.2f;
	part = &body.parts[1];
	part->shape = _ragdoll_shape_sphere;
	part->size[0] = 0.25f;
	part->center.y = 1.0f;

	memset(ragdolls, 0, sizeof(ragdolls));
	ragdolls[0].used = TRUE;
	ragdolls[0].body = &body;
	ragdolls[0].posed = TRUE;
	ragdolls[0].awake = TRUE;
	ragdolls[0].broken_joints = 1u << 1;
	ragdolls[0].poses[0] = quarter_turn(1.0f, 2.0f, 3.0f);
	ragdolls[0].poses[1] = quarter_turn(10.0f, 0.0f, 0.0f);
	ragdolls[1] = ragdolls[0];
	ragdolls[1].awake = FALSE;
	ragdolls[2] = ragdolls[0];
	ragdolls[2].posed = FALSE;

	memset(debris, 0, sizeof(debris));
	debris[0].shape = HOST_PHYSICS_SHAPE_BOX;
	debris[0].size[0] = 1.0f;
	debris[0].size[1] = 2.0f;
	debris[0].size[2] = 3.0f;
	debris[0].pose = quarter_turn(0.0f, 0.0f, 5.0f);
	debris[1].shape = HOST_PHYSICS_SHAPE_CAPSULE;
	debris[1].size[0] = 0.005f;
	debris[1].size[1] = 0.01f;
	debris[1].pose = quarter_turn(0.0f, 0.0f, 0.0f);

	setting = TRUE;
	host = TRUE;
	changes++;
	drawn_count = 0;
	debris_asked_first = debris_asked_last = -1;
	debris_asked = 0;
}

static long count_kind(char kind)
{
	long count = 0;
	long index;

	for (index = 0; index < drawn_count; index++)
		count += drawn[index].kind == kind;
	return count;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	world();

	/* nothing with the setting off */
	CASE("off")
	{
		setting = FALSE;
		changes++;
		physics_draw_render();
		CHECK(drawn_count == 0 && debris_asked == 0, "off: drew %ld, asked %ld", drawn_count, debris_asked);
		CHECK(!physics_draw_enabled(), "off: enabled");
		return 0;
	}
	/* nor without the host's physics (another build, or ragdolls and debris
	both off) */
	CASE("off without the host")
	{
		host = FALSE;
		physics_draw_render();
		CHECK(drawn_count == 0, "no host: drew %ld", drawn_count);
		return 0;
	}
	/* the setting read again once the settings change */
	CASE("setting changed")
	{
		CHECK(physics_draw_enabled(), "on: not enabled");
		setting = FALSE;
		CHECK(physics_draw_enabled(), "read before the settings changed");
		changes++;
		CHECK(!physics_draw_enabled(), "not read again");
		return 0;
	}
	/* the capsule along its frame's x axis, turned to y, centred on its
	part's centre: from (1, 1.7, 3) one unit up y, as wide as it is */
	CASE("capsule")
	{
		struct drawn const *d;

		physics_draw_render();
		d = &drawn[0];
		CHECK(d->kind == 'p', "capsule: drew %c", d->kind);
		CHECK(point_near(&d->a, 1.0f, 1.7f, 3.0f), "capsule: from (%g %g %g)", d->a.x, d->a.y, d->a.z);
		CHECK(near(d->b.i, 0.0f) && near(d->b.j, 1.0f) && near(d->b.k, 0.0f), "capsule: along (%g %g %g)",
			d->b.i, d->b.j, d->b.k);
		CHECK(d->size == 0.1f, "capsule: %g wide", d->size);
		return 0;
	}
	/* the sphere at its centre turned with its part: (0, 1, 0) to (-1, 0, 0) */
	CASE("sphere")
	{
		struct drawn const *d;

		physics_draw_render();
		d = &drawn[1];
		CHECK(d->kind == 's', "sphere: drew %c", d->kind);
		CHECK(point_near(&d->a, 9.0f, 0.0f, 0.0f), "sphere: at (%g %g %g)", d->a.x, d->a.y, d->a.z);
		CHECK(d->size == 0.25f, "sphere: radius %g", d->size);
		return 0;
	}
	/* the box's twelve edges, each along one of its turned axes and as
	long as the box is there, its corners about its centre */
	CASE("box")
	{
		long lines = 0;
		long along[3] = {0, 0, 0};
		long index;

		physics_draw_render();
		for (index = 0; index < drawn_count; index++)
		{
			struct drawn const *d = &drawn[index];

			if (d->kind != 'l')
				continue;
			lines++;
			CHECK(near(fabsf(d->a.x), 2.0f) && near(fabsf(d->a.y), 1.0f) && near(fabsf(d->a.z - 5.0f), 3.0f),
				"box: a corner at (%g %g %g)", d->a.x, d->a.y, d->a.z);
			/* (x turned to y, y to -x) */
			if (near(d->b.i, 0.0f) && near(d->b.j, 2.0f) && near(d->b.k, 0.0f))
				along[0]++;
			else if (near(d->b.i, -4.0f) && near(d->b.j, 0.0f) && near(d->b.k, 0.0f))
				along[1]++;
			else if (near(d->b.i, 0.0f) && near(d->b.j, 0.0f) && near(d->b.k, 6.0f))
				along[2]++;
			else
				CHECK(FALSE, "box: an edge along (%g %g %g)", d->b.i, d->b.j, d->b.k);
		}
		CHECK(lines == 12 && along[0] == 4 && along[1] == 4 && along[2] == 4, "box: %ld edges (%ld %ld %ld)", lines,
			along[0], along[1], along[2]);
		return 0;
	}
	/* green awake, red torn, grey asleep (torn or not), yellow debris awake */
	CASE("colours")
	{
		physics_draw_render();
		CHECK(drawn_count == 2 + 2 + 12 + 1, "colours: drew %ld", drawn_count);
		CHECK(same_color(&drawn[0].color, &physics_draw_awake), "an awake part");
		CHECK(same_color(&drawn[1].color, &physics_draw_torn), "a torn part");
		CHECK(same_color(&drawn[2].color, &physics_draw_asleep) && same_color(&drawn[3].color, &physics_draw_asleep),
			"an asleep ragdoll's parts");
		CHECK(same_color(&drawn[4].color, &physics_draw_debris_awake), "awake debris");
		CHECK(same_color(&drawn[drawn_count - 1].color, &physics_draw_asleep), "debris asleep");
		CHECK(!same_color(&physics_draw_awake, &physics_draw_asleep) && !same_color(&physics_draw_torn, &physics_draw_awake) &&
			!same_color(&physics_draw_debris_awake, &physics_draw_awake), "colours alike");
		return 0;
	}
	/* a ragdoll the host has not posed yet is not drawn: two ragdolls' parts */
	CASE("unposed")
	{
		physics_draw_render();
		CHECK(count_kind('p') == 2 + 1 && count_kind('s') == 2, "unposed: %ld pills, %ld spheres", count_kind('p'),
			count_kind('s'));
		return 0;
	}
	/* every debris id asked for, in order */
	CASE("every debris id")
	{
		physics_draw_render();
		CHECK(debris_asked_first == PHYSICS_DEBRIS_FIRST_ID && debris_asked_last == PHYSICS_DEBRIS_FIRST_ID + PHYSICS_DEBRIS_MAX - 1 &&
			debris_asked == PHYSICS_DEBRIS_MAX, "asked %ld to %ld, %ld", debris_asked_first, debris_asked_last, debris_asked);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
