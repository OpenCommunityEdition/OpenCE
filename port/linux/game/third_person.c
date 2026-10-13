/*
THIRD_PERSON.C

A third-person camera on foot, over the right shoulder, as many games have
it (Helldivers 2 among them): controls.third_person (Numpad Enter) turns it
on and off for the first local player, in a campaign (co-op too), never in
multiplayer.

- The camera sits behind the player's head, a little to the right and up,
  looking where the player looks: the mouse turns the player as in first
  person. The wheel brings it nearer or further (THIRD_PERSON_NEAREST to
  THIRD_PERSON_FURTHEST), and switches no weapon meanwhile. The level and
  its objects push it in, as the game's own vehicle cameras are
  (observer.c's penetration check).
- Alt held looks freely: the player keeps the facing and aim it had (moving
  on that way), the mouse turns the camera alone all the way round, and the
  player's head turns to where the camera looks (over the shoulder, when it
  looks back). Let go, and the player turns to the camera's way.
- On foot only: in a vehicle its own cameras, dead the game's dead camera, in
  a cutscene the cutscene's. The body is drawn and the first-person weapon
  is not (the director takes it as a third-person view: director.c).

Only the first local player's view and controls change; nothing is sent.
*/

#include "cseries.h"
#include "real_math.h"
#include "camera/observer.h"
#include "cutscene/cinematics.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/unit_control_data.h"

/* xinput_sdl.c's, port_config.c's */
void platform_third_person_active(int active);
float platform_third_person_wheel(void);
int platform_third_person_key(void);
int platform_free_look_held(void);

/* ---------- constants */

/* the camera behind the head, world units (a world unit about 3 m): nearest,
furthest and at first (about 0.75 m, 3.7 m and 1.7 m), and a notch of the
wheel */
#define THIRD_PERSON_NEAREST 0.25f
#define THIRD_PERSON_FURTHEST 1.2f
#define THIRD_PERSON_START 0.55f
#define THIRD_PERSON_NOTCH 0.08f
/* the camera over the right shoulder and a little up (about 40 cm, 15 cm) */
#define THIRD_PERSON_RIGHT 0.13f
#define THIRD_PERSON_UP 0.05f
/* the wheel's turn a notch is (as xinput_sdl.c's) */
#define THIRD_PERSON_WHEEL_NOTCH 1.0f

/* ---------- globals */

static boolean third_person_on = FALSE;
static real third_person_distance = THIRD_PERSON_START;
/* free look: the player's facing held while it lasts */
static boolean third_person_free_look = FALSE;
static boolean third_person_frozen = FALSE;
static real_euler_angles2d third_person_frozen_facing;

/* ---------- private code */

/* the first local player's unit, alive and on foot (NONE: none) */
static long third_person_unit(
	void)
{
	long player_index = local_player_get_player_index(0);
	struct player_datum *player = player_index != NONE ? player_try_and_get(player_index) : NULL;
	struct unit_datum *unit;

	if (!player || player->unit_index == NONE)
		return NONE;
	unit = unit_try_and_get(player->unit_index);
	if (!unit || unit->object.parent_object_index != NONE ||
		TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
	{
		return NONE;
	}
	return player->unit_index;
}

/* ---------- public code */

/* (director.c, each frame) whether the local player's view is the
third-person camera now, its key read */
boolean third_person_camera_wanted(
	short local_player_index)
{
	boolean wanted;

	if (local_player_index != 0)
		return FALSE;
	if (platform_third_person_key())
		third_person_on = !third_person_on;
	/* (a campaign's, co-op's too: never in multiplayer, where a game
	engine runs) */
	wanted = third_person_on && !game_engine_running() && !cinematic_in_progress() && third_person_unit() != NONE;
	/* (zoomed in, a scope: first person at once, as the scope is the
	player's own view; back out again as it zooms out) */
	if (wanted)
	{
		struct unit_datum *unit = unit_try_and_get(third_person_unit());

		if (unit && unit->unit.current_zoom_level != NONE)
			wanted = FALSE;
	}
	platform_third_person_active(wanted);
	third_person_free_look = wanted && platform_free_look_held();
	if (!third_person_free_look)
		third_person_frozen = FALSE;
	return wanted;
}

/* (director.c) the camera: a director_camera_update_proc */
void third_person_camera_update(
	void *camera,
	void *controls,
	void *result)
{
	struct observer_command *command = (struct observer_command *)result;
	short local_player_index = *(short const *)controls;
	long unit_index = third_person_unit();
	real wheel = platform_third_person_wheel();
	real_vector3d forward;

	(void)camera;
	if (unit_index == NONE)
		return;
	/* (a wheel turned forward: nearer) */
	third_person_distance -= wheel / THIRD_PERSON_WHEEL_NOTCH * THIRD_PERSON_NOTCH;
	third_person_distance = PIN(third_person_distance, THIRD_PERSON_NEAREST, THIRD_PERSON_FURTHEST);

	player_control_get_facing_direction(local_player_index, &forward);
	if (normalize3d(&forward) <= 0.f)
		return;
	csmemset(command, 0, sizeof(*command));
	command->flags = FLAG(_observer_command_valid_bit);
	unit_get_camera_position(unit_index, &command->focus_position);
	/* (forward, right, up: observer_update's frame, by the heading) */
	command->focus_offset.i = 0.f;
	command->focus_offset.j = THIRD_PERSON_RIGHT;
	command->focus_offset.k = THIRD_PERSON_UP;
	command->focus_distance = third_person_distance;
	command->field_of_view = player_control_get_field_of_view(local_player_index);
	command->forward = forward;
	observer_up_from_forward(&command->forward, &command->up);
	object_get_velocities(unit_index, &command->focus_velocity, NULL);
}

/* (players.c) the local player's control, as it goes to its unit: in free
look, the facing and aim held, and the head turned to the camera's way */
void third_person_control(
	long player_index,
	real_euler_angles2d const *desired_facing,
	struct unit_control_data *control_data)
{
	struct player_datum *player = player_try_and_get(player_index);
	real_vector3d held;

	if (!player || player->local_player_index != 0 || !third_person_free_look)
		return;
	if (!third_person_frozen)
	{
		third_person_frozen = TRUE;
		third_person_frozen_facing = *desired_facing;
	}
	player_aiming_vector_from_facing(player_index, &held, &third_person_frozen_facing);
	/* (the head: where the camera looks, the mouse's facing) */
	control_data->looking_vector = control_data->aiming_vector;
	control_data->facing_vector = held;
	control_data->aiming_vector = held;
}
