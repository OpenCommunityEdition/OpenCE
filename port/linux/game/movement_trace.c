/* movement_trace.c: the reference traces of the native engine's movement
(conductor/tracks/native_engine_20261021, engine/tests/traces).

debug.movement_trace names a script of inputs that the first local player
plays in place of the keyboard and mouse, tick by tick, once their biped has
stood for a second at the trace's start (bloodgulch's red base's side,
facing the slope down from it; the same for every script, so a trace
depends on its inputs only); each tick the biped's state is logged as a line that
starts "movement trace:" (tools/engine_movement_traces.py reads them):

	movement trace: <script> <tick> <x> <y> <z> <vx> <vy> <vz> <yaw> <airborne>
		<airborne ticks> <crouch> <landing> <body> <shield>

with the position in world units, the velocity in world units a tick, the
facing in radians, the airborne flag (0 or 1), the crouch (0 to 1), the
biped's landing (0 none, 1 soft, 2 hard) and its body and shield
vitalities. "movement trace: <script> done" ends it. The scripts:

	run     forward for three seconds, then let go for a second
	strafe  left for two seconds, then let go for a second
	jump    a jump half a second in, standing, then two seconds
	runjump forward, jumping a second in, for three seconds
	crouch  crouching, forward for three seconds, then let go
	fall    lifted 3 world units at the start (6 is a deadly fall), then three
	        seconds
	ramp    forward and turning slowly left for six seconds (the ground's
	        slopes: climbing, stepping up and sliding)

Usually with debug.network_test "host:<map>" (a game with no menus). */

#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/bipeds.h"
#include "real_math.h"

#include "movement_trace.h"

#include <string.h>

/* the platform layer's (port/linux/src/port_config.c) */
const char *config_string(char const *name);
void platform_log(char const *format, ...);

enum
{
	/* (a second standing still first: the spawn's settling) */
	MOVEMENT_TRACE_SETTLE_TICKS = TICKS_PER_SECOND,
};

/* where every script starts (dropped a little, to settle on the ground) */
static real_point3d const movement_trace_start = { 87.1f, -158.45f, 0.4f };
static real const movement_trace_start_yaw = -0.35657f;

struct movement_trace_script
{
	char const *name;
	long ticks;
};

static struct movement_trace_script const movement_trace_scripts[] =
{
	{ "run", 4 * TICKS_PER_SECOND },
	{ "strafe", 3 * TICKS_PER_SECOND },
	{ "jump", 5 * TICKS_PER_SECOND / 2 },
	{ "runjump", 3 * TICKS_PER_SECOND },
	{ "crouch", 4 * TICKS_PER_SECOND },
	{ "fall", 3 * TICKS_PER_SECOND },
	{ "ramp", 6 * TICKS_PER_SECOND },
};

static struct
{
	boolean read;
	struct movement_trace_script const *script;
	long unit_index;
	long first_tick;
	long last_logged_tick;
	boolean done;
} movement_trace = { FALSE, NULL, NONE, NONE, NONE, FALSE };

static struct movement_trace_script const *movement_trace_script(
	void)
{
	if (!movement_trace.read)
	{
		char const *name = config_string("debug.movement_trace");
		long index;

		movement_trace.read = TRUE;
		for (index = 0; name && index < (long)NUMBEROF(movement_trace_scripts); index++)
		{
			if (strcmp(name, movement_trace_scripts[index].name) == 0)
				movement_trace.script = &movement_trace_scripts[index];
		}
		if (name && *name && !movement_trace.script)
			platform_log("movement trace: no script %s", name);
	}
	return movement_trace.script;
}

/* the ticks since the script began (negative while the biped settles), or
NONE before its biped (or with no script) */
static long movement_trace_ticks(
	void)
{
	if (!movement_trace_script() || movement_trace.first_tick == NONE)
		return NONE;
	return game_time_get() - movement_trace.first_tick;
}

static boolean movement_trace_is(
	char const *name)
{
	return strcmp(movement_trace.script->name, name) == 0;
}

void movement_trace_input(
	short local_player_index,
	real_vector2d *throttle,
	real_euler_angles2d *facing_delta,
	unsigned long *unit_control_flags)
{
	long ticks = movement_trace_ticks();

	if (local_player_index != 0 || !movement_trace_script() || movement_trace.done)
		return;
	throttle->i = 0.f;
	throttle->j = 0.f;
	facing_delta->yaw = 0.f;
	facing_delta->pitch = 0.f;
	SET_FLAG(*unit_control_flags, _unit_control_jump_bit, FALSE);
	SET_FLAG(*unit_control_flags, _unit_control_crouch_modifier_bit, FALSE);
	/* (standing still while the biped settles) */
	if (ticks == NONE || ticks < 0)
		return;
	if (movement_trace_is("run") && ticks < 3 * TICKS_PER_SECOND)
		throttle->i = 1.f;
	else if (movement_trace_is("strafe") && ticks < 2 * TICKS_PER_SECOND)
		throttle->j = 1.f;
	else if (movement_trace_is("jump") && ticks == TICKS_PER_SECOND / 2)
		SET_FLAG(*unit_control_flags, _unit_control_jump_bit, TRUE);
	else if (movement_trace_is("runjump"))
	{
		throttle->i = 1.f;
		SET_FLAG(*unit_control_flags, _unit_control_jump_bit, ticks == TICKS_PER_SECOND);
	}
	else if (movement_trace_is("crouch"))
	{
		SET_FLAG(*unit_control_flags, _unit_control_crouch_modifier_bit, TRUE);
		throttle->i = ticks < 3 * TICKS_PER_SECOND ? 1.f : 0.f;
	}
	else if (movement_trace_is("ramp"))
	{
		throttle->i = 1.f;
		/* (a quarter turn over the six seconds) */
		facing_delta->yaw = DEGREES_TO_RADIANS(90) / (6 * TICKS_PER_SECOND);
	}
}

void movement_trace_update(
	void)
{
	long player_index;
	struct player_datum *player;
	struct biped_datum *biped;
	long tick = game_time_get();
	long ticks;

	if (!movement_trace_script() || movement_trace.done || tick == movement_trace.last_logged_tick)
		return;
	player_index = local_player_get_player_index(0);
	if (player_index == NONE)
		return;
	player = player_get(player_index);
	biped = player->unit_index != NONE ? biped_try_and_get(player->unit_index) : NULL;
	if (!biped || (player->unit_index != movement_trace.unit_index && movement_trace_ticks() >= 0))
	{
		/* (the script's biped died: the trace ends there) */
		if (movement_trace_ticks() >= 0)
		{
			movement_trace.done = TRUE;
			platform_log("movement trace: %s died", movement_trace.script->name);
		}
		return;
	}
	if (player->unit_index != movement_trace.unit_index)
	{
		/* (a new biped: the script waits for it to settle) */
		real_vector3d forward;

		movement_trace.unit_index = player->unit_index;
		movement_trace.first_tick = tick + MOVEMENT_TRACE_SETTLE_TICKS;
		forward.i = (real)cos(movement_trace_start_yaw);
		forward.j = (real)sin(movement_trace_start_yaw);
		forward.k = 0.f;
		object_set_position(player->unit_index, &movement_trace_start, &forward, NULL);
		player_control_set_facing(0, &forward);
		return;
	}
	ticks = movement_trace_ticks();
	if (ticks < 0)
		return;
	if (ticks == 0 && movement_trace_is("fall"))
	{
		real_point3d position = biped->object.position;

		position.z += 3.f;
		object_set_position(player->unit_index, &position, NULL, NULL);
	}
	movement_trace.last_logged_tick = tick;
	platform_log("movement trace: %s %ld %.5f %.5f %.5f %.6f %.6f %.6f %.5f %d %d %.4f %d %.4f %.4f",
		movement_trace.script->name, ticks,
		biped->object.position.x, biped->object.position.y, biped->object.position.z,
		biped->object.translational_velocity.i, biped->object.translational_velocity.j,
		biped->object.translational_velocity.k,
		(real)atan2(biped->object.forward.j, biped->object.forward.i),
		TEST_FLAG(biped->biped.flags, _biped_airborne_bit) ? 1 : 0, (int)biped->biped.airborne_ticks,
		biped->biped.crouch, (int)biped->biped.landing,
		biped->object.body_vitality, biped->object.shield_vitality);
	if (ticks >= movement_trace.script->ticks)
	{
		movement_trace.done = TRUE;
		platform_log("movement trace: %s done", movement_trace.script->name);
	}
}
