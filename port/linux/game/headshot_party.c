/*
HEADSHOT_PARTY.C

A grunt that a player kills with a shot to the head (display.headshot_party,
off by default: the stock game has none) pops with confetti, and a sound
plays. The confetti is drawn on the screen, in 2D: pieces thrown up from
where the grunt's head was, falling with a little gravity, fading out. The
sound is party.wav in the data folder (a WAV file; no file, no sound).

Only the machine that runs the AI knows a grunt (the host of a co-op game,
or a single player), so only it sees the party.
*/

#include "cseries.h"
#include "ai/actors.h"
#include "ai/actor_definitions.h"
#include "ai/actor_types.h"
#include "cutscene/cinematics.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "render/render.h"
#include "render/render_camera_projection.h"
#include "units/units.h"

#include "headshot_party.h"
#include "../src/effect_audio.h"

#include <math.h>
#include <stdio.h>

/* ---------- constants */

enum
{
	PARTY_BURSTS = 6,
	PARTY_PIECES = 70,
	/* how long the confetti lasts, and how long it fades at the end
	(milliseconds) */
	PARTY_LIFE_MILLISECONDS = 2400,
	PARTY_FADE_MILLISECONDS = 600,
};

/* the pieces' speeds and fall in the HUD's 480 lines a second */
#define PARTY_MINIMUM_SPEED 120.0f
#define PARTY_MAXIMUM_SPEED 420.0f
#define PARTY_GRAVITY 520.0f
#define PARTY_GAIN 0.8f

/* ---------- structures */

struct party_burst
{
	boolean used;
	boolean placed;
	unsigned long start;
	unsigned long seed;
	real_point3d world;
	real x, y;
};

/* ---------- prototypes */

/* port_config.c's, cinematics.c's / cseries_windows.c's, xbox_files.c's,
rasterizer's */
int config_boolean(const char *name);
unsigned long config_changes(void);
unsigned long system_milliseconds(void);
const char *platform_data_root(void);
void draw_quad(rectangle2d *rectangle, pixel32 color);

/* ---------- globals */

static struct party_burst party_bursts[PARTY_BURSTS];

static const unsigned long party_colors[] =
{
	0xFFFF4D4D, 0xFFFFB347, 0xFFFFE94D, 0xFF5CE65C, 0xFF4DC3FF, 0xFFB57BFF, 0xFFFF7BD5,
};

/* ---------- private code */

static boolean party_enabled(
	void)
{
	static unsigned long changes = (unsigned long)-1;
	static boolean enabled = FALSE;

	if (changes != config_changes())
	{
		changes = config_changes();
		enabled = config_boolean("display.headshot_party") != 0;
	}
	return enabled && game_in_progress() && !game_engine_running();
}

static boolean party_unit_is_grunt(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	long actor_index = unit->unit.swarm_actor_index != NONE ? unit->unit.swarm_actor_index : unit->unit.actor_index;
	struct actor_variant_definition *variant;
	long variant_index;

	/* (the AI is the host's: a client's units have no actors) */
	if (actor_index == NONE)
		return FALSE;
	variant_index = actor_get(actor_index)->meta.variant_definition_index;
	if (variant_index == NONE)
		return FALSE;
	variant = actor_variant_definition_get(variant_index);
	return variant->actor_reference.index != NONE &&
		actor_definition_get(variant->actor_reference.index)->type == _actor_grunt;
}

/* a random number from 0 to 1, from a seed that it moves on */
static real party_random(
	unsigned long *seed)
{
	*seed = *seed * 1664525UL + 1013904223UL;
	return (real)((*seed >> 8) & 0xFFFF) / 65535.0f;
}

/* ---------- public code */

void headshot_party_kill(
	long unit_index,
	long killing_player_index)
{
	struct party_burst *burst = NULL;
	unsigned long now;
	char path[512];
	long index;

	if (killing_player_index == NONE || !party_enabled())
		return;
	if (!player_try_and_get(killing_player_index) || !party_unit_is_grunt(unit_index))
		return;
	now = system_milliseconds();
	/* (a free place, else the oldest) */
	for (index = 0; index < PARTY_BURSTS; index++)
	{
		if (!party_bursts[index].used)
		{
			burst = &party_bursts[index];
			break;
		}
		if (!burst || party_bursts[index].start < burst->start)
			burst = &party_bursts[index];
	}
	burst->used = TRUE;
	burst->placed = FALSE;
	burst->start = now;
	burst->seed = now ^ (unsigned long)(unit_index * 2654435761UL);
	unit_get_head_position(unit_index, &burst->world);
	snprintf(path, sizeof(path), "%s/party.wav", platform_data_root());
	effect_audio_play_file(path, PARTY_GAIN);

	return;
}

void headshot_party_project(void)
{
	long index;

	if (!party_enabled() || render.local_player_index != local_player_get_next(NONE))
		return;
	for (index = 0; index < PARTY_BURSTS; index++)
	{
		struct party_burst *burst = &party_bursts[index];
		real_point3d view_position;
		real_point2d screen_position;

		if (!burst->used)
			continue;
		matrix4x3_transform_point(&render.frustum.world_to_view, &burst->world, &view_position);
		if (render_camera_view_to_screen(&render.camera, &render.frustum, &view_position, &screen_position))
		{
			/* (the view's own coordinates, as the HUD's) */
			burst->x = screen_position.x - (real)render.camera.viewport_bounds.x0;
			burst->y = screen_position.y - (real)render.camera.viewport_bounds.y0;
			burst->placed = TRUE;
		}
	}

	return;
}

void headshot_party_draw(void)
{
	unsigned long now = system_milliseconds();
	rectangle2d window;
	real scale;
	long index;

	if (cinematic_in_progress())
		return;
	window = render.camera.window_bounds;
	offset_rectangle2d(&window, (short)-render.camera.viewport_bounds.x0, (short)-render.camera.viewport_bounds.y0);
	scale = (real)(window.y1 - window.y0) / 480.0f;
	for (index = 0; index < PARTY_BURSTS; index++)
	{
		struct party_burst *burst = &party_bursts[index];
		unsigned long age = now - burst->start;
		unsigned long seed;
		real seconds, fade;
		long piece;

		if (!burst->used)
			continue;
		if (age >= PARTY_LIFE_MILLISECONDS)
		{
			burst->used = FALSE;
			continue;
		}
		if (!burst->placed)
			continue;
		seconds = (real)age / 1000.0f;
		fade = age + PARTY_FADE_MILLISECONDS > PARTY_LIFE_MILLISECONDS ?
			(real)(PARTY_LIFE_MILLISECONDS - age) / (real)PARTY_FADE_MILLISECONDS : 1.0f;
		seed = burst->seed;
		for (piece = 0; piece < PARTY_PIECES; piece++)
		{
			/* (thrown up and out in a fan, some slower) */
			real angle = -3.14159265f * (0.08f + 0.84f * party_random(&seed));
			real speed = PARTY_MINIMUM_SPEED + (PARTY_MAXIMUM_SPEED - PARTY_MINIMUM_SPEED) * party_random(&seed);
			unsigned long color = party_colors[(unsigned long)(party_random(&seed) * 6.99f)];
			real spin = 6.0f + 10.0f * party_random(&seed);
			real phase = 6.2831853f * party_random(&seed);
			real flutter = 0.35f + 0.65f * (real)fabs(cos(seconds * spin + phase));
			real x = burst->x + (real)cos(angle) * speed * seconds * scale * (1.0f - 0.25f * seconds);
			real y = burst->y + ((real)sin(angle) * speed * seconds + 0.5f * PARTY_GRAVITY * seconds * seconds) * scale;
			real width = MAX(5.0f * scale * flutter, 2.0f);
			real height = MAX(3.0f * scale, 2.0f);
			rectangle2d quad;
			pixel32 pixel = ((pixel32)(fade * 255.0f) << 24) | (pixel32)(color & 0x00FFFFFF);

			quad.x0 = (short)(x - width * 0.5f);
			quad.x1 = (short)(x + width * 0.5f);
			quad.y0 = (short)(y - height * 0.5f);
			quad.y1 = (short)(y + height * 0.5f);
			if (quad.x1 <= quad.x0)
				quad.x1 = (short)(quad.x0 + 1);
			if (quad.y1 <= quad.y0)
				quad.y1 = (short)(quad.y0 + 1);
			draw_quad(&quad, pixel);
		}
	}

	return;
}
