/*
MUTATOR_SCRIPTS.C

A mutator's Lua script (docs/mutators.md, "Scripts"), which runs on the machine
that has it: a frame() called once a frame for the first local player, and a
tick() called once a game tick. A frame can read the game and its own machine's
controls, ask for a camera view and draw lines. A tick moves what the game moves:
each machine's copy of a player, which the netcode's corrections keep together as
they do any prediction, and on the host, which decides, any other object. What a
script has of another machine is the buttons it sets (input.button), sent with
its player's input.

A script is untrusted, as a map is. It gets a state of its own, with Lua's base,
string, table and math libraries, without what loads code or reaches files, a
limit on its memory and on the instructions of each call. One that breaks a limit
or fails is said in debug.txt and stopped for this run of the game, and the
view is then first person. With no script on, nothing here does anything.
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

/* (Lua is built as C11, with a 64-bit integer: gnu89's <limits.h> does not say so) */
#include <limits.h>
#ifndef LLONG_MAX
#define LLONG_MAX __LONG_LONG_MAX__
#define LLONG_MIN (-LLONG_MAX - 1LL)
#define ULLONG_MAX (LLONG_MAX * 2ULL + 1ULL)
#endif

#include "../../third_party/lua/lua.h"
#include "../../third_party/lua/lauxlib.h"
#include "../../third_party/lua/lualib.h"

#include <stdlib.h>
#include <string.h>

/* ---------- constants */

enum
{
	MAXIMUM_MUTATOR_SCRIPTS = 8,
	SCRIPT_KEY_LENGTH = 32,
	/* a script's memory, and the instructions of a call */
	MAXIMUM_SCRIPT_BYTES = 0x400000,
	MAXIMUM_SCRIPT_INSTRUCTIONS = 2000000,
	SCRIPT_HOOK_INSTRUCTIONS = 10000,
	MAXIMUM_SCRIPT_BUTTONS = 8,
	MAXIMUM_SCRIPT_PLAYERS = 16,
	MAXIMUM_SCRIPT_LINES = 64,
	MAXIMUM_SCRIPT_MODELS = 16,
	/* the controls a frame's scripts claim, and a control's name's most bytes */
	MAXIMUM_SCRIPT_CLAIMS = 8,
	SCRIPT_CLAIM_LENGTH = 32,
};

/* how fast a script can send an object, a tick on each axis (world units) */
#define MAXIMUM_SCRIPT_VELOCITY 10.0f
/* how far a ray goes (world units) */
#define MAXIMUM_SCRIPT_RAY 1000.0f

/* ---------- structures */

struct mutator_script
{
	char key[SCRIPT_KEY_LENGTH];
	lua_State *state;
	unsigned long bytes;
	long instructions;
	boolean kept;
	boolean failed;
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

/* a script's allocations, within its limit */
static void *script_allocate(
	void *context,
	void *block,
	size_t old_size,
	size_t new_size)
{
	struct mutator_script *script = context;

	if (!block)
		old_size = 0;
	if (!new_size)
	{
		script->bytes -= (unsigned long)old_size;
		if (block)
			free(block);
		return NULL;
	}
	if (new_size > old_size && new_size - old_size > MAXIMUM_SCRIPT_BYTES - script->bytes)
		return NULL;
	{
		void *grown = malloc(new_size);

		if (!grown)
			return NULL;
		if (block)
		{
			memcpy(grown, block, old_size < new_size ? old_size : new_size);
			free(block);
		}
		script->bytes += (unsigned long)new_size - (unsigned long)old_size;
		return grown;
	}
}

static void script_count_hook(
	lua_State *state,
	lua_Debug *debug)
{
	struct mutator_script *script = *(struct mutator_script **)lua_getextraspace(state);

	(void)debug;
	script->instructions += SCRIPT_HOOK_INSTRUCTIONS;
	if (script->instructions > MAXIMUM_SCRIPT_INSTRUCTIONS)
		luaL_error(state, "it ran too long in one frame");
}

static int script_input_held(
	lua_State *state)
{
	/* (one name, or two as a Controls Setup row has them: "F5, G") */
	char name[64];
	char const *text = luaL_checkstring(state, 1);
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
	lua_pushboolean(state, held);
	return 1;
}

static int script_input_claim(
	lua_State *state)
{
	char const *name = luaL_checkstring(state, 1);

	if (mutator_scripts.claim_count < MAXIMUM_SCRIPT_CLAIMS && strlen(name) < SCRIPT_CLAIM_LENGTH)
		csstrcpy(mutator_scripts.claims[mutator_scripts.claim_count++], name);
	return 0;
}

/* input.button(n, down): one of the first local player's buttons, which go with
their input to the host and from it to every machine, for player.button */
static int script_input_button(
	lua_State *state)
{
	lua_Integer number = luaL_checkinteger(state, 1);

	luaL_argcheck(state, number >= 0 && number < MAXIMUM_SCRIPT_BUTTONS, 1, "no such button");
	SET_FLAG(mutator_scripts.local_buttons, (long)number, lua_toboolean(state, 2) != 0);
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
	lua_State *state,
	short bit)
{
	struct unit_datum *unit = script_player_unit();

	lua_pushboolean(state, unit && TEST_FLAG(unit->unit.control_flags, bit));
	return 1;
}

static int script_input_jump(
	lua_State *state)
{
	return script_input_control(state, _unit_control_jump_bit);
}

static int script_input_crouch(
	lua_State *state)
{
	return script_input_control(state, _unit_control_crouch_modifier_bit);
}

static int script_player_alive(
	lua_State *state)
{
	struct unit_datum *unit = script_player_unit();

	lua_pushboolean(state, unit && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit));
	return 1;
}

static int script_player_in_vehicle(
	lua_State *state)
{
	struct unit_datum *unit = script_player_unit();

	lua_pushboolean(state, unit && unit->object.parent_object_index != NONE);
	return 1;
}

/* a player's datum from a script's index of it, NULL if it has none */
static struct player_datum *script_player_at(
	lua_State *state,
	int argument)
{
	lua_Integer index = luaL_checkinteger(state, argument);
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
	lua_State *state,
	int argument)
{
	struct player_datum *player = script_player_at(state, argument);

	return player && player->unit_index != NONE ? unit_try_and_get(player->unit_index) : NULL;
}

static int script_game_time(
	lua_State *state)
{
	lua_pushinteger(state, game_time_get());
	return 1;
}

static int script_game_host(
	lua_State *state)
{
	lua_pushboolean(state, !network_game_distributed_client());
	return 1;
}

static int script_game_players(
	lua_State *state)
{
	struct data_iterator iterator;
	lua_Integer count = 0;

	lua_newtable(state);
	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
	{
		lua_pushinteger(state, (short)iterator.datum_index);
		lua_rawseti(state, -2, ++count);
	}
	return 1;
}

static int script_player_local(
	lua_State *state)
{
	long player_index = local_player_get_player_index(mutator_scripts.local_player_index);

	if (player_index == NONE)
		lua_pushnil(state);
	else
		lua_pushinteger(state, (short)player_index);
	return 1;
}

static int script_player_button(
	lua_State *state)
{
	lua_Integer number = luaL_checkinteger(state, 2);
	lua_Integer index = luaL_checkinteger(state, 1);

	luaL_argcheck(state, number >= 0 && number < MAXIMUM_SCRIPT_BUTTONS, 2, "no such button");
	lua_pushboolean(state, index >= 0 && index < MAXIMUM_SCRIPT_PLAYERS &&
		TEST_FLAG(mutator_scripts.player_buttons[index], (long)number));
	return 1;
}

/* the player's unit, nil with none or dead */
static int script_player_unit_of(
	lua_State *state)
{
	struct player_datum *player = script_player_at(state, 1);
	struct unit_datum *unit = player && player->unit_index != NONE ? unit_try_and_get(player->unit_index) : NULL;

	if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		lua_pushnil(state);
	else
		lua_pushinteger(state, player->unit_index);
	return 1;
}

static int script_player_aim(
	lua_State *state)
{
	struct unit_datum *unit = script_player_at_unit(state, 1);

	if (!unit)
		return 0;
	lua_pushnumber(state, unit->unit.aiming_vector.i);
	lua_pushnumber(state, unit->unit.aiming_vector.j);
	lua_pushnumber(state, unit->unit.aiming_vector.k);
	return 3;
}

static int script_player_crouch(
	lua_State *state)
{
	struct player_datum *player = script_player_at(state, 1);
	struct biped_datum *biped = player && player->unit_index != NONE ?
		(struct biped_datum *)object_try_and_get_and_verify_type(player->unit_index, _object_mask_biped) : NULL;

	lua_pushnumber(state, biped ? biped->biped.crouch : 0.0f);
	return 1;
}

/* an object from a script's handle: it is a number the script could have made
up, and an identifier of none is one the data arrays assert on */
static struct object_datum *script_object_at(
	lua_State *state,
	int argument,
	long *object_index)
{
	lua_Integer handle = luaL_checkinteger(state, argument);
	long index = (long)handle;

	if ((lua_Integer)index != handle || index == NONE || !(index >> 16))
		return NULL;
	if (object_index)
		*object_index = index;
	return object_try_and_get(index);
}

static int script_object_position(
	lua_State *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object)
		return 0;
	lua_pushnumber(state, object->object.position.x);
	lua_pushnumber(state, object->object.position.y);
	lua_pushnumber(state, object->object.position.z);
	return 3;
}

static int script_object_velocity(
	lua_State *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object)
		return 0;
	lua_pushnumber(state, object->object.translational_velocity.i);
	lua_pushnumber(state, object->object.translational_velocity.j);
	lua_pushnumber(state, object->object.translational_velocity.k);
	return 3;
}

static int script_object_parent(
	lua_State *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object || object->object.parent_object_index == NONE)
		lua_pushnil(state);
	else
		lua_pushinteger(state, object->object.parent_object_index);
	return 1;
}

static int script_object_kind(
	lua_State *state)
{
	static char const *const kinds[] =
	{
		"biped", "vehicle", "weapon", "equipment", "garbage", "projectile", "scenery", "machine", "control",
		"light_fixture", "placeholder", "sound_scenery",
	};
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object || object->object.type < 0 || object->object.type >= NUMBEROF(kinds))
		lua_pushnil(state);
	else
		lua_pushstring(state, kinds[object->object.type]);
	return 1;
}

static int script_object_tag(
	lua_State *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	if (!object)
		lua_pushnil(state);
	else
		lua_pushstring(state, tag_get_name(object->definition_index));
	return 1;
}

static int script_object_alive(
	lua_State *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	lua_pushboolean(state, object && !TEST_FLAG(object->object.damage_flags, _object_dead_bit));
	return 1;
}

static int script_object_acceleration_scale(
	lua_State *state)
{
	struct object_datum *object = script_object_at(state, 1, NULL);

	lua_pushnumber(state, object ? object_definition_get(object->definition_index)->object.acceleration_scale :
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
	lua_State *state,
	int argument)
{
	real value = (real)luaL_checknumber(state, argument);

	/* (written so that a NaN is none) */
	if (!(value >= -MAXIMUM_SCRIPT_VELOCITY))
		return value < 0.0f ? -MAXIMUM_SCRIPT_VELOCITY : 0.0f;
	return value > MAXIMUM_SCRIPT_VELOCITY ? MAXIMUM_SCRIPT_VELOCITY : value;
}

static int script_object_set_velocity(
	lua_State *state)
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
	lua_State *state)
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
	lua_State *state,
	int argument)
{
	lua_Number value = luaL_checknumber(state, argument);

	luaL_argcheck(state, value >= -1.0e6 && value <= 1.0e6, argument, "out of range");
	return (real)value;
}

/* world.ray(x, y, z, dx, dy, dz, ignore): where the vector from the point first
hits the world or an object, and the object: x, y, z, object or nil. Nothing with
no hit. */
static int script_world_ray(
	lua_State *state)
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
	luaL_argcheck(state, magnitude3d(&vector) <= MAXIMUM_SCRIPT_RAY, 4, "too long");
	if (!lua_isnoneornil(state, 7) && !script_object_at(state, 7, &ignore))
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
	lua_pushnumber(state, collision.point.x);
	lua_pushnumber(state, collision.point.y);
	lua_pushnumber(state, collision.point.z);
	if (collision.type != _collision_result_object)
		lua_pushnil(state);
	else
		lua_pushinteger(state, collision.object_index);
	return 4;
}

static void script_line_color(
	lua_State *state,
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
	lua_State *state)
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
	lua_State *state)
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
	lua_State *state)
{
	struct unit_datum *unit = script_player_unit();

	lua_pushboolean(state, unit && unit->object.parent_object_index == NONE && !cinematic_in_progress() &&
		!(director_camera_scripted && *director_camera_scripted) &&
		!mutator_view_active(mutator_scripts.local_player_index));
	return 1;
}

static int script_camera_free(
	lua_State *state)
{
	lua_pushboolean(state, !cinematic_in_progress() && !(director_camera_scripted && *director_camera_scripted));
	return 1;
}

static real script_view_number(
	lua_State *state,
	char const *name,
	real minimum,
	real maximum,
	real default_value)
{
	real value = default_value;

	if (lua_getfield(state, 1, name) != LUA_TNIL)
		value = (real)luaL_checknumber(state, -1);
	lua_pop(state, 1);
	/* (written so that a NaN is the least) */
	if (!(value >= minimum))
		value = minimum;
	return value > maximum ? maximum : value;
}

static boolean script_view_boolean(
	lua_State *state,
	char const *name,
	boolean default_value)
{
	boolean value = default_value;

	if (lua_getfield(state, 1, name) != LUA_TNIL)
		value = lua_toboolean(state, -1) != 0;
	lua_pop(state, 1);
	return value;
}

/* camera.view{ distance, height, backward, crosshair }: for this frame only */
static int script_camera_view(
	lua_State *state)
{
	luaL_checktype(state, 1, LUA_TTABLE);
	mutator_scripts.view_distance = script_view_number(state, "distance", 0.6f, 20.f, 2.5f);
	mutator_scripts.view_height = script_view_number(state, "height", -2.f, 5.f, 0.f);
	mutator_scripts.view_backward = script_view_boolean(state, "backward", FALSE);
	mutator_scripts.view_crosshair = script_view_boolean(state, "crosshair", TRUE);
	mutator_scripts.view_active = TRUE;
	return 0;
}

/* ---------- models */

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
	lua_State *state,
	int argument)
{
	struct mutator_script *script = *(struct mutator_script **)lua_getextraspace(state);
	short script_index = (short)(script - mutator_scripts.scripts);
	lua_Integer handle = luaL_checkinteger(state, argument);
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
	lua_State *state,
	int argument,
	real_vector3d *vector)
{
	vector->i = script_finite_argument(state, argument);
	vector->j = script_finite_argument(state, argument + 1);
	vector->k = script_finite_argument(state, argument + 2);
	return normalize3d(vector) > 0.0001f;
}

/* model.create(tag, x, y, z, owner): a weapon's, a piece of equipment's,
garbage's, a projectile's or scenery's model at the point, its handle or nil.
Its object's creation effect plays, and its attachments are made. The owner, a
unit, is whose it is (what a projectile of no one's is to a unit nearby) */
static int script_model_create(
	lua_State *state)
{
	static long const groups[] = { 'weap', 'eqip', 'garb', 'proj', 'scen' };
	struct mutator_script *script = *(struct mutator_script **)lua_getextraspace(state);
	char const *name = luaL_checkstring(state, 1);
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
		return 0;
	if (lua_isnoneornil(state, 5) || !script_object_at(state, 5, &owner) ||
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
		return 0;
	{
		struct object_header_datum *header = object_header_get(object_index);
		struct object_datum *object = object_get(object_index);

		/* (never updated: it stays as the script puts it, and does nothing
		itself; never hit: it is in the clusters' lists of what is not) */
		SET_FLAG(object->object.flags, _object_cannot_be_activated_bit, TRUE);
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
	model->view_node = NONE;
	mutator_scripts_model_count++;
	lua_pushinteger(state, object_index);
	return 1;
}

static int script_model_delete_function(
	lua_State *state)
{
	struct mutator_script_model *model = script_model_at(state, 1);

	if (model)
		script_model_delete(model);
	return 0;
}

static int script_model_exists(
	lua_State *state)
{
	lua_pushboolean(state, script_model_at(state, 1) != NULL);
	return 1;
}

/* model.place(handle, x, y, z, forward x, y, z, up x, y, z): where it is and
which way it faces (else as it was), its nodes as its model has them */
static int script_model_place(
	lua_State *state)
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
	faces = !lua_isnoneornil(state, 5) && script_unit_vector_argument(state, 5, &forward);
	upright = faces && !lua_isnoneornil(state, 8) && script_unit_vector_argument(state, 8, &up);
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
	return 0;
}

static short script_model_node_argument(
	lua_State *state,
	struct mutator_script_model const *model,
	int argument)
{
	lua_Integer node = luaL_checkinteger(state, argument);

	return node >= 0 && node < script_model_node_count(model->object_index) ? (short)node : NONE;
}

/* model.node(handle, node, x, y, z): one of its nodes moved there (until it is
placed again), which draws its vertices of that node there: a rope between two */
static int script_model_node(
	lua_State *state)
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
	}
	script_model_bound(model->object_index);
	return 0;
}

/* model.node_from_view(handle, node, right, down, forward): the same, this
frame, from a place by the camera as it is drawn (draw.line_from_view's) */
static int script_model_node_from_view(
	lua_State *state)
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
	lua_State *state)
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

static void script_register(
	lua_State *state,
	char const *table,
	luaL_Reg const *functions)
{
	lua_newtable(state);
	luaL_setfuncs(state, functions, 0);
	lua_setglobal(state, table);
}

static void script_setting_add(
	void *context,
	char const *name,
	char const *text,
	double number,
	int boolean_value,
	int kind)
{
	lua_State *state = context;

	/* (port_mutators.c's: 0 text, 1 a number, 2 true or false) */
	if (kind == 1)
		lua_pushnumber(state, number);
	else if (kind == 2)
		lua_pushboolean(state, boolean_value);
	else
		lua_pushstring(state, text);
	lua_setfield(state, -2, name);
}

/* protected: its settings read again, into the table it has */
static int script_settings_read(
	lua_State *state)
{
	struct mutator_script *script = lua_touserdata(state, 1);

	lua_getglobal(state, "settings");
	if (lua_istable(state, -1))
		port_mutators_script_settings(script->key, script_setting_add, state);
	return 0;
}

/* protected: the libraries, without what loads code, then the script run */
static int script_open(
	lua_State *state)
{
	static luaL_Reg const input_functions[] =
	{
		{ "held", script_input_held },
		{ "claim", script_input_claim },
		{ "button", script_input_button },
		{ "jump", script_input_jump },
		{ "crouch", script_input_crouch },
		{ NULL, NULL },
	};
	static luaL_Reg const player_functions[] =
	{
		{ "alive", script_player_alive },
		{ "in_vehicle", script_player_in_vehicle },
		{ "local_index", script_player_local },
		{ "button", script_player_button },
		{ "unit", script_player_unit_of },
		{ "aim", script_player_aim },
		{ "crouch", script_player_crouch },
		{ NULL, NULL },
	};
	static luaL_Reg const camera_functions[] =
	{
		{ "free", script_camera_free },
		{ "view", script_camera_view },
		{ "first_person", script_camera_first_person },
		{ NULL, NULL },
	};
	static luaL_Reg const game_functions[] =
	{
		{ "time", script_game_time },
		{ "host", script_game_host },
		{ "players", script_game_players },
		{ NULL, NULL },
	};
	static luaL_Reg const object_functions[] =
	{
		{ "position", script_object_position },
		{ "velocity", script_object_velocity },
		{ "parent", script_object_parent },
		{ "kind", script_object_kind },
		{ "tag", script_object_tag },
		{ "alive", script_object_alive },
		{ "acceleration_scale", script_object_acceleration_scale },
		{ "set_velocity", script_object_set_velocity },
		{ "turn_around", script_object_turn_around },
		{ NULL, NULL },
	};
	static luaL_Reg const world_functions[] =
	{
		{ "ray", script_world_ray },
		{ NULL, NULL },
	};
	static luaL_Reg const model_functions[] =
	{
		{ "create", script_model_create },
		{ "delete", script_model_delete_function },
		{ "exists", script_model_exists },
		{ "place", script_model_place },
		{ "node", script_model_node },
		{ "node_from_view", script_model_node_from_view },
		{ "face_view", script_model_face_view },
		{ NULL, NULL },
	};
	static luaL_Reg const draw_functions[] =
	{
		{ "line", script_draw_line },
		{ "line_from_view", script_draw_line_from_view },
		{ NULL, NULL },
	};
	static char const *const removed[] = { "dofile", "loadfile", "load", "collectgarbage", "print", "warn" };
	struct mutator_script *script = lua_touserdata(state, 1);
	unsigned long size = 0;
	char *text;
	short index;
	int status;

	luaL_requiref(state, LUA_GNAME, luaopen_base, 1);
	luaL_requiref(state, LUA_STRLIBNAME, luaopen_string, 1);
	luaL_requiref(state, LUA_TABLIBNAME, luaopen_table, 1);
	luaL_requiref(state, LUA_MATHLIBNAME, luaopen_math, 1);
	lua_settop(state, 1);
	for (index = 0; index < NUMBEROF(removed); index++)
	{
		lua_pushnil(state);
		lua_setglobal(state, removed[index]);
	}
	lua_getglobal(state, LUA_STRLIBNAME);
	lua_pushnil(state);
	lua_setfield(state, -2, "dump");
	lua_pop(state, 1);
	script_register(state, "input", input_functions);
	script_register(state, "player", player_functions);
	script_register(state, "camera", camera_functions);
	script_register(state, "game", game_functions);
	script_register(state, "object", object_functions);
	script_register(state, "world", world_functions);
	script_register(state, "draw", draw_functions);
	script_register(state, "model", model_functions);
	lua_newtable(state);
	port_mutators_script_settings(script->key, script_setting_add, state);
	lua_setglobal(state, "settings");

	text = port_mutators_script_read(script->key, &size);
	if (!text)
		return luaL_error(state, "this machine has no mutator with this script");
	/* (text only: not Lua's compiled form, which it does not check) */
	status = luaL_loadbufferx(state, text, size, script->key, "t");
	port_mutators_script_free(text);
	if (status != LUA_OK)
		return lua_error(state);
	lua_call(state, 0, 0);

	return 0;
}

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
	script_models_delete((short)(script - mutator_scripts.scripts));
	if (script->state)
		lua_close(script->state);
	csmemset(script, 0, sizeof(*script));
	scripts_count();
}

static void script_failed(
	struct mutator_script *script,
	char const *when)
{
	char const *message = lua_tostring(script->state, -1);

	error(_error_silent, "mutator scripts: %s %s, and is stopped: %s", script->key, when, message ? message : "no reason");
	lua_settop(script->state, 0);
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
	script->state = lua_newstate(script_allocate, script);
	if (!script->state)
	{
		script_close(script);
		return;
	}
	*(struct mutator_script **)lua_getextraspace(script->state) = script;
	lua_sethook(script->state, script_count_hook, LUA_MASKCOUNT, SCRIPT_HOOK_INSTRUCTIONS);
	lua_pushcfunction(script->state, script_open);
	lua_pushlightuserdata(script->state, script);
	if (lua_pcall(script->state, 1, 0, 0) != LUA_OK)
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
		if (lua_getglobal(script->state, "frame") != LUA_TFUNCTION)
		{
			lua_settop(script->state, 0);
			continue;
		}
		script->instructions = 0;
		if (lua_pcall(script->state, 0, 0, 0) != LUA_OK)
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
	unsigned long players = 0;
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
			players |= 1UL << index;
	}
	for (index = 0; index < MAXIMUM_SCRIPT_PLAYERS; index++)
	{
		if (!(players & (1UL << index)))
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
		if (lua_getglobal(script->state, "tick") != LUA_TFUNCTION)
		{
			lua_settop(script->state, 0);
			continue;
		}
		script->instructions = 0;
		if (lua_pcall(script->state, 0, 0, 0) != LUA_OK)
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
	short index;

	for (index = 0; index < MAXIMUM_MUTATOR_SCRIPTS; index++)
	{
		struct mutator_script *script = &mutator_scripts.scripts[index];

		if (!script->state || script->failed)
			continue;
		lua_pushcfunction(script->state, script_settings_read);
		lua_pushlightuserdata(script->state, script);
		script->instructions = 0;
		if (lua_pcall(script->state, 1, 0, 0) != LUA_OK)
			script_failed(script, "cannot read its settings");
	}
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
