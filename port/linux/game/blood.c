/*
BLOOD.C

Dynamic blood (Video Settings' Dynamic Blood, game.dynamic_blood, off by
default), with the game's own blood (each character's: its collision model's
area damage effect, "effects\blood aoe elite", and the splats of its kind,
"effects\decals\blood splats\blood elite splat"):

- A body hit throws blood onto the wall or slope behind it, and blood lands
  on the level's two-sided surfaces too (a tree, a fence).
- Blood on a wall (any blood decal landing on a steep surface) runs: drips,
  each a trail of thin streaks laid down the surface as it runs, slowing,
  thinning and stopping at the surface's edge, a drop at its head, and
  drops falling from edges and undersides (blood_drops.c). No animated
  pictures: the trails are the game's own blood splats, drawn as they run.

Bullet holes and scorch marks are laid as the game lays them.

Everything here is this machine's own (local random numbers, nothing over
the network). Units: world units (1 = 3.048 m) and ticks (1/30 s).
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "objects/damage.h"
#include "effects/decals.h"
#include "effects/decal_definitions.h"
#include "effects/effect_definitions.h"
#include "physics/collision_model_definitions.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "tag_files/tag_files.h"
#include "game/game.h"
#include "units/bipeds.h"

#include "blood.h"

#include <math.h>
#include <string.h>

#if BLOOD_EFFECTS

/* cache_files.c */
long tag_loaded(long group_tag, char const *name);

/* ---------- constants */

/* render_interpolation.c: how far between the last tick and the next this frame is */
real render_interpolation_fraction(void);

/* (a run: the splat's picture nearly whole, stretched along it, so its
ragged round outline is the run's edge and ends, short stretches overlapping
in a chain; a bead: its picture whole, round. The picture's edge is see-
through, so what shows is narrower than the quad: BLOOD_RUN_WIDEN) */
#define BLOOD_RUN_CORE_ALONG 0.85f
#define BLOOD_RUN_CORE_ACROSS 1.f
#define BLOOD_BEAD_CORE 0.9f
#define BLOOD_RUN_WIDEN 1.6f

enum
{
	BLOOD_MAXIMUM_KINDS = 32,
	BLOOD_MAXIMUM_DRIPS = 160,
};

#define BLOOD_METRES 3.048f
/* a surface this steep (its normal's height under this) runs */
#define BLOOD_STEEP 0.9f
/* (a drip's run sits this far off its surface) */
#define BLOOD_LIFT 0.004f

/* ---------- structures */

/* a biped's blood: its spray and its splats */
struct blood_kind
{
	long biped_definition_index; /* NONE: free */
	long spray; /* effect */
	long splat; /* decal */
	long splat_large;
	long splat_small;
};

/* blood running down a surface */
struct blood_drip
{
	boolean active;
	byte pad[3];
	long decal_definition_index;
	short cluster_index;
	short pad_cluster;
	real_point3d position;
	real_vector3d normal;
	real_point3d laid; /* where the last streak ended */
	real speed; /* world units a tick */
	real slowing; /* its speed kept each tick */
	real remaining; /* world units */
	short age; /* ticks */
	short pause; /* ticks it holds still, gathering (a bead) */
	real length;
	real width;
	real wander; /* radians off straight down it runs, wandering */
	real start_width;
	/* (its live streak and bead, moved as it runs) */
	long live_decal;
	long live_bead;
	real_point3d head_before; /* its head a tick ago and now (world) */
	real_point3d head;
	real_vector3d live_normal;
	real live_half_width;
};

/* ---------- globals */

struct blood_parameters blood_parameters = { 1.f, 1.f, 1.f, 1.f };

static struct blood_kind blood_kinds[BLOOD_MAXIMUM_KINDS];
static struct blood_drip blood_drips[BLOOD_MAXIMUM_DRIPS];
static long blood_last_time = NONE;
/* (blood's own decals landing: no drips of drips) */
static boolean blood_laying = FALSE;
/* a decal definition known to be blood, or not: the last asked */
static long blood_checked_definition = NONE;
static boolean blood_checked_is_blood = FALSE;

/* ---------- vectors */

static real blood_dot(real_vector3d const *a, real_vector3d const *b)
{
	return a->i * b->i + a->j * b->j + a->k * b->k;
}

static real blood_normalize(real_vector3d *v)
{
	real length = (real)sqrt(blood_dot(v, v));

	if (length > 1e-6f && valid_real(length))
	{
		v->i /= length;
		v->j /= length;
		v->k /= length;
		return length;
	}
	return 0.f;
}

static void blood_add(real_point3d *p, real_vector3d const *v, real scale)
{
	p->x += v->i * scale;
	p->y += v->j * scale;
	p->z += v->k * scale;
}

/* down a surface: gravity along it (none on a floor) */
static boolean blood_downhill(real_vector3d const *normal, real_vector3d *down)
{
	down->i = normal->i * normal->k;
	down->j = normal->j * normal->k;
	down->k = -1.f + normal->k * normal->k;
	return blood_normalize(down) > 0.f;
}

/* a point and a normal from a node's frame to the world, and back */
static boolean blood_name_has(long tag_index, char const *word)
{
	char const *name = tag_index != NONE && tag_index != 0 ? tag_get_name(tag_index) : NULL;

	return name && strstr(name, word) != NULL;
}

/* the decal "effects\decals\blood splats\<name>", or NONE */
static long blood_splat_named(char const *name)
{
	char path[256];

	snprintf(path, sizeof(path), "effects\\decals\\blood splats\\%s", name);
	return tag_loaded(DECAL_GROUP_TAG, path);
}

static struct blood_kind const *blood_kind_get(long biped_index)
{
	struct object_datum *object = object_try_and_get(biped_index);
	struct object_definition *definition;
	struct blood_kind *kind = NULL;
	short index;

	if (!object)
		return NULL;
	for (index = 0; index < BLOOD_MAXIMUM_KINDS; index++)
	{
		if (blood_kinds[index].biped_definition_index == object->definition_index)
			return &blood_kinds[index];
		if (!kind && blood_kinds[index].biped_definition_index == NONE)
			kind = &blood_kinds[index];
	}
	if (!kind)
		kind = &blood_kinds[object->definition_index % BLOOD_MAXIMUM_KINDS];

	kind->biped_definition_index = object->definition_index;
	kind->spray = NONE;
	kind->splat = kind->splat_large = kind->splat_small = NONE;
	definition = object_definition_get(object->definition_index);

	/* its spray: the area damage effect of its collision model ("effects\blood
	aoe elite"), and its splats by the word after "aoe" */
	if (definition && definition->object.collision_model.index != NONE)
	{
		struct collision_model *model = collision_model_definition_get(definition->object.collision_model.index);
		char const *name;

		if (model)
			kind->spray = model->resistance.area_damage_effect.index;
		name = kind->spray != NONE ? tag_get_name(kind->spray) : NULL;
		if (name && strstr(name, "blood"))
		{
			char const *word = strstr(name, "aoe ");
			char species[64] = "";

			if (word)
				snprintf(species, sizeof(species), "%s", word + 4);
			if (!species[0] || !strcmp(species, "human"))
			{
				kind->splat = blood_splat_named("blood splat");
				kind->splat_large = blood_splat_named("blood splat large");
				kind->splat_small = blood_splat_named("blood splat small");
			}
			else
			{
				char splat[96];

				snprintf(splat, sizeof(splat), "blood %s splat", species);
				kind->splat = blood_splat_named(splat);
				snprintf(splat, sizeof(splat), "blood splat %s small", species);
				kind->splat_small = blood_splat_named(splat);
			}
		}
		else
		{
			/* (no blood: a sentinel, a robot) */
			kind->spray = NONE;
		}
	}
	if (kind->splat_large == NONE)
		kind->splat_large = kind->splat;
	if (kind->splat_small == NONE)
		kind->splat_small = kind->splat;
	return kind;
}

/* (not effects_corpse_nonviolent: that is the game's rule that a corpse,
once settled, shows no gore when shot, and it is on by default; this is
blood_parameters.enabled's to decide) */
/* port_config.c */
char const *config_string(char const *name);
int config_boolean(char const *name);
unsigned long config_changes(void);

static void blood_drop_fall(long decal_definition_index, real_point3d const *point, real radius);

/* Video Settings' Dynamic Blood (config.toml's game.dynamic_blood, off by
default): all of this module's blood, on the level's walls and slopes, and
running down them; off, the game's own blood alone. Read again whenever a
setting changes */
static boolean blood_violence_allowed(void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean on = FALSE;

	if (changes != config_changes())
	{
		changes = config_changes();
		on = config_boolean("game.dynamic_blood") != 0;
	}
	return on && blood_parameters.enabled >= 0.5f && decals_enabled;
}

/* drops at a run's head and falling from edges and undersides */
boolean blood_drops_animated(
	void)
{
	return blood_violence_allowed() && blood_parameters.drips >= 0.5f;
}

static void blood_drip_start(long decal_definition_index, real_point3d const *point, real_vector3d const *normal,
	real radius, short cluster_index);

/* a drip's quad: its run from one point to another (a thin
slice down the middle of the splat's picture stretched along it, soft
across), or a round bead at a point (the picture's round middle); its
corners as decal_new_quad winds them, clockwise seen from in front */
static void blood_drip_corners(struct blood_drip const *drip, real_point3d const *from, real_point3d const *to,
	boolean bead, real lift, real_point3d *middle, real_vector3d *forward, real *half_length, real *half_width,
	real_point3d corners[4])
{
	real_vector3d const *normal = &drip->live_normal;
	real_vector3d left;
	real length;
	real dot;
	short corner;

	forward->i = to->x - from->x;
	forward->j = to->y - from->y;
	forward->k = to->z - from->z;
	dot = blood_dot(forward, normal);
	forward->i -= normal->i * dot;
	forward->j -= normal->j * dot;
	forward->k -= normal->k * dot;
	length = blood_normalize(forward);
	if (length == 0.f && !blood_downhill(normal, forward))
		perpendicular3d(normal, forward);
	left.i = normal->j * forward->k - normal->k * forward->j;
	left.j = normal->k * forward->i - normal->i * forward->k;
	left.k = normal->i * forward->j - normal->j * forward->i;
	blood_normalize(&left);
	middle->x = (from->x + to->x) * 0.5f + normal->i * lift;
	middle->y = (from->y + to->y) * 0.5f + normal->j * lift;
	middle->z = (from->z + to->z) * 0.5f + normal->k * lift;
	if (bead)
	{
		*half_length = drip->live_half_width * BLOOD_RUN_WIDEN * 1.25f;
		*half_width = drip->live_half_width * BLOOD_RUN_WIDEN * 1.15f;
	}
	else
	{
		*half_length = length * 0.5f + drip->live_half_width * BLOOD_RUN_WIDEN * 0.7f;
		*half_width = drip->live_half_width * BLOOD_RUN_WIDEN;
	}
	for (corner = 0; corner < 4; corner++)
	{
		real x = (corner >= 2) ? *half_length : -*half_length;
		real y = (corner == 1 || corner == 2) ? *half_width : -*half_width;

		corners[corner].x = middle->x + forward->i * x + left.i * y;
		corners[corner].y = middle->y + forward->j * x + left.j * y;
		corners[corner].z = middle->z + forward->k * x + left.k * y;
	}
}

static long blood_drip_quad(struct blood_drip const *drip, real_point3d const *from, real_point3d const *to,
	boolean bead, real lift)
{
	real_point3d middle;
	real_vector3d forward;
	real half_length;
	real half_width;
	real_point3d corners[4];
	long decal_index;

	blood_drip_corners(drip, from, to, bead, lift, &middle, &forward, &half_length, &half_width, corners);
	if (half_length <= 0.f || half_width <= 0.f || drip->cluster_index == NONE)
		return NONE;
	decal_port_texture_core_along = bead ? BLOOD_BEAD_CORE : BLOOD_RUN_CORE_ALONG;
	decal_port_texture_core_across = bead ? BLOOD_BEAD_CORE : BLOOD_RUN_CORE_ACROSS;
	decal_port_full_intensity = TRUE;
	decal_index = decal_new_quad(drip->decal_definition_index, &middle, &drip->live_normal, &forward, half_length,
		half_width, drip->cluster_index, corners);
	decal_port_texture_core_along = decal_port_texture_core_across = 1.f;
	decal_port_full_intensity = FALSE;
	return decal_index;
}

/* its live run (from where its last kept run ended to its head) and the
bead at its head, moved there, or made if gone */
static void blood_drip_place(struct blood_drip *drip, real_point3d const *from, real_point3d const *head, boolean run)
{
	real_point3d middle;
	real_vector3d forward;
	real half_length;
	real half_width;
	real_point3d corners[4];

	if (drip->cluster_index == NONE || drip->live_half_width <= 0.f)
		return;
	if (run && distance3d(from, head) > drip->live_half_width * 0.2f)
	{
		blood_drip_corners(drip, from, head, FALSE, BLOOD_LIFT * 0.5f, &middle, &forward, &half_length, &half_width, corners);
		if (drip->live_decal == NONE ||
			!decal_move_quad(drip->live_decal, drip->decal_definition_index, corners, drip->cluster_index))
		{
			drip->live_decal = blood_drip_quad(drip, from, head, FALSE, BLOOD_LIFT * 0.5f);
		}
	}
	blood_drip_corners(drip, head, head, TRUE, BLOOD_LIFT, &middle, &forward, &half_length, &half_width, corners);
	if (drip->live_bead == NONE ||
		!decal_move_quad(drip->live_bead, drip->decal_definition_index, corners, drip->cluster_index))
	{
		drip->live_bead = blood_drip_quad(drip, head, head, TRUE, BLOOD_LIFT);
	}
}

/* every frame: each running drip drawn out to where its head
is between ticks (smooth, not a stretch a tick) */
static void blood_drips_render(void)
{
	real fraction = PIN(render_interpolation_fraction(), 0.f, 1.f);
	short index;

	for (index = 0; index < BLOOD_MAXIMUM_DRIPS; index++)
	{
		struct blood_drip *drip = &blood_drips[index];
		real_point3d head;

		if (!drip->active || drip->cluster_index == NONE ||
			(drip->live_decal == NONE && drip->live_bead == NONE))
		{
			continue;
		}
		head.x = drip->head_before.x + (drip->head.x - drip->head_before.x) * fraction;
		head.y = drip->head_before.y + (drip->head.y - drip->head_before.y) * fraction;
		head.z = drip->head_before.z + (drip->head.z - drip->head_before.z) * fraction;
		blood_laying = TRUE;
		blood_drip_place(drip, &drip->laid, &head, TRUE);
		blood_laying = FALSE;
	}
}

void blood_render_update(
	void)
{
	if (blood_violence_allowed() && blood_parameters.drips >= 0.5f)
		blood_drips_render();
}

/* ---------- drips */

/* the patterns a splat runs in: each splat picks one, and each of its drips
its own length, speed, width, wander and moment to start */
enum
{
	_blood_drip_pattern_single, /* one run */
	_blood_drip_pattern_pair, /* two, side by side */
	_blood_drip_pattern_curtain, /* a row across the splat, of all lengths */
	_blood_drip_pattern_fan, /* from one spot, spreading as they run */
	_blood_drip_pattern_trickle, /* many short stubs */
	_blood_drip_pattern_runner, /* one long heavy run, short ones beside it */
	_blood_drip_pattern_late, /* one, then more after a while (still seeping) */
	_blood_drip_pattern_cascade, /* from heights down the splat, one under another */
	_blood_drip_pattern_drizzle, /* many fine quick threads */
	_blood_drip_pattern_sheet, /* one wide slow run */
	_blood_drip_pattern_wander, /* a few that weave as they run */
	_blood_drip_pattern_burst, /* a spray of runs, starting through some seconds */
	_blood_drip_pattern_twin, /* two long runs that spread apart */
	NUMBER_OF_BLOOD_DRIP_PATTERNS
};

/* how a kind of blood runs, by its decal's name: an Elite's and a Jackal's
half again as fast, a Grunt's at two thirds the speed and half again as
thick */
static void blood_drip_viscosity(long decal_definition_index, real *speed_scale, real *thickness_scale)
{
	*speed_scale = 1.f;
	*thickness_scale = 1.f;
	if (decal_definition_index == NONE)
		return;
	if (blood_name_has(decal_definition_index, "elite") || blood_name_has(decal_definition_index, "jackal"))
		*speed_scale = 1.5f;
	else if (blood_name_has(decal_definition_index, "grunt"))
	{
		*speed_scale = 0.65f;
		*thickness_scale = 1.5f;
	}
}

static void blood_drip_start(long decal_definition_index, real_point3d const *point, real_vector3d const *normal,
	real radius, short cluster_index)
{
	short pattern = (short)local_random_range(0, NUMBER_OF_BLOOD_DRIP_PATTERNS);
	short count;
	short made;
	real fan_spot = real_local_random_range(-0.3f, 0.3f);
	real speed_scale = 1.f;
	real thickness_scale = 1.f;

	switch (pattern)
	{
	case _blood_drip_pattern_single: count = 1; break;
	case _blood_drip_pattern_pair: count = 2; break;
	case _blood_drip_pattern_curtain: count = (short)local_random_range(3, 7); break;
	case _blood_drip_pattern_fan: count = (short)local_random_range(2, 5); break;
	case _blood_drip_pattern_trickle: count = (short)local_random_range(4, 8); break;
	case _blood_drip_pattern_runner: count = (short)local_random_range(2, 4); break;
	case _blood_drip_pattern_cascade: count = (short)local_random_range(3, 6); break;
	case _blood_drip_pattern_drizzle: count = (short)local_random_range(6, 11); break;
	case _blood_drip_pattern_sheet: count = 1; break;
	case _blood_drip_pattern_wander: count = (short)local_random_range(2, 4); break;
	case _blood_drip_pattern_burst: count = (short)local_random_range(5, 10); break;
	case _blood_drip_pattern_twin: count = 2; break;
	default: count = (short)local_random_range(2, 4); break;
	}
	/* (each kind's blood runs its own way: an Elite's and a Jackal's
	quicker, a Grunt's slow and thick, and as many runs from a small splat) */
	blood_drip_viscosity(decal_definition_index, &speed_scale, &thickness_scale);
	if (radius < 0.05f && thickness_scale <= 1.f)
		count = (short)MAX(1, count / 2);

	for (made = 0; made < count; made++)
	{
		struct blood_drip *drip = NULL;
		real_vector3d down;
		real_vector3d across;
		real_point3d start = *point;
		real length_scale = real_local_random_range(1.f, 3.5f);
		real width_scale = real_local_random_range(0.08f, 0.16f);
		real speed = real_local_random_range(0.25f, 0.6f);
		real wander = real_local_random_range(-0.15f, 0.15f);
		real offset_across = real_local_random_range(-0.5f, 0.5f);
		real offset_down = real_local_random_range(0.f, 0.4f);
		short pause = (short)local_random_range(0, 8);
		short index;

		for (index = 0; index < BLOOD_MAXIMUM_DRIPS; index++)
		{
			if (!blood_drips[index].active)
			{
				drip = &blood_drips[index];
				break;
			}
		}
		if (!drip || !blood_downhill(normal, &down))
			return;

		switch (pattern)
		{
		case _blood_drip_pattern_single:
			length_scale = real_local_random_range(1.5f, 4.5f);
			break;
		case _blood_drip_pattern_pair:
			offset_across = (made ? 1.f : -1.f) * real_local_random_range(0.1f, 0.35f);
			pause = (short)(made * local_random_range(0, 20));
			break;
		case _blood_drip_pattern_curtain:
			/* (spread evenly across, some long, most short) */
			offset_across = -0.6f + 1.2f * (made + real_local_random_range(0.2f, 0.8f)) / count;
			length_scale = local_random_range(0, 3) ? real_local_random_range(0.4f, 1.6f) : real_local_random_range(2.f, 4.f);
			width_scale = real_local_random_range(0.05f, 0.11f);
			pause = (short)local_random_range(0, 25);
			break;
		case _blood_drip_pattern_fan:
			offset_across = fan_spot + real_local_random_range(-0.05f, 0.05f);
			wander = -0.35f + 0.7f * (made + 0.5f) / count + real_local_random_range(-0.05f, 0.05f);
			length_scale = real_local_random_range(1.f, 3.f);
			break;
		case _blood_drip_pattern_trickle:
			length_scale = real_local_random_range(0.3f, 1.f);
			width_scale = real_local_random_range(0.05f, 0.1f);
			speed = real_local_random_range(0.15f, 0.35f);
			pause = (short)local_random_range(0, 40);
			break;
		case _blood_drip_pattern_runner:
			if (made == 0)
			{
				length_scale = real_local_random_range(3.5f, 6.f);
				width_scale = real_local_random_range(0.15f, 0.22f);
				speed = real_local_random_range(0.4f, 0.8f);
				offset_across = real_local_random_range(-0.15f, 0.15f);
			}
			else
			{
				length_scale = real_local_random_range(0.3f, 1.2f);
				width_scale = real_local_random_range(0.05f, 0.1f);
				pause = (short)local_random_range(5, 30);
			}
			break;
		case _blood_drip_pattern_late:
			/* (the first at once, the rest seeping out seconds later, slowly) */
			if (made > 0)
			{
				pause = (short)local_random_range(30, 120);
				speed = real_local_random_range(0.12f, 0.3f);
				length_scale = real_local_random_range(0.6f, 2.5f);
			}
			break;
		case _blood_drip_pattern_cascade:
			offset_down = 0.9f * made / count + real_local_random_range(0.f, 0.15f);
			offset_across = real_local_random_range(-0.35f, 0.35f);
			length_scale = real_local_random_range(0.8f, 2.8f);
			pause = (short)(made * local_random_range(4, 14));
			break;
		case _blood_drip_pattern_drizzle:
			width_scale = real_local_random_range(0.03f, 0.06f);
			length_scale = real_local_random_range(0.6f, 2.2f);
			speed = real_local_random_range(0.45f, 0.9f);
			offset_across = real_local_random_range(-0.7f, 0.7f);
			pause = (short)local_random_range(0, 15);
			break;
		case _blood_drip_pattern_sheet:
			width_scale = real_local_random_range(0.25f, 0.4f);
			length_scale = real_local_random_range(2.f, 4.5f);
			speed = real_local_random_range(0.12f, 0.25f);
			offset_across = real_local_random_range(-0.1f, 0.1f);
			break;
		case _blood_drip_pattern_wander:
			wander = (made & 1 ? 1.f : -1.f) * real_local_random_range(0.25f, 0.45f);
			length_scale = real_local_random_range(1.5f, 3.5f);
			break;
		case _blood_drip_pattern_burst:
			offset_across = real_local_random_range(-0.8f, 0.8f);
			offset_down = real_local_random_range(0.f, 0.7f);
			length_scale = real_local_random_range(0.3f, 3.f);
			width_scale = real_local_random_range(0.04f, 0.14f);
			pause = (short)local_random_range(0, 90);
			break;
		case _blood_drip_pattern_twin:
			offset_across = (made ? 1.f : -1.f) * real_local_random_range(0.05f, 0.15f);
			wander = (made ? 1.f : -1.f) * real_local_random_range(0.1f, 0.25f);
			length_scale = real_local_random_range(3.f, 5.5f);
			width_scale = real_local_random_range(0.1f, 0.18f);
			break;
		}
		speed *= speed_scale;
		width_scale *= thickness_scale;

		/* (from somewhere in the splat's lower half) */
		across.i = normal->j * down.k - normal->k * down.j;
		across.j = normal->k * down.i - normal->i * down.k;
		across.k = normal->i * down.j - normal->j * down.i;
		blood_add(&start, &across, offset_across * radius);
		blood_add(&start, &down, offset_down * radius);

		drip->active = TRUE;
		drip->decal_definition_index = decal_definition_index;
		drip->cluster_index = cluster_index;
		drip->width = PIN(radius * width_scale, 0.003f, 0.035f * thickness_scale);
		drip->start_width = drip->width;
		drip->wander = PIN(wander, -0.45f, 0.45f);
		drip->pause = pause; /* (a moment gathering before it runs) */
		drip->live_decal = NONE;
		drip->live_bead = NONE;
		drip->head_before = drip->head = start;
		drip->live_normal = *normal;
		drip->live_half_width = 0.f;
		drip->length = PIN(radius * length_scale, 0.02f, 0.6f) *
			PIN(blood_parameters.drip_length, 0.f, 5.f);
		drip->remaining = drip->length;
		drip->age = 0;
		/* (metres a second, slowing as it runs) */
		drip->speed = speed / (BLOOD_METRES * 30.f);
		/* (slowing so it comes to rest about its length down: a run of speed /
		(1 - slowing)) */
		drip->slowing = PIN(1.f - drip->speed / MAX(drip->length, 1e-4f), 0.85f, 0.99f);
		drip->position = start;
		drip->normal = *normal;
		drip->laid = drip->position;
	}
}

void blood_decal_landed(
	long decal_definition_index,
	real_point3d const *point,
	real_vector3d const *normal,
	real radius,
	short cluster_index)
{
	if (blood_laying || blood_parameters.drips < 0.5f || !blood_violence_allowed() ||
		fabs(normal->k) >= BLOOD_STEEP || radius <= 0.f)
	{
		return;
	}
	if (decal_definition_index != blood_checked_definition)
	{
		blood_checked_definition = decal_definition_index;
		blood_checked_is_blood = blood_name_has(decal_definition_index, "blood");
	}
	if (!blood_checked_is_blood)
		return;
	blood_drip_start(decal_definition_index, point, normal, radius, cluster_index);
}

/* a drip's step: it runs on down its surface, laying a streak behind it,
until it has run its length, slowed to nothing or come to the surface's edge */
/* what is left of a run, pooled where its surface turned level */
static void blood_drip_pool(struct blood_drip *drip, real_point3d const *point, real_vector3d const *normal)
{
	real share = PIN(drip->remaining / MAX(drip->length, 1e-4f), 0.f, 1.f);
	real radius = drip->start_width * (1.2f + 2.5f * share);
	real definition_radius = decal_definition_radius(drip->decal_definition_index);

	if (share < 0.1f || radius <= 0.f || definition_radius <= 0.f)
		return;
	blood_laying = TRUE;
	decal_port_full_intensity = TRUE;
	{
		real_point3d above = *point;
		real_vector3d into;

		blood_add(&above, normal, 0.05f);
		scale_vector3d(normal, -0.15f, &into);
		decal_new(drip->decal_definition_index, &above, &into, radius / definition_radius, FALSE, NONE, NULL);
	}
	decal_port_full_intensity = FALSE;
	blood_laying = FALSE;
}

/* a branch off a run: from where it is, with some of what is left of it,
wandering off its own way */
static void blood_drip_branch(struct blood_drip *drip, real_point3d const *point, real_vector3d const *normal)
{
	struct blood_drip *branch = NULL;
	short index;

	for (index = 0; index < BLOOD_MAXIMUM_DRIPS; index++)
	{
		if (!blood_drips[index].active)
		{
			branch = &blood_drips[index];
			break;
		}
	}
	if (!branch)
		return;
	*branch = *drip;
	branch->remaining = drip->remaining * 0.5f;
	branch->length = drip->length * 0.6f;
	branch->width = drip->width * 0.7f;
	branch->start_width = branch->width;
	branch->wander = drip->wander + (local_random_range(0, 2) ? 0.35f : -0.35f);
	branch->pause = 0;
	branch->live_decal = NONE;
	branch->live_bead = NONE;
	drip->remaining *= 0.7f;
	/* (where it is now: the branch's run starts there) */
	branch->position = *point;
	branch->normal = *normal;
	branch->laid = branch->position;
}

static void blood_drip_update(struct blood_drip *drip)
{
	real_point3d world;
	real_vector3d normal;
	real_vector3d down;
	real_point3d next;
	real_point3d origin;
	real_vector3d ray;
	boolean on_surface;
	real laid_distance;
	real_point3d laid_world;
	real_point3d hit_point;
	real_vector3d hit_normal;

	world = drip->position;
	normal = drip->normal;
	laid_world = drip->laid;
	if (!blood_downhill(&normal, &down) || fabs(normal.k) >= BLOOD_STEEP + 0.15f)
	{
		/* (its surface turned level: what is left of it pools there) */
		if (normal.k < -0.3f)
			blood_drop_fall(drip->decal_definition_index, &world, drip->width * 0.7f); /* (an underside: it drops) */
		else
			blood_drip_pool(drip, &world, &normal);
		drip->active = FALSE;
		return;
	}
	/* (a gathering bead: it holds, then lets go faster) */
	if (drip->pause > 0)
	{
		drip->pause--;
		if (drip->pause == 0)
			drip->speed *= 1.6f;
		/* (its head held where it is, frame to frame) */
		drip->head_before = drip->head;
		return;
	}
	/* (not straight down: wandering a little either way, as a run finds its
	way over a surface) */
	{
		real_vector3d across;

		across.i = normal.j * down.k - normal.k * down.j;
		across.j = normal.k * down.i - normal.i * down.k;
		across.k = normal.i * down.j - normal.j * down.i;
		drip->wander = PIN(drip->wander + real_local_random_range(-0.09f, 0.09f), -0.4f, 0.4f);
		down.i = down.i * (real)cos(drip->wander) + across.i * (real)sin(drip->wander);
		down.j = down.j * (real)cos(drip->wander) + across.j * (real)sin(drip->wander);
		down.k = down.k * (real)cos(drip->wander) + across.k * (real)sin(drip->wander);
	}

	next = world;
	blood_add(&next, &down, drip->speed * real_local_random_range(0.75f, 1.25f));

	/* still on its surface? */
	origin = next;
	blood_add(&origin, &normal, 0.04f);
	scale_vector3d(&normal, -0.1f, &ray);
	{
		struct collision_result collision;

		match_assert("blood.c", __LINE__, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
		global_current_collision_users[global_current_collision_user_depth++] = _collision_user_decals;
		on_surface = collision_test_vector(
			FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_structure_bit),
			&origin, &ray, NONE, &collision) &&
			collision.type == _collision_result_structure && valid_real_plane3d(&collision.plane);
		global_current_collision_user_depth--;
		if (on_surface)
		{
			hit_point = collision.point;
			hit_normal = collision.plane.n;
			drip->cluster_index = collision.location.cluster_index;
		}
	}
	on_surface = on_surface && blood_dot(&hit_normal, &normal) > 0.9f;
	if (on_surface)
	{
		next = hit_point;
		normal = hit_normal;
	}
	else
	{
		/* (over the surface's edge: it stops where it was, at the edge; what
		is left of it gathers there and falls) */
		next = world;
		if (drip->remaining > drip->length * 0.15f)
			blood_drop_fall(drip->decal_definition_index, &world, drip->width * 0.6f);
	}

	/* a streak from where the last ended, once it has run a few times its
	width (few decals: a trail is a handful of long thin quads), or stops */
	laid_distance = distance3d(&laid_world, &next);
	drip->remaining -= drip->speed;
	drip->speed *= drip->slowing;
	drip->age++;
	/* (all but run out: it stops, in a few seconds at most) */
	if (drip->remaining < drip->length * 0.12f || drip->speed < 0.0004f || drip->age > 6 * 30)
		drip->remaining = 0.f;
	/* (now and then it stops to gather, a bead at its head; now and then a
	branch runs off it with some of what is left) */
	if (on_surface && drip->remaining > drip->length * 0.3f && local_random_range(0, 40) == 0)
		drip->pause = (short)local_random_range(4, 16);
	if (on_surface && drip->remaining > drip->length * 0.45f && local_random_range(0, 70) == 0)
		blood_drip_branch(drip, &next, &normal);
	if (drip->cluster_index != NONE)
	{
		/* a live run, its streak from where the last ended to
		its head and a bead at its head, both moved as it runs (and, between
		ticks, frame by frame: blood_render_update); kept and a new one begun
		every so often, or where it stops or gathers */
		boolean stopping = !on_surface || drip->remaining <= 0.f;
		boolean keep = stopping || drip->pause > 0 || laid_distance >= MAX(drip->width * 3.5f, 0.03f);

		drip->head_before = drip->live_decal != NONE || drip->live_bead != NONE ? drip->head : next;
		drip->head = next;
		drip->live_normal = normal;
		drip->live_half_width = drip->width * (0.45f + 0.55f * PIN(drip->remaining / MAX(drip->length, 1e-4f), 0.f, 1.f)) * 0.5f;
		blood_laying = TRUE;
		blood_drip_place(drip, &laid_world, &next, TRUE);
		if (keep)
		{
			/* (its run kept, laid over again for depth) */
			short layers = 2;
			short layer;

			for (layer = 1; layer < layers; layer++)
				blood_drip_quad(drip, &laid_world, &next, FALSE, BLOOD_LIFT * (0.5f + 0.25f * layer));
			if (stopping || drip->pause > 0)
				blood_drip_quad(drip, &next, &next, TRUE, BLOOD_LIFT);
			drip->live_decal = NONE;
			drip->head_before = next;
			if (stopping)
				drip->live_bead = NONE;
			laid_world = next;
		}
		blood_laying = FALSE;
		if (stopping)
		{
			drip->active = FALSE;
			return;
		}
	}
	else if (!on_surface || drip->remaining <= 0.f)
	{
		drip->active = FALSE;
		return;
	}

	drip->position = next;
	drip->normal = normal;
	drip->laid = laid_world;
}

/* ---------- public code */

static void blood_falling_drops_reset(void);


void blood_initialize_for_new_map(
	void)
{
	short index;

	for (index = 0; index < BLOOD_MAXIMUM_KINDS; index++)
		blood_kinds[index].biped_definition_index = NONE;
	for (index = 0; index < BLOOD_MAXIMUM_DRIPS; index++)
		blood_drips[index].active = FALSE;
	blood_last_time = NONE;
	blood_checked_definition = NONE;
	blood_falling_drops_reset();
}

/* ---------- falling drops (purely drawn: blood_drops.c draws them) */

enum
{
	BLOOD_MAXIMUM_FALLING_DROPS = 48,
};

static struct
{
	boolean active;
	long decal_definition_index;
	real_point3d previous;
	real_point3d position;
	real_vector3d velocity; /* world units a tick */
	real radius;
	short age;
} blood_falling_drops[BLOOD_MAXIMUM_FALLING_DROPS];

static void blood_falling_drops_reset(void)
{
	short index;

	for (index = 0; index < BLOOD_MAXIMUM_FALLING_DROPS; index++)
		blood_falling_drops[index].active = FALSE;
}

/* a drop let go from a point (a run over an edge, an underside) */
static void blood_drop_fall(long decal_definition_index, real_point3d const *point, real radius)
{
	short index;

	if (!blood_drops_animated() || decal_definition_index == NONE || radius <= 0.f)
		return;
	for (index = 0; index < BLOOD_MAXIMUM_FALLING_DROPS; index++)
	{
		if (!blood_falling_drops[index].active)
		{
			blood_falling_drops[index].active = TRUE;
			blood_falling_drops[index].decal_definition_index = decal_definition_index;
			blood_falling_drops[index].previous = *point;
			blood_falling_drops[index].position = *point;
			blood_falling_drops[index].velocity.i = 0.f;
			blood_falling_drops[index].velocity.j = 0.f;
			blood_falling_drops[index].velocity.k = 0.f;
			blood_falling_drops[index].radius = PIN(radius, 0.004f, 0.03f);
			/* (a moment hanging, swelling, before it lets go) */
			blood_falling_drops[index].age = (short)-local_random_range(4, 12);
			return;
		}
	}
}

/* (a tick: hanging, then falling; where it lands, a small splat) */
static void blood_drop_update(short index)
{
	real_vector3d step;
	boolean landed = FALSE;

	blood_falling_drops[index].previous = blood_falling_drops[index].position;
	if (blood_falling_drops[index].age++ < 0)
		return;
	/* (gravity: about 9.8 m/s/s, in world units a tick each tick) */
	blood_falling_drops[index].velocity.k -= 9.8f / (BLOOD_METRES * 30.f * 30.f);
	step = blood_falling_drops[index].velocity;
	if (blood_falling_drops[index].age > 90)
	{
		blood_falling_drops[index].active = FALSE;
		return;
	}
	/* (on the level) */
	{
		struct collision_result collision;

		match_assert("blood.c", __LINE__, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
		global_current_collision_users[global_current_collision_user_depth++] = _collision_user_decals;
		if (collision_test_vector(FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_structure_bit),
			&blood_falling_drops[index].position, &step, NONE, &collision) &&
			collision.type == _collision_result_structure)
		{
			real_point3d above = collision.point;
			real_vector3d into;
			real definition_radius = decal_definition_radius(blood_falling_drops[index].decal_definition_index);

			landed = TRUE;
			blood_add(&above, &collision.plane.n, 0.05f);
			scale_vector3d(&collision.plane.n, -0.15f, &into);
			global_current_collision_user_depth--;
			if (definition_radius > 0.f)
			{
				blood_laying = TRUE;
				decal_new(blood_falling_drops[index].decal_definition_index, &above, &into,
					blood_falling_drops[index].radius * 1.6f / definition_radius, FALSE, NONE, NULL);
				blood_laying = FALSE;
			}
		}
		else
		{
			global_current_collision_user_depth--;
		}
	}
	if (landed)
	{
		blood_falling_drops[index].active = FALSE;
		return;
	}
	blood_add(&blood_falling_drops[index].position, &step, 1.f);
}

short blood_drops_for_drawing(
	real_point3d *positions,
	real_vector3d *directions,
	real *radii,
	long *decals,
	short maximum)
{
	real fraction = PIN(render_interpolation_fraction(), 0.f, 1.f);
	short count = 0;
	short index;

	if (!blood_drops_animated() || !blood_violence_allowed())
		return 0;
	/* (a run's head is led by its own bead, a decal like the run's, lit and
	coloured as it is: blood_drip_place; only drops in the air are drawn
	here) */
	/* (the falling ones: hanging, they swell; falling, they stretch along
	their way) */
	for (index = 0; index < BLOOD_MAXIMUM_FALLING_DROPS && count < maximum; index++)
	{
		if (!blood_falling_drops[index].active)
			continue;
		positions[count].x = blood_falling_drops[index].previous.x +
			(blood_falling_drops[index].position.x - blood_falling_drops[index].previous.x) * fraction;
		positions[count].y = blood_falling_drops[index].previous.y +
			(blood_falling_drops[index].position.y - blood_falling_drops[index].previous.y) * fraction;
		positions[count].z = blood_falling_drops[index].previous.z +
			(blood_falling_drops[index].position.z - blood_falling_drops[index].previous.z) * fraction;
		directions[count] = blood_falling_drops[index].velocity;
		radii[count] = blood_falling_drops[index].radius *
			(blood_falling_drops[index].age < 0 ? 0.8f + 0.04f * (12 + blood_falling_drops[index].age) : 1.f);
		decals[count] = blood_falling_drops[index].decal_definition_index;
		count++;
	}
	return count;
}

void blood_update(
	void)
{
	long now = game_time_get();
	long steps;
	short index;

	/* (a revert, a saved game loaded: the drips stop) */
	if (blood_last_time != NONE && now < blood_last_time)
	{
		for (index = 0; index < BLOOD_MAXIMUM_DRIPS; index++)
			blood_drips[index].active = FALSE;
		for (index = 0; index < BLOOD_MAXIMUM_FALLING_DROPS; index++)
			blood_falling_drops[index].active = FALSE;
		blood_last_time = now - 1;
	}
	/* (each tick once: decals_update runs every frame drawn, several a tick) */
	steps = blood_last_time == NONE ? 1 : now - blood_last_time;
	if (steps <= 0)
		return;
	blood_last_time = now;
	if (!blood_violence_allowed())
		return;
	for (steps = MIN(steps, 4); steps > 0; steps--)
	{
		for (index = 0; index < BLOOD_MAXIMUM_DRIPS; index++)
		{
			if (blood_drips[index].active)
				blood_drip_update(&blood_drips[index]);
		}
	}
	for (index = 0; index < BLOOD_MAXIMUM_FALLING_DROPS; index++)
	{
		if (blood_falling_drops[index].active)
			blood_drop_update(index);
	}
}

boolean blood_decal_on_two_sided(
	long decal_definition_index)
{
	return blood_violence_allowed() && decal_definition_index != NONE && blood_name_has(decal_definition_index, "blood");
}

void blood_biped_damaged(
	long biped_index,
	struct damage_data const *damage,
	real damage_amount)
{
	struct blood_kind const *kind;
	real_vector3d through;

	/* most hits on the body throw blood onto the wall or slope behind, which
	runs: the game's own throws only a little, often nowhere */
	if (!blood_violence_allowed() || !(damage_amount > 0.f) || blood_parameters.amount <= 0.f ||
		!valid_real(damage->epicenter.x) || object_get(biped_index)->object.shield_vitality > 0.f ||
		real_local_random_range(0.f, 1.f) >= 0.85f)
	{
		return;
	}
	through = damage->direction;
	kind = blood_kind_get(biped_index);
	if (kind && kind->splat != NONE && blood_normalize(&through) > 0.f)
	{
		real_point3d from = damage->epicenter;
		real_vector3d reach;

		blood_add(&from, &through, 0.12f);
		scale_vector3d(&through, 1.8f, &reach);
		decal_new(kind->splat, &from, &reach, real_local_random_range(0.7f, 1.2f), FALSE, NONE, NULL);
	}
}

#endif // BLOOD_EFFECTS
