/*
MUTATOR_SCRIPTS.C

A mutator's native HS program, with bounded startup/frame/tick/stop calls.
Frame reads local controls and draws; tick predicts player motion and only the
host moves other objects. Private programs have no file or engine-script access.
*/

#include "cseries.h"
#include "errors.h"
#include "camera/director.h"
#include "cutscene/cinematics.h"
#include "game/players.h"
#include "game/game.h"
#include "memory/data.h"
#include "models/model_definitions.h"
#include "objects/object_definitions.h"
#include "objects/objects.h"
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "rasterizer/rasterizer.h"
#include "tag_files/tag_files.h"
#include "units/bipeds.h"
#include "units/units.h"
#include "halo_keyboard.h"
#include "mutator_scripts.h"
#include "mutator_share.h"


#include "mutator_hs.h"
#include "hs/hs.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef calloc
#undef realloc
#undef free

/* ---------- constants */

enum
{
	MAXIMUM_MUTATOR_SCRIPTS = 8,
	SCRIPT_KEY_LENGTH = 32,
	/* a script's memory, and the instructions of a call */
	MAXIMUM_SCRIPT_INSTRUCTIONS = 65536,
	MAXIMUM_SCRIPT_BUTTONS = 8,
	MAXIMUM_SCRIPT_PLAYERS = 128,
	MAXIMUM_SCRIPT_LINES = 512,
	MAXIMUM_SCRIPT_MODELS = 256,
	/* the controls a frame's scripts claim, and a control's name's most bytes */
	MAXIMUM_SCRIPT_CLAIMS = 8,
	SCRIPT_CLAIM_LENGTH = 32,
};

/* how fast a script can send an object, a tick on each axis (world units) */
#define MAXIMUM_SCRIPT_VELOCITY 10.0f
/* how far a ray goes (world units) */
#define MAXIMUM_SCRIPT_RAY 1000.0f
/* the longest script (mutator_hs_program_compile's own limit, bytes) */
#define MAXIMUM_SCRIPT_SOURCE 32767

/* ---------- structures */

struct mutator_script
{
	char key[SCRIPT_KEY_LENGTH];
	struct mutator_hs_program *state;
	struct mutator_hs_result result;
	long output[4];
	char setting_strings[64][256];
	short setting_string_count;
	/* Bounded, per-script/map diagnostics: repeated frame attempts stay quiet. */
	char model_failure_tags[8][256];
	unsigned long model_failure_reasons[8];
	short model_failure_count;
	boolean kept;
	boolean failed;
	long current_player;
	struct {
		boolean used;
		long datum;
		long integers[32];
		real reals[32];
	} players[MAXIMUM_SCRIPT_PLAYERS];
};

struct mutator_script_line
{
	/* from the camera: its right, down and forward, as it is drawn */
	boolean from_view;
	real from[3];
	real_point3d to;
	real_argb_color color;
};

/* an object a script made (model.create), on this machine alone */
struct mutator_script_model
{
	boolean used;
	short script_index;
	long object_index;
	char tag[256];
	unsigned long pose_logged;
	/* this frame: a node by the camera as it is drawn (NONE for none), and
	whether its nodes face the camera */
	short view_node;
	real view_from[3];
	boolean face_view;
};

/* ---------- globals */

/* the objects scripts have made (mutator_scripts.h) */
short mutator_scripts_model_count;

static struct
{
	struct mutator_script scripts[MAXIMUM_MUTATOR_SCRIPTS];
	struct mutator_script_line lines[MAXIMUM_SCRIPT_LINES];
	short line_count;
	struct mutator_script_model models[MAXIMUM_SCRIPT_MODELS];
	boolean creating_model;
	byte local_buttons;
	byte player_buttons[MAXIMUM_SCRIPT_PLAYERS];
	short local_player_index;
	/* the scripts running (started, kept, not failed): with none, the game's
	hooks return at once */
	short running;
	boolean ticking;
	/* the controls scripts claimed this frame (input.claim), from the game's
	own uses of them */
	char claims[MAXIMUM_SCRIPT_CLAIMS][SCRIPT_CLAIM_LENGTH];
	short claim_count;
	boolean view_active;
	boolean view_backward;
	boolean view_crosshair;
	real view_distance;
	real view_height;
} mutator_scripts;

/* ---------- prototypes */

boolean network_game_distributed_client(void);
void distributed_note_script_velocity(short player_index, long unit_index, real_vector3d const *velocity);
void rasterizer_debug_line(real_point3d const *p0, real_point3d const *p1, real_argb_color const *color);

/* ---------- prototypes/PORT_MUTATORS.C */

char *port_mutators_script_read(char const *key, unsigned long *size);
void port_mutators_script_free(char *text);
typedef void (*port_mutators_setting_proc)(void *context, char const *name, char const *text, double number,
	int boolean_value, int kind);
void port_mutators_script_settings(char const *key, port_mutators_setting_proc proc, void *context);

/* ---------- private code */

/* Scalar marshaling for the typed host callbacks. Outputs of vector/ray calls
 * remain in the owning program until its next such call. */
struct script_arguments
{
	struct mutator_script *script;
	long arguments[10];
	long output[4];
	short count;
	short outputs;
	boolean invalid;
};
static real argument_real(struct script_arguments *state, int argument)
{
	real value;
	long bits = state->arguments[argument - 1];
	memcpy(&value, &bits, sizeof(value));
	return value;
}
static long argument_integer(struct script_arguments *state, int argument)
{
	return state->arguments[argument - 1];
}
static char const *argument_string(struct script_arguments *state, int argument)
{
	return (char const *)state->arguments[argument - 1];
}
static int argument_missing(struct script_arguments *state, int argument)
{
	return argument > state->count || state->arguments[argument - 1] == NONE;
}
static void output_integer(struct script_arguments *state, long value)
{
	if (state->outputs < 4) state->output[state->outputs++] = value;
}
static void output_real(struct script_arguments *state, real value)
{
	long bits = 0;
	memcpy(&bits, &value, sizeof(value));
	output_integer(state, bits);
}
static void output_string(struct script_arguments *state, char const *value)
{
	output_integer(state, value ? (long)value : (long)"");
}
static void output_none(struct script_arguments *state)
{
	output_integer(state, NONE);
}
#define argument_check(state, test, argument, message) \
	do { if (!(test)) { (state)->invalid = TRUE; return 0; } } while (0)

static int script_input_held(
	struct script_arguments *state)
{
	/* (one name, or two as a Controls Setup row has them: "F5, G") */
	char name[64];
	char const *text = argument_string(state, 1);
	boolean held = FALSE;

	while (*text && !held)
	{
		size_t length;

		while (*text == ' ' || *text == ',')
			text++;
		length = strcspn(text, ",");
		if (length && length < sizeof(name))
		{
			memcpy(name, text, length);
			while (length && name[length - 1] == ' ')
				length--;
			name[length] = 0;
			held = length && halo_input_held_named(name);
		}
		text += strcspn(text, ",");
	}
	output_integer(state, held);
	return 1;
}

static int script_input_claim(
	struct script_arguments *state)
{
	char const *name = argument_string(state, 1);

	if (mutator_scripts.claim_count < MAXIMUM_SCRIPT_CLAIMS && strlen(name) < SCRIPT_CLAIM_LENGTH)
		csstrcpy(mutator_scripts.claims[mutator_scripts.claim_count++], name);
	return 0;
}

/* input.button(n, down): one of the first local player's buttons, which go with
their input to the host and from it to every machine, for player.button */
static int script_input_button(
	struct script_arguments *state)
{
	long number = argument_integer(state, 1);

	argument_check(state, number >= 0 && number < MAXIMUM_SCRIPT_BUTTONS, 1, "no such button");
	SET_FLAG(mutator_scripts.local_buttons, (long)number, argument_integer(state, 2) != 0);
	return 0;
}

static struct unit_datum *script_player_unit(
	void)
{
	long player_index = local_player_get_player_index(mutator_scripts.local_player_index);
	long unit_index = player_control_get_unit_index(mutator_scripts.local_player_index);

	if (player_index == NONE || unit_index == NONE || player_get(player_index)->unit_index != unit_index)
		return NULL;
	return unit_try_and_get(unit_index);
}

/* input.jump(), input.crouch(): whether the first local player holds the game's
jump or crouch, whatever it is bound to, as their unit last took it */
static int script_input_control(
	struct script_arguments *state,
	short bit)
{
	struct unit_datum *unit = script_player_unit();

	output_integer(state, unit && TEST_FLAG(unit->unit.control_flags, bit));
	return 1;
}

static int script_input_jump(
	struct script_arguments *state)
{
	return script_input_control(state, _unit_control_jump_bit);
}

static int script_input_crouch(
	struct script_arguments *state)
{
	return script_input_control(state, _unit_control_crouch_modifier_bit);
}

static int script_player_alive(
	struct script_arguments *state)
{
	struct unit_datum *unit = script_player_unit();

	output_integer(state, unit && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit));
	return 1;
}

static int script_player_in_vehicle(
	struct script_arguments *state)
{
	struct unit_datum *unit = script_player_unit();

	output_integer(state, unit && unit->object.parent_object_index != NONE);
	return 1;
}

/* a player's datum from a script's index of it, NULL if it has none */
static struct player_datum *script_player_at(
	struct script_arguments *state,
	int argument)
{
	long index = argument_integer(state, argument);
	struct data_iterator iterator;
	struct player_datum *player;

	data_iterator_new(&iterator, player_data);
	while (player = data_iterator_next(&iterator))
	{
		if ((short)iterator.datum_index == index)
			return player;
	}
	return NULL;
}

/* (a unit alive, not one in a seat's camera: the script looks at its parent) */
static struct unit_datum *script_player_at_unit(
	struct script_arguments *state,
	int argument)
{
	struct player_datum *player = script_player_at(state, argument);

	return player && player->unit_index != NONE ? unit_try_and_get(player->unit_index) : NULL;
}

static int script_game_time(
	struct script_arguments *state)
{
	output_integer(state, game_time_get());
	return 1;
}

static int script_game_host(
	struct script_arguments *state)
{
	output_integer(state, !network_game_distributed_client());
	return 1;
}

static int script_game_players(struct script_arguments *state)
{
	struct data_iterator iterator;
	long ordinal = argument_integer(state, 1);
	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
		if (ordinal-- == 0) { output_integer(state, (short)iterator.datum_index); return 1; }
	output_none(state);
	return 1;
}

static int script_player_local(
	struct script_arguments *state)
{
	long player_index = local_player_get_player_index(mutator_scripts.local_player_index);

	if (player_index == NONE)
		output_none(state);
	else
		output_integer(state, (short)player_index);
	return 1;
}

static int script_player_button(
	struct script_arguments *state)
{
	long number = argument_integer(state, 2);
	long index = argument_integer(state, 1);

	argument_check(state, number >= 0 && number < MAXIMUM_SCRIPT_BUTTONS, 2, "no such button");
	output_integer(state, index >= 0 && index < MAXIMUM_SCRIPT_PLAYERS &&
		TEST_FLAG(mutator_scripts.player_buttons[index], (long)number));
	return 1;
}

/* the player's unit, nil with none or dead */
static int script_player_unit_of(
	struct script_arguments *state)
{
	struct player_datum *player = script_player_at(state, 1);
	struct unit_datum *unit = player && player->unit_index != NONE ? unit_try_and_get(player->unit_index) : NULL;

	if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		output_none(state);
	else
		output_integer(state, player->unit_index);
	return 1;
}

static int script_player_aim(
	struct script_arguments *state)
{
	struct unit_datum *unit = script_player_at_unit(state, 1);

	if (!unit)
		return 0;
	output_real(state, unit->unit.aiming_vector.i);
	output_real(state, unit->unit.aiming_vector.j);
	output_real(state, unit->unit.aiming_vector.k);
	return 3;
}

static int script_player_crouch(
	struct script_arguments *state)
{
	struct player_datum *player = script_player_at(state, 1);
	struct biped_datum *biped = player && player->unit_index != NONE ?
		(struct biped_datum *)object_try_and_get_and_verify_type(player->unit_index, _object_mask_biped) : NULL;

	output_real(state, biped ? biped->biped.crouch : 0.0f);
	return 1;
}

/* an object from a script's handle: it is a number the script could have made
up, and an identifier of none is one the data arrays assert on */
static struct object_datum *script_object_at(
	struct script_arguments *state,
	int argument,
	long *object_index)
{
	long handle = argument_integer(state, argument);
	long index = (long)handle;

	if ((long)index != handle || index == NONE || !(index >> 16))
		return NULL;
	if (object_index)
		*object_index = index;
	return object_try_and_get(index);
}

static int script_object_position(
	struct script_arguments *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object)
		return 0;
	output_real(state, object->object.position.x);
	output_real(state, object->object.position.y);
	output_real(state, object->object.position.z);
	return 3;
}

static int script_object_velocity(
	struct script_arguments *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object)
		return 0;
	output_real(state, object->object.translational_velocity.i);
	output_real(state, object->object.translational_velocity.j);
	output_real(state, object->object.translational_velocity.k);
	return 3;
}

static int script_object_parent(
	struct script_arguments *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object || object->object.parent_object_index == NONE)
		output_none(state);
	else
		output_integer(state, object->object.parent_object_index);
	return 1;
}

static int script_object_kind(
	struct script_arguments *state)
{
	static char const *const kinds[] =
	{
		"biped", "vehicle", "weapon", "equipment", "garbage", "projectile", "scenery", "machine", "control",
		"light_fixture", "placeholder", "sound_scenery",
	};
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object || object->object.type < 0 || object->object.type >= NUMBEROF(kinds))
		output_string(state, "");
	else
		output_string(state, kinds[object->object.type]);
	return 1;
}

static int script_object_tag(
	struct script_arguments *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object)
		output_string(state, "");
	else
		output_string(state, tag_get_name(object->definition_index));
	return 1;
}

static int script_object_alive(
	struct script_arguments *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	output_integer(state, object && !TEST_FLAG(object->object.damage_flags, _object_dead_bit));
	return 1;
}

static int script_object_acceleration_scale(
	struct script_arguments *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	output_real(state, object ? object_definition_get(object->definition_index)->object.acceleration_scale :
		0.0f);
	return 1;
}

/* whether this call may move this object: in a tick, and on a client only what
it has of a player's own, which the host's word corrects */
static boolean script_object_movable(
	long object_index)
{
	struct data_iterator iterator;
	struct player_datum *player;

	if (!mutator_scripts.ticking)
		return FALSE;
	if (!network_game_distributed_client())
		return TRUE;
	data_iterator_new(&iterator, player_data);
	while (player = data_iterator_next(&iterator))
	{
		if (player->unit_index == object_index)
			return TRUE;
	}
	return FALSE;
}

static real script_velocity_argument(
	struct script_arguments *state,
	int argument)
{
	real value = (real)argument_real(state, argument);

	/* (written so that a NaN is none) */
	if (!(value >= -MAXIMUM_SCRIPT_VELOCITY))
		return value < 0.0f ? -MAXIMUM_SCRIPT_VELOCITY : 0.0f;
	return value > MAXIMUM_SCRIPT_VELOCITY ? MAXIMUM_SCRIPT_VELOCITY : value;
}

static int script_object_set_velocity(
	struct script_arguments *state)
{
	long object_index = NONE;
	struct object_datum *object = script_object_at(state, 1, &object_index);

	if (object && script_object_movable(object_index))
	{
		object->object.translational_velocity.i = script_velocity_argument(state, 2);
		object->object.translational_velocity.j = script_velocity_argument(state, 3);
		object->object.translational_velocity.k = script_velocity_argument(state, 4);
		SET_FLAG(object->object.flags, _object_at_rest_bit, FALSE);
		SET_FLAG(object->object.flags, _object_do_not_recompute_node_matrices_bit, FALSE);
		/* (what the host has sent a client's player, it lets that client say) */
		if (!network_game_distributed_client())
		{
			struct data_iterator iterator;
			struct player_datum *player;

			data_iterator_new(&iterator, player_data);
			while (player = data_iterator_next(&iterator))
			{
				if (player->unit_index == object_index)
				{
					distributed_note_script_velocity((short)iterator.datum_index, object_index,
						&object->object.translational_velocity);
				}
			}
		}
	}
	return 0;
}

/* on the host: an object faces the way it was facing away from */
static int script_object_turn_around(
	struct script_arguments *state)
{
	long object_index = NONE;
	struct object_datum *object = script_object_at(state, 1, &object_index);

	if (object && mutator_scripts.ticking && !network_game_distributed_client())
	{
		object->object.forward.i = -object->object.forward.i;
		object->object.forward.j = -object->object.forward.j;
	}
	return 0;
}

/* a number a ray or a line can use: NaN and infinity are an error, not a crash */
static real script_finite_argument(
	struct script_arguments *state,
	int argument)
{
	double value = argument_real(state, argument);

	argument_check(state, value >= -1.0e6 && value <= 1.0e6, argument, "out of range");
	if (state->invalid) return 0.0f;
	return (real)value;
}

/* world.ray(x, y, z, dx, dy, dz, ignore): where the vector from the point first
hits the world or an object, and the object: x, y, z, object or nil. Nothing with
no hit. */
static int script_world_ray(
	struct script_arguments *state)
{
	struct collision_result collision;
	real_point3d point;
	real_vector3d vector;
	long ignore = NONE;
	boolean hit;

	point.x = script_finite_argument(state, 1);
	point.y = script_finite_argument(state, 2);
	point.z = script_finite_argument(state, 3);
	vector.i = script_finite_argument(state, 4);
	vector.j = script_finite_argument(state, 5);
	vector.k = script_finite_argument(state, 6);
	argument_check(state, magnitude3d(&vector) <= MAXIMUM_SCRIPT_RAY, 4, "too long");
	if (!argument_missing(state, 7) && !script_object_at(state, 7, &ignore))
		ignore = NONE;
	if (global_current_collision_user_depth >= MAXIMUM_COLLISION_USER_STACK_DEPTH)
		return 0;
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_unknown;
	/* (as a projectile's, through water) */
	hit = collision_test_vector((_collision_test_for_projectiles_flags | _collision_test_objects_all_types_flags) &
		~FLAG(_collision_test_media_bit), &point, &vector, ignore, &collision);
	global_current_collision_user_depth--;
	if (!hit)
		return 0;
	output_real(state, collision.point.x);
	output_real(state, collision.point.y);
	output_real(state, collision.point.z);
	if (collision.type != _collision_result_object)
		output_none(state);
	else
		output_integer(state, collision.object_index);
	return 4;
}

static void script_line_color(
	struct script_arguments *state,
	int argument,
	struct mutator_script_line *line)
{
	line->color.alpha = 1.0f;
	line->color.red = PIN(script_finite_argument(state, argument), 0.0f, 1.0f);
	line->color.green = PIN(script_finite_argument(state, argument + 1), 0.0f, 1.0f);
	line->color.blue = PIN(script_finite_argument(state, argument + 2), 0.0f, 1.0f);
}

/* draw.line(x, y, z, x2, y2, z2, r, g, b): a line in the world, this frame */
static int script_draw_line(
	struct script_arguments *state)
{
	if (mutator_scripts.line_count < MAXIMUM_SCRIPT_LINES)
	{
		struct mutator_script_line *line = &mutator_scripts.lines[mutator_scripts.line_count];

		line->from_view = FALSE;
		line->from[0] = script_finite_argument(state, 1);
		line->from[1] = script_finite_argument(state, 2);
		line->from[2] = script_finite_argument(state, 3);
		line->to.x = script_finite_argument(state, 4);
		line->to.y = script_finite_argument(state, 5);
		line->to.z = script_finite_argument(state, 6);
		script_line_color(state, 7, line);
		mutator_scripts.line_count++;
	}
	return 0;
}

/* draw.line_from_view(right, down, forward, x, y, z, r, g, b): the same from a
place by the camera as this frame draws it, which stays with the first person's
view where the player's position, a tick's, would not */
static int script_draw_line_from_view(
	struct script_arguments *state)
{
	if (mutator_scripts.line_count < MAXIMUM_SCRIPT_LINES)
	{
		struct mutator_script_line *line = &mutator_scripts.lines[mutator_scripts.line_count];

		line->from_view = TRUE;
		line->from[0] = script_finite_argument(state, 1);
		line->from[1] = script_finite_argument(state, 2);
		line->from[2] = script_finite_argument(state, 3);
		line->to.x = script_finite_argument(state, 4);
		line->to.y = script_finite_argument(state, 5);
		line->to.z = script_finite_argument(state, 6);
		script_line_color(state, 7, line);
		mutator_scripts.line_count++;
	}
	return 0;
}

static int script_camera_first_person(
	struct script_arguments *state)
{
	struct unit_datum *unit = script_player_unit();

	output_integer(state, unit && unit->object.parent_object_index == NONE && !cinematic_in_progress() &&
		!(director_camera_scripted && *director_camera_scripted) &&
		!mutator_view_active(mutator_scripts.local_player_index));
	return 1;
}

static int script_camera_free(
	struct script_arguments *state)
{
	output_integer(state, !cinematic_in_progress() && !(director_camera_scripted && *director_camera_scripted));
	return 1;
}

static int script_camera_view(struct script_arguments *state)
{
	real distance = argument_real(state, 1);
	real height = argument_real(state, 2);
	mutator_scripts.view_distance = distance >= 0.6f ? (distance < 20.f ? distance : 20.f) : 0.6f;
	mutator_scripts.view_height = height >= -2.f ? (height < 5.f ? height : 5.f) : -2.f;
	mutator_scripts.view_backward = argument_integer(state, 3) != 0;
	mutator_scripts.view_crosshair = argument_integer(state, 4) != 0;
	mutator_scripts.view_active = TRUE;
	return 0;
}

/* ---------- models */

static void script_model_failure(struct mutator_script *script, char const *tag,
	short reason, char const *message);

/* a script's models are objects it makes on its own machine: never sent, never
updated, picked up or hit, and gone before a game is saved. It places and poses
them itself, for what it draws */

static void script_model_forget(
	struct mutator_script_model *model)
{
	csmemset(model, 0, sizeof(*model));
	mutator_scripts_model_count--;
}

static void script_model_delete(
	struct mutator_script_model *model)
{
	if (object_try_and_get(model->object_index))
		object_delete(model->object_index);
	script_model_forget(model);
}

/* the script's model from its handle, NULL if it has none there (or the game
has deleted it) */
static struct mutator_script_model *script_model_at(
	struct script_arguments *state,
	int argument)
{
	struct mutator_script *script = state->script;
	short script_index = (short)(script - mutator_scripts.scripts);
	long handle = argument_integer(state, argument);
	short index;

	for (index = 0; index < MAXIMUM_SCRIPT_MODELS; index++)
	{
		struct mutator_script_model *model = &mutator_scripts.models[index];
		struct object_datum *object;

		if (!model->used || model->script_index != script_index || model->object_index != handle)
			continue;
		object = object_try_and_get(model->object_index);
		if (!object || !TEST_FLAG(object->object.flags, _object_cannot_be_activated_bit))
		{
			script_model_failure(script, model->tag, object ? 4 : 3,
				object ? "live object lost script-only flag" : "object deleted after model_create");
			script_model_forget(model);
			return NULL;
		}
		return model;
	}
	return NULL;
}

static short script_model_node_count(
	long object_index)
{
	struct object_datum *object = object_get(object_index);
	struct object_definition *definition = object_definition_get(object->definition_index);
	short count;

	if (definition->object.model.index == NONE)
		return 0;
	count = (short)MIN(model_definition_get(definition->object.model.index)->nodes.count, MAXIMUM_NODES_PER_MODEL);
	return (short)MIN(count, object->object.node_matrices.size / (short)sizeof(real_matrix4x3));
}

/* its bounding sphere round its nodes where they are, so that it is drawn
wherever they are seen */
static void script_model_bound(
	long object_index)
{
	struct object_datum *object = object_get(object_index);
	real_matrix4x3 const *nodes = object_get_node_matrices(object_index);
	short count = script_model_node_count(object_index);
	real_point3d low, high;
	short index;

	if (!count)
		return;
	low = high = nodes[0].position;
	for (index = 1; index < count; index++)
	{
		low.x = MIN(low.x, nodes[index].position.x);
		low.y = MIN(low.y, nodes[index].position.y);
		low.z = MIN(low.z, nodes[index].position.z);
		high.x = MAX(high.x, nodes[index].position.x);
		high.y = MAX(high.y, nodes[index].position.y);
		high.z = MAX(high.z, nodes[index].position.z);
	}
	object_disconnect_from_map(object_index);
	object->object.bounding_sphere_center.x = (low.x + high.x) * 0.5f;
	object->object.bounding_sphere_center.y = (low.y + high.y) * 0.5f;
	object->object.bounding_sphere_center.z = (low.z + high.z) * 0.5f;
	object->object.bounding_sphere_radius = distance3d(&object->object.bounding_sphere_center, &high) +
		object_definition_get(object->definition_index)->object.bounding_radius;
	object_reconnect_to_map(object_index, NULL);
}

/* a unit vector from three arguments, FALSE for none */
static boolean script_unit_vector_argument(
	struct script_arguments *state,
	int argument,
	real_vector3d *vector)
{
	vector->i = script_finite_argument(state, argument);
	vector->j = script_finite_argument(state, argument + 1);
	vector->k = script_finite_argument(state, argument + 2);
	return normalize3d(vector) > 0.0001f;
}

/* A tag can fail at several stages; report each reason only once for this
script and tag, not once per frame or player. */
static void script_model_failure(
	struct mutator_script *script,
	char const *tag,
	short reason,
	char const *message)
{
	short index;
	for (index = 0; index < script->model_failure_count; index++)
		if (!strcmp(script->model_failure_tags[index], tag))
			break;
	if (index == script->model_failure_count)
	{
		if (index == NUMBEROF(script->model_failure_tags)) return;
		snprintf(script->model_failure_tags[index], sizeof(script->model_failure_tags[index]), "%s", tag);
		script->model_failure_count++;
	}
	if (TEST_FLAG(script->model_failure_reasons[index], reason)) return;
	SET_FLAG(script->model_failure_reasons[index], reason, TRUE);
	error(_error_silent, "mutators: %s model '%s': %s", script->key, tag, message);
}

/* model.create(tag, x, y, z, owner): a weapon's, a piece of equipment's,
garbage's, a projectile's or scenery's model at the point, its handle or nil.
Its object's creation effect plays, and its attachments are made. The owner, a
unit, is whose it is (what a projectile of no one's is to a unit nearby) */
static int script_model_create(
	struct script_arguments *state)
{
	static long const groups[] = { 'weap', 'eqip', 'garb', 'proj', 'scen' };
	struct mutator_script *script = state->script;
	char const *name = argument_string(state, 1);
	struct object_placement_data data;
	struct mutator_script_model *model = NULL;
	long definition_index = NONE;
	long owner = NONE;
	long object_index;
	short index;

	for (index = 0; index < NUMBEROF(groups) && definition_index == NONE; index++)
		definition_index = tag_loaded(groups[index], name);
	for (index = 0; index < MAXIMUM_SCRIPT_MODELS && !model; index++)
	{
		if (!mutator_scripts.models[index].used)
			model = &mutator_scripts.models[index];
	}
	if (definition_index == NONE || !model)
	{
		script_model_failure(script, name, definition_index == NONE ? 0 : 1,
			definition_index == NONE ? "tag not loaded (weap/eqip/garb/proj/scen; exact path required)" : "model registry full");
		return 0;
	}
	if (argument_missing(state, 5) || !script_object_at(state, 5, &owner) ||
		!object_try_and_get_and_verify_type(owner, _object_mask_unit))
	{
		owner = NONE;
	}
	object_placement_data_new(&data, definition_index, owner);
	data.position.x = script_finite_argument(state, 2);
	data.position.y = script_finite_argument(state, 3);
	data.position.z = script_finite_argument(state, 4);
	mutator_scripts.creating_model = TRUE;
	object_index = object_new(&data);
	mutator_scripts.creating_model = FALSE;
	if (object_index == NONE)
	{
		script_model_failure(script, name, 2, "object_new failed (tag runtime type, allocation, or constructor)");
		return 0;
	}
	{
		struct object_header_datum *header = object_header_get(object_index);
		struct object_datum *object = object_get(object_index);

		/* (never updated: it stays as the script puts it, and does nothing
		itself; never hit: it is in the clusters' lists of what is not) */
		SET_FLAG(object->object.flags, _object_cannot_be_activated_bit, TRUE);
		/* Projectile/garbage constructors request PVS deletion. A deliberately
		inactive script visual must survive leaving the active clusters. */
		SET_FLAG(object->object.flags, _object_deleted_when_deactivated_bit, FALSE);
		SET_FLAG(object->object.flags, _object_shadowless_bit, TRUE);
		SET_FLAG(header->flags, _object_header_being_created_bit, FALSE);
		object_deactivate(object_index);
		object_disconnect_from_map(object_index);
		SET_FLAG(object->object.flags, _object_has_collision_model_bit, FALSE);
		object_reconnect_to_map(object_index, NULL);
	}
	model->used = TRUE;
	model->script_index = (short)(script - mutator_scripts.scripts);
	model->object_index = object_index;
	snprintf(model->tag, sizeof(model->tag), "%s", name);
	model->view_node = NONE;
	model->pose_logged = 0;
	error(_error_silent, "mutator models: %s created tag=%s definition=0x%08lx object=0x%08lx registry=%ld player=%ld nodes=%d position=(%g,%g,%g)",
		script->key, name, definition_index, object_index, (long)(model - mutator_scripts.models),
		script->current_player, script_model_node_count(object_index), data.position.x, data.position.y, data.position.z);
	mutator_scripts_model_count++;
	output_integer(state, object_index);
	return 1;
}

static int script_model_delete_function(
	struct script_arguments *state)
{
	struct mutator_script_model *model = script_model_at(state, 1);

	if (model)
		script_model_delete(model);
	return 0;
}

static int script_model_exists(
	struct script_arguments *state)
{
	output_integer(state, script_model_at(state, 1) != NULL);
	return 1;
}

/* model.place(handle, x, y, z, forward x, y, z, up x, y, z): where it is and
which way it faces (else as it was), its nodes as its model has them */
static int script_model_place(
	struct script_arguments *state)
{
	struct mutator_script_model *model = script_model_at(state, 1);
	real_point3d position;
	real_vector3d forward, up;
	boolean faces, upright;

	if (!model)
		return 0;
	position.x = script_finite_argument(state, 2);
	position.y = script_finite_argument(state, 3);
	position.z = script_finite_argument(state, 4);
	faces = !argument_missing(state, 5) && script_unit_vector_argument(state, 5, &forward);
	upright = faces && !argument_missing(state, 8) && script_unit_vector_argument(state, 8, &up);
	if (upright)
	{
		/* (at right angles to forward) */
		real along = dot_product3d(&up, &forward);

		up.i -= forward.i * along;
		up.j -= forward.j * along;
		up.k -= forward.k * along;
		upright = normalize3d(&up) > 0.0001f;
	}
	object_set_position(model->object_index, &position, faces ? &forward : NULL, upright ? &up : NULL);
	if (!TEST_FLAG(model->pose_logged, 0))
	{
		SET_FLAG(model->pose_logged, 0, TRUE);
		error(_error_silent, "mutator models: %s first place tag=%s object=0x%08lx registry=%ld player=%ld position=(%g,%g,%g)",
			state->script->key, model->tag, model->object_index, (long)(model - mutator_scripts.models),
			state->script->current_player, position.x, position.y, position.z);
	}
	return 0;
}

static short script_model_node_argument(
	struct script_arguments *state,
	struct mutator_script_model const *model,
	int argument)
{
	long node = argument_integer(state, argument);

	return node >= 0 && node < script_model_node_count(model->object_index) ? (short)node : NONE;
}

/* model.node(handle, node, x, y, z): one of its nodes moved there (until it is
placed again), which draws its vertices of that node there: a rope between two */
static int script_model_node(
	struct script_arguments *state)
{
	struct mutator_script_model *model = script_model_at(state, 1);
	short node;

	if (!model || (node = script_model_node_argument(state, model, 2)) == NONE)
		return 0;
	{
		real_matrix4x3 *nodes = object_get_node_matrices(model->object_index);

		nodes[node].position.x = script_finite_argument(state, 3);
		nodes[node].position.y = script_finite_argument(state, 4);
		nodes[node].position.z = script_finite_argument(state, 5);
		if (node < 2 && !TEST_FLAG(model->pose_logged, node + 1))
		{
			SET_FLAG(model->pose_logged, node + 1, TRUE);
			error(_error_silent, "mutator models: %s first node tag=%s object=0x%08lx registry=%ld player=%ld node=%d position=(%g,%g,%g) scale=%g",
				state->script->key, model->tag, model->object_index, (long)(model - mutator_scripts.models),
				state->script->current_player, node, nodes[node].position.x, nodes[node].position.y, nodes[node].position.z, nodes[node].scale);
		}
	}
	script_model_bound(model->object_index);
	return 0;
}

/* model.node_from_view(handle, node, right, down, forward): the same, this
frame, from a place by the camera as it is drawn (draw.line_from_view's) */
static int script_model_node_from_view(
	struct script_arguments *state)
{
	struct mutator_script_model *model = script_model_at(state, 1);
	short node;

	if (!model || (node = script_model_node_argument(state, model, 2)) == NONE)
		return 0;
	model->view_node = node;
	model->view_from[0] = script_finite_argument(state, 3);
	model->view_from[1] = script_finite_argument(state, 4);
	model->view_from[2] = script_finite_argument(state, 5);
	return 0;
}

/* model.face_view(handle): its nodes turned as the camera is, this frame, where
they are */
static int script_model_face_view(
	struct script_arguments *state)
{
	struct mutator_script_model *model = script_model_at(state, 1);

	if (model)
		model->face_view = TRUE;
	return 0;
}

static void script_models_delete(
	short script_index)
{
	short index;

	for (index = 0; index < MAXIMUM_SCRIPT_MODELS; index++)
	{
		struct mutator_script_model *model = &mutator_scripts.models[index];

		if (model->used && (script_index == NONE || model->script_index == script_index))
			script_model_delete(model);
	}
}

static int script_dispatch(void *context, const long *arguments, long *value,
	int (*function)(struct script_arguments *), short count, boolean vector)
{
	struct script_arguments state;
	short index;
	csmemset(&state, 0, sizeof(state));
	state.script = context;
	/* A missing handle is NONE; void callbacks return zero. */
	for (index = 0; index < 4; index++) state.output[index] = NONE;
	state.count = count;
	for (index = 0; index < count; index++) state.arguments[index] = arguments[index];
	function(&state);
	if (state.invalid) return FALSE;
	if (vector)
	{
		for (index = 0; index < 4; index++) state.script->output[index] = state.output[index];
		*value = state.outputs >= 3;
	}
	else *value = state.outputs ? state.output[0] : NONE;
	return TRUE;
}
#define BRIDGE(name, count, vector) \
static int api_##name(void *c, const long *a, long *v) \
{ return script_dispatch(c, a, v, script_##name, count, vector); }
BRIDGE(input_held, 1, FALSE)
BRIDGE(input_claim, 1, FALSE)
BRIDGE(input_button, 2, FALSE)
BRIDGE(input_jump, 0, FALSE)
BRIDGE(input_crouch, 0, FALSE)
BRIDGE(player_alive, 0, FALSE)
BRIDGE(player_in_vehicle, 0, FALSE)
BRIDGE(player_local, 0, FALSE)
BRIDGE(player_button, 2, FALSE)
BRIDGE(player_unit_of, 1, FALSE)
BRIDGE(player_aim, 1, TRUE)
BRIDGE(player_crouch, 1, FALSE)
BRIDGE(game_players, 1, FALSE)
BRIDGE(game_time, 0, FALSE)
BRIDGE(game_host, 0, FALSE)
BRIDGE(camera_free, 0, FALSE)
BRIDGE(camera_first_person, 0, FALSE)
BRIDGE(camera_view, 4, FALSE)
BRIDGE(object_position, 1, TRUE)
BRIDGE(object_velocity, 1, TRUE)
BRIDGE(object_set_velocity, 4, FALSE)
BRIDGE(object_parent, 1, FALSE)
BRIDGE(object_kind, 1, FALSE)
BRIDGE(object_tag, 1, FALSE)
BRIDGE(object_alive, 1, FALSE)
BRIDGE(object_acceleration_scale, 1, FALSE)
BRIDGE(object_turn_around, 1, FALSE)
BRIDGE(world_ray, 7, TRUE)
BRIDGE(model_create, 5, FALSE)
BRIDGE(model_delete_function, 1, FALSE)
BRIDGE(model_place, 4, FALSE)
BRIDGE(model_exists, 1, FALSE)
BRIDGE(model_node, 5, FALSE)
BRIDGE(model_node_from_view, 5, FALSE)
BRIDGE(model_face_view, 1, FALSE)
#undef BRIDGE
/* Packed RGB keeps the drawing signature under HS's eight argument limit. */
static int api_draw_line(void *c, const long *a, long *v)
{
	long expanded[9];
	real color;
	short index;
	for (index = 0; index < 6; index++) expanded[index] = a[index];
	for (index = 0; index < 3; index++)
	{
		color = ((a[6] >> (16 - 8 * index)) & 255) / 255.0f;
		memcpy(&expanded[6 + index], &color, sizeof(color));
	}
	return script_dispatch(c, expanded, v, script_draw_line, 9, FALSE);
}
static int api_draw_line_from_view(void *c, const long *a, long *v)
{
	long expanded[9];
	real color;
	short index;
	for (index = 0; index < 6; index++) expanded[index] = a[index];
	for (index = 0; index < 3; index++)
	{
		color = ((a[6] >> (16 - 8 * index)) & 255) / 255.0f;
		memcpy(&expanded[6 + index], &color, sizeof(color));
	}
	return script_dispatch(c, expanded, v, script_draw_line_from_view, 9, FALSE);
}
static int api_model_forward(void *context, const long *arguments, long *value)
{
	struct script_arguments state;
	struct mutator_script_model *model;
	struct object_datum *object;
	short index;
	csmemset(&state, 0, sizeof(state));
	state.script = context; state.count = 4;
	for (index = 0; index < 4; index++) state.arguments[index] = arguments[index];
	model = script_model_at(&state, 1);
	object = model ? object_try_and_get(model->object_index) : NULL;
	if (object)
	{
		for (index = 0; index < 3; index++) state.arguments[4 + index] = arguments[1 + index];
		memcpy(&state.arguments[1], &object->object.position.x, sizeof(real));
		memcpy(&state.arguments[2], &object->object.position.y, sizeof(real));
		memcpy(&state.arguments[3], &object->object.position.z, sizeof(real));
		state.count = 7;
		script_model_place(&state);
	}
	*value = 0;
	return !state.invalid;
}
/* The scalar orientation setter retains world up. Full orientation remains
 * available inside the host's placement helper; HS calls never need tables. */
#define RESULT(name, index) \
static int api_result_##name(void *c, const long *a, long *v) \
{ (void)a; *v = ((struct mutator_script *)c)->output[index]; return TRUE; }
RESULT(x, 0)
RESULT(y, 1)
RESULT(z, 2)
RESULT(object, 3)
#undef RESULT
struct setting_query { struct mutator_script *script; const char *name; short kind; long value; };
static void script_setting_find(void *context, const char *name, const char *text,
	double number, int boolean_value, int kind)
{
	struct setting_query *query = context;
	if (strcmp(name, query->name) || kind != query->kind) return;
	if (kind == 1) { real scalar = (real)number; memcpy(&query->value, &scalar, sizeof(scalar)); }
	else if (kind == 2) query->value = boolean_value;
	else
	{
		short index;
		struct mutator_script *script = query->script;
		for (index = 0; index < script->setting_string_count; index++)
			if (!strcmp(script->setting_strings[index], text)) break;
		if (index == script->setting_string_count && index < 64 && strlen(text) < 256)
		{
			csstrcpy(script->setting_strings[index], text);
			script->setting_string_count++;
		}
		if (index < script->setting_string_count) query->value = (long)script->setting_strings[index];
	}
}
static int script_setting_get(void *context, const long *arguments, long *value, short kind)
{
	struct setting_query query;
	query.script = context; query.name = (const char *)arguments[0]; query.kind = kind;
	query.value = kind == 0 ? (long)"" : 0;
	port_mutators_script_settings(query.script->key, script_setting_find, &query);
	*value = query.value;
	return TRUE;
}
static int api_settings_real(void *c, const long *a, long *v) { return script_setting_get(c, a, v, 1); }
static int api_settings_string(void *c, const long *a, long *v) { return script_setting_get(c, a, v, 0); }
static int api_settings_boolean(void *c, const long *a, long *v) { return script_setting_get(c, a, v, 2); }
/* Typed, bounded script-private storage. Lifecycle synchronizes full salted
 * player datums before exposing a row, so a reused slot never inherits state. */
static int script_state_slot(struct mutator_script *s, const long *a)
{
	return a[0] >= 0 && a[0] < MAXIMUM_SCRIPT_PLAYERS &&
		a[1] >= 0 && a[1] < 32 && s->players[a[0]].used;
}
static int api_state_get_long(void *c, const long *a, long *v)
{ struct mutator_script *s = c; *v = script_state_slot(s,a) ? s->players[a[0]].integers[a[1]] : 0; return TRUE; }
static int api_state_set_long(void *c, const long *a, long *v)
{ struct mutator_script *s = c; if (script_state_slot(s,a)) s->players[a[0]].integers[a[1]] = a[2]; *v = 0; return TRUE; }
static int api_state_get_real(void *c, const long *a, long *v)
{ struct mutator_script *s = c; *v = 0; if (script_state_slot(s,a)) memcpy(v, &s->players[a[0]].reals[a[1]], sizeof(real)); return TRUE; }
static int api_state_set_real(void *c, const long *a, long *v)
{ struct mutator_script *s = c; real r; memcpy(&r,&a[2],sizeof(r)); if (script_state_slot(s,a) && __builtin_isfinite(r)) s->players[a[0]].reals[a[1]] = r; *v = 0; return TRUE; }
static int api_game_player_current(void *c, const long *a, long *v)
{ (void)a; *v = ((struct mutator_script *)c)->current_player; return TRUE; }
static int api_math_sqrt(void *c, const long *a, long *v)
{ real x, r = 0; (void)c; memcpy(&x,a,sizeof(x)); if (__builtin_isfinite(x) && x > 0) r = sqrtf(x); *v = 0; memcpy(v,&r,sizeof(r)); return TRUE; }
static int api_math_divide(void *c, const long *a, long *v)
{ real x,y,r = 0; (void)c; memcpy(&x,&a[0],sizeof(x)); memcpy(&y,&a[1],sizeof(y)); if (__builtin_isfinite(x) && __builtin_isfinite(y) && y != 0) { r = x/y; if (!__builtin_isfinite(r)) r = 0; } *v = 0; memcpy(v,&r,sizeof(r)); return TRUE; }
static int api_math_equals_string(void *c, const long *a, long *v)
{ (void)c; *v = !strcmp((const char *)a[0], (const char *)a[1]); return TRUE; }
static int api_object_root(void *c, const long *a, long *v)
{
	long handle = a[0]; short depth; (void)c; *v = NONE;
	for (depth = 0; depth < 128 && handle != NONE && (handle >> 16); depth++)
	{
		struct object_datum *o = object_try_and_get(handle);
		if (!o) return TRUE;
		if (o->object.parent_object_index == NONE) { *v = handle; return TRUE; }
		handle = o->object.parent_object_index;
	}
	return TRUE;
}
#define R _hs_type_real
#define I _hs_type_long_integer
#define B _hs_type_boolean
#define S _hs_type_string
#define V _hs_type_void
#define API0(name, type) { #name, type, 0, {0}, api_##name }
#define API1(name, type, a) { #name, type, 1, {a}, api_##name }
#define API2(name, type, a,b) { #name, type, 2, {a,b}, api_##name }
#define API4(name, type, a,b,c,d) { #name, type, 4, {a,b,c,d}, api_##name }
#define API5(name, type, a,b,c,d,e) { #name, type, 5, {a,b,c,d,e}, api_##name }
static const struct mutator_hs_api script_api[] =
{
	API0(game_player_current, I),
	API2(state_get_long, I, I,I), API2(state_get_real, R, I,I),
	{ "state_set_long", V, 3, {I,I,I}, api_state_set_long },
	{ "state_set_real", V, 3, {I,I,R}, api_state_set_real },
	API1(math_sqrt, R, R), API2(math_divide, R, R,R),
	API2(math_equals_string, B, S,S), API1(object_root, I, I),
	API1(input_held, B, S), API1(input_claim, V, S), API2(input_button, V, I,B),
	API0(input_jump, B), API0(input_crouch, B),
	API0(player_alive, B), API0(player_in_vehicle, B), API0(player_local, I),
	API2(player_button, B, I,I), API1(player_unit_of, I, I), API1(player_aim, B, I), API1(player_crouch, R, I),
	API1(game_players, I, I), API0(game_time, I), API0(game_host, B),
	API0(camera_free, B), API0(camera_first_person, B), API4(camera_view, V, R,R,B,B),
	API1(object_position, B, I), API1(object_velocity, B, I), API4(object_set_velocity, V, I,R,R,R),
	API1(object_parent, I, I), API1(object_kind, S, I), API1(object_tag, S, I),
	API1(object_alive, B, I), API1(object_acceleration_scale, R, I), API1(object_turn_around, V, I),
	{ "world_ray", B, 7, {R,R,R,R,R,R,I}, api_world_ray },
	{ "draw_line", V, 7, {R,R,R,R,R,R,I}, api_draw_line },
	{ "draw_line_from_view", V, 7, {R,R,R,R,R,R,I}, api_draw_line_from_view },
	API4(model_forward, V, I,R,R,R),
	API5(model_create, I, S,R,R,R,I), API1(model_delete_function, V, I), API4(model_place, V, I,R,R,R),
	API1(model_exists, B, I), API5(model_node, V, I,I,R,R,R),
	API5(model_node_from_view, V, I,I,R,R,R), API1(model_face_view, V, I),
	{ "player_local_index", I, 0, {0}, api_player_local },
	{ "player_unit", I, 1, {I}, api_player_unit_of },
	{ "model_delete", V, 1, {I}, api_model_delete_function },
	API0(result_x, R), API0(result_y, R), API0(result_z, R), API0(result_object, I),
	API1(settings_real, R, S), API1(settings_string, S, S), API1(settings_boolean, B, S)
};
/* Compile against the exact live API without entering game lifecycle callbacks.
 * Literal-only global initialization is enforced by the native backend. */
boolean mutator_scripts_validate_source(const char *source, long length, struct mutator_hs_result *result)
{
	struct mutator_script *context = calloc(1, sizeof(*context));
	struct mutator_hs_program *program;
	if (!context) return FALSE;
	program = mutator_hs_program_compile(source, length, script_api,
		sizeof(script_api)/sizeof(script_api[0]), context, result);
	if (program) mutator_hs_program_free(program);
	free(context);
	return program != NULL;
}

/* halo --check-script <file.hsc> (port/linux/src/server_mode.c reads it):
compiles a mutator's script against the host API and says why it does not,
for script authors and the tests; the process's code, 0 when it compiles */
int mutator_scripts_check_text(const char *name, const char *source, long length)
{
	struct mutator_hs_result result;
	boolean compiled;

	if (length > MAXIMUM_SCRIPT_SOURCE)
	{
		fprintf(stderr, "%s: longer than %d bytes\n", name, MAXIMUM_SCRIPT_SOURCE);
		return 1;
	}
	memset(&result, 0, sizeof(result));
	compiled = mutator_scripts_validate_source(source, length, &result);
	if (!compiled)
	{
		fprintf(stderr, "%s: %s\n", name, *result.error ? result.error : "does not compile");
		return 1;
	}
	fprintf(stdout, "%s: compiles\n", name);
	return 0;
}
#undef API0
#undef API1
#undef API2
#undef API4
#undef API5
#undef R
#undef I
#undef B
#undef S
#undef V

/* the scripts running counted again; with none, nothing of theirs is left for
the game's hooks to find */
static void scripts_count(
	void)
{
	short index;

	mutator_scripts.running = 0;
	for (index = 0; index < MAXIMUM_MUTATOR_SCRIPTS; index++)
	{
		struct mutator_script const *script = &mutator_scripts.scripts[index];

		if (script->state && script->kept && !script->failed)
			mutator_scripts.running++;
	}
	if (!mutator_scripts.running)
	{
		mutator_scripts.view_active = FALSE;
		mutator_scripts.line_count = 0;
		mutator_scripts.claim_count = 0;
		mutator_scripts.local_buttons = 0;
		csmemset(mutator_scripts.player_buttons, 0, sizeof(mutator_scripts.player_buttons));
	}
}

static void script_close(
	struct mutator_script *script)
{
	if (script->state)
	{
		if (!script->failed)
			mutator_hs_program_call(script->state, "stop", MAXIMUM_SCRIPT_INSTRUCTIONS, &script->result);
		mutator_hs_program_free(script->state);
	}
	script_models_delete((short)(script - mutator_scripts.scripts));
	csmemset(script, 0, sizeof(*script));
	scripts_count();
}

static void script_failed(
	struct mutator_script *script,
	char const *when)
{
	char const *message = script->result.error;

	error(_error_silent, "mutator scripts: %s %s, and is stopped: %s", script->key, when, message);
	mutator_share_fail("A required host script failed compilation or execution. Ask the host to repair its script or use a compatible build, then reconnect.");
	script->failed = TRUE;
	script_models_delete((short)(script - mutator_scripts.scripts));
	scripts_count();
}

/* whether an object is a script's model */
static boolean script_model_object(
	long object_index)
{
	short index;

	for (index = 0; index < MAXIMUM_SCRIPT_MODELS; index++)
	{
		if (mutator_scripts.models[index].used && mutator_scripts.models[index].object_index == object_index)
			return TRUE;
	}
	return FALSE;
}

/* ---------- public code */

void mutator_scripts_begin(
	void)
{
	short index;

	for (index = 0; index < MAXIMUM_MUTATOR_SCRIPTS; index++)
		mutator_scripts.scripts[index].kept = FALSE;
	/* (the objects of the game before, which its map takes with it) */
	for (index = 0; index < MAXIMUM_SCRIPT_MODELS; index++)
	{
		if (mutator_scripts.models[index].used)
			script_model_forget(&mutator_scripts.models[index]);
	}
}

void mutator_scripts_add(
	char const *key)
{
	struct mutator_script *script = NULL;
	short index;

	for (index = 0; index < MAXIMUM_MUTATOR_SCRIPTS; index++)
	{
		if (mutator_scripts.scripts[index].state && !strcmp(mutator_scripts.scripts[index].key, key))
		{
			mutator_scripts.scripts[index].kept = TRUE;
			return;
		}
		if (!script && !mutator_scripts.scripts[index].state)
			script = &mutator_scripts.scripts[index];
	}
	if (!script)
	{
		error(_error_silent, "mutator scripts: more than %d", MAXIMUM_MUTATOR_SCRIPTS);
		return;
	}
	csmemset(script, 0, sizeof(*script));
	csstrncpy(script->key, key, SCRIPT_KEY_LENGTH - 1);
	script->kept = TRUE;
	{
		unsigned long size = 0;
		char *text = port_mutators_script_read(script->key, &size);
		if (text)
		{
			script->state = mutator_hs_program_compile(text, size, script_api,
				NUMBEROF(script_api), script, &script->result);
			port_mutators_script_free(text);
		}
		else csstrcpy(script->result.error, "cannot read program");
	}
	if (!script->state || !mutator_hs_program_call(script->state, "startup",
		MAXIMUM_SCRIPT_INSTRUCTIONS, &script->result))
		script_failed(script, "cannot start");
	else
		error(_error_silent, "mutator scripts: %s started", key);
	scripts_count();
}

void mutator_scripts_commit(
	void)
{
	short index;

	for (index = 0; index < MAXIMUM_MUTATOR_SCRIPTS; index++)
	{
		if (mutator_scripts.scripts[index].state && !mutator_scripts.scripts[index].kept)
			script_close(&mutator_scripts.scripts[index]);
	}
	scripts_count();
}

short mutator_scripts_keys(
	char keys[][SCRIPT_KEY_LENGTH],
	short maximum)
{
	short count = 0;
	short index;

	for (index = 0; index < MAXIMUM_MUTATOR_SCRIPTS && count < maximum; index++)
	{
		if (mutator_scripts.scripts[index].state && mutator_scripts.scripts[index].kept)
			csmemcpy(keys[count++], mutator_scripts.scripts[index].key, SCRIPT_KEY_LENGTH);
	}
	return count;
}

/* Deliver departure before clearing a row; player_stop may release models and
 * other script-owned resources even though the engine player is already gone. */
static int script_players_call(struct mutator_script *s, const char *entry)
{
	long datums[MAXIMUM_SCRIPT_PLAYERS];
	struct data_iterator iterator;
	short i;
	for (i=0;i<MAXIMUM_SCRIPT_PLAYERS;i++) datums[i]=NONE;
	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
	{
		long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		if (slot >= 0 && slot < MAXIMUM_SCRIPT_PLAYERS) datums[slot]=iterator.datum_index;
	}
	for (i=0;i<MAXIMUM_SCRIPT_PLAYERS;i++)
	{
		s->current_player=i;
		if (s->players[i].used && s->players[i].datum != datums[i])
		{
			if (!mutator_hs_program_call(s->state,"player_stop",MAXIMUM_SCRIPT_INSTRUCTIONS,&s->result)) return FALSE;

			memset(&s->players[i],0,sizeof(s->players[i]));
		}
		if (datums[i] == NONE) continue;
		s->players[i].used=TRUE;
		s->players[i].datum=datums[i];
		if (!mutator_hs_program_call(s->state,entry,MAXIMUM_SCRIPT_INSTRUCTIONS,&s->result)) return FALSE;
	}
	s->current_player=NONE;
	return TRUE;
}

void mutator_scripts_update(
	short local_player_index)
{
	short index;

	if (local_player_index != 0 || !mutator_scripts.running)
		return;
	mutator_scripts.view_active = FALSE;
	mutator_scripts.claim_count = 0;
	mutator_scripts.line_count = 0;
	mutator_scripts.local_player_index = local_player_index;
	for (index = 0; index < MAXIMUM_SCRIPT_MODELS; index++)
	{
		mutator_scripts.models[index].view_node = NONE;
		mutator_scripts.models[index].face_view = FALSE;
	}
	for (index = 0; index < MAXIMUM_MUTATOR_SCRIPTS; index++)
	{
		struct mutator_script *script = &mutator_scripts.scripts[index];

		if (!script->state || !script->kept || script->failed)
			continue;
		if (!mutator_hs_program_call(script->state, "frame", MAXIMUM_SCRIPT_INSTRUCTIONS, &script->result) ||
			!script_players_call(script, "player_frame"))
		{
			script_failed(script, "failed");
			mutator_scripts.view_active = FALSE;
		}
	}
	/* (one that holds no unit has no camera to move) */
	if (mutator_scripts.view_active && !script_player_unit())
		mutator_scripts.view_active = FALSE;
}

void mutator_scripts_tick(
	void)
{
	struct data_iterator iterator;
	boolean players[MAXIMUM_SCRIPT_PLAYERS] = { FALSE };
	long local_player;
	short index;

	if (!mutator_scripts.running)
		return;
	/* (what this machine's first player holds, as the others' came in the
	network's messages, and none for a player who is gone) */
	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
	{
		index = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		if (VALID_INDEX(index, MAXIMUM_SCRIPT_PLAYERS))
			players[index] = TRUE;
	}
	for (index = 0; index < MAXIMUM_SCRIPT_PLAYERS; index++)
	{
		if (!players[index])
			mutator_scripts.player_buttons[index] = 0;
	}
	local_player = local_player_get_player_index(0);
	if (local_player != NONE && (short)local_player < MAXIMUM_SCRIPT_PLAYERS)
		mutator_scripts.player_buttons[(short)local_player] = mutator_scripts.local_buttons;
	mutator_scripts.ticking = TRUE;
	for (index = 0; index < MAXIMUM_MUTATOR_SCRIPTS; index++)
	{
		struct mutator_script *script = &mutator_scripts.scripts[index];

		if (!script->state || !script->kept || script->failed)
			continue;
		if (!mutator_hs_program_call(script->state, "tick", MAXIMUM_SCRIPT_INSTRUCTIONS, &script->result) ||
			!script_players_call(script, "player_tick"))
			script_failed(script, "failed");
	}
	mutator_scripts.ticking = FALSE;
}

byte mutator_scripts_local_buttons(
	void)
{
	return mutator_scripts.running ? mutator_scripts.local_buttons : 0;
}

byte mutator_scripts_player_buttons(
	short player_index)
{
	return mutator_scripts.running && player_index >= 0 && player_index < MAXIMUM_SCRIPT_PLAYERS ?
		mutator_scripts.player_buttons[player_index] : 0;
}

void mutator_scripts_note_buttons(
	short player_index,
	byte buttons)
{
	if (player_index >= 0 && player_index < MAXIMUM_SCRIPT_PLAYERS)
		mutator_scripts.player_buttons[player_index] = buttons;
}

void mutator_scripts_render(
	short local_player_index,
	real_point3d const *camera_position,
	real_vector3d const *camera_forward,
	real_vector3d const *camera_up)
{
	real_vector3d right;
	short index;

	if (!mutator_scripts.line_count)
		return;
	cross_product3d(camera_forward, camera_up, &right);
	for (index = 0; index < mutator_scripts.line_count; index++)
	{
		struct mutator_script_line const *line = &mutator_scripts.lines[index];
		real_point3d from;

		if (line->from_view)
		{
			/* (its camera's: not another player's window) */
			if (local_player_index != 0)
				continue;
			from.x = camera_position->x + right.i * line->from[0] - camera_up->i * line->from[1] +
				camera_forward->i * line->from[2];
			from.y = camera_position->y + right.j * line->from[0] - camera_up->j * line->from[1] +
				camera_forward->j * line->from[2];
			from.z = camera_position->z + right.k * line->from[0] - camera_up->k * line->from[1] +
				camera_forward->k * line->from[2];
		}
		else
		{
			from.x = line->from[0];
			from.y = line->from[1];
			from.z = line->from[2];
		}
		rasterizer_debug_line(&from, &line->to, &line->color);
	}
}

void mutator_scripts_render_begin(
	short local_player_index,
	real_point3d const *camera_position,
	real_vector3d const *camera_forward,
	real_vector3d const *camera_up)
{
	real_vector3d right, left;
	short index;

	if (!mutator_scripts_model_count || local_player_index != 0)
		return;
	cross_product3d(camera_forward, camera_up, &right);
	cross_product3d(camera_up, camera_forward, &left);
	for (index = 0; index < MAXIMUM_SCRIPT_MODELS; index++)
	{
		struct mutator_script_model *model = &mutator_scripts.models[index];
		struct object_datum *object = model->used ? object_try_and_get(model->object_index) : NULL;
		real_matrix4x3 *nodes;

		if (!object || (model->view_node == NONE && !model->face_view))
			continue;
		nodes = object_get_node_matrices(model->object_index);
		if (model->face_view)
		{
			short count = script_model_node_count(model->object_index);
			short node;

			for (node = 0; node < count; node++)
			{
				nodes[node].forward = *camera_forward;
				nodes[node].left = left;
				nodes[node].up = *camera_up;
			}
		}
		if (model->view_node != NONE)
		{
			real_point3d *point = &nodes[model->view_node].position;

			point->x = camera_position->x + right.i * model->view_from[0] - camera_up->i * model->view_from[1] +
				camera_forward->i * model->view_from[2];
			point->y = camera_position->y + right.j * model->view_from[0] - camera_up->j * model->view_from[1] +
				camera_forward->j * model->view_from[2];
			point->z = camera_position->z + right.k * model->view_from[0] - camera_up->k * model->view_from[1] +
				camera_forward->k * model->view_from[2];
		}
		script_model_bound(model->object_index);
	}
}

boolean mutator_scripts_object_local_found(
	long object_index)
{
	return script_model_object(object_index);
}

boolean mutator_scripts_creating_model(
	void)
{
	return mutator_scripts.creating_model;
}

void mutator_scripts_before_save(
	void)
{
	if (mutator_scripts_model_count)
		script_models_delete(NONE);
}

void mutator_scripts_after_load(
	void)
{
	short index;

	/* (the objects are the saved game's now: none of them a script's) */
	for (index = 0; index < MAXIMUM_SCRIPT_MODELS; index++)
	{
		if (mutator_scripts.models[index].used)
			script_model_forget(&mutator_scripts.models[index]);
	}
}

void mutator_scripts_settings_changed(
	void)
{
	/* Getters read current settings at each call. Program-owned string copies
	 * remain valid until the program closes. */
}

boolean mutator_view_active(
	short local_player_index)
{
	return local_player_index == 0 && mutator_scripts.view_active;
}

boolean mutator_view_backward(
	short local_player_index)
{
	return mutator_view_active(local_player_index) && mutator_scripts.view_backward;
}

boolean mutator_view_crosshair(
	short local_player_index)
{
	return !mutator_view_active(local_player_index) || mutator_scripts.view_crosshair;
}

real mutator_view_distance(
	short local_player_index)
{
	(void)local_player_index;
	return mutator_scripts.view_distance;
}

real mutator_view_height(
	short local_player_index)
{
	(void)local_player_index;
	return mutator_scripts.view_height;
}

boolean mutator_scripts_control_claimed(
	char const *name)
{
	short index;

	for (index = 0; index < mutator_scripts.claim_count; index++)
	{
		if (!_stricmp(mutator_scripts.claims[index], name))
			return TRUE;
	}
	return FALSE;
}
