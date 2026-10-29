/*
FLOODFECTION_FORMS.C

Floodfection's zombies as the Flood (infection.c): each living zombie's
Spartan is drawn as a Flood combat form, a human's or an elite's (one or
the other for each body, the same on every machine: by the unit's index,
which a network game's machines share), running, standing, jumping,
landing and striking as the Spartan does. Its own player still sees their
Spartan's arms and the skull in first person. A zombie's body is not drawn
once it dies (Floodfection's burst plays there: infection.c).

Only the drawing is changed, as first_person_legs.c changes it: the Spartan
is the game's unit as ever, its hit boxes, markers and physics the
Spartan's, so the game plays alike on every machine and with a build
without this. The forms' models and animations are Two Betrayals' (c40),
copied into the level as it loads (infection_tag_import, from
opence_floodfection.pack); a level without them draws its zombies as
Spartans.

Each tick picks a form's animation by what its Spartan does and plays it
(its frames at the Spartan's pace when running; the form turned to run the
way it goes, as the Flood only runs forward, but for the human form's run
backward); a frame draws it between its last two ticks, as the game's
objects are (render_interpolation.c), blending into a new animation over
a few ticks.
*/

#include "cseries.h"
#include "math/real_math.h"
#include "game/game.h"
#include "game/players.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "models/models.h"
#include "objects/objects.h"
#include "tag_files/tag_groups.h"
#include "units/bipeds.h"
#include "units/units.h"

#include "halo_port_limits.h"
#include "floodfection_forms.h"

#include <math.h>
#include <string.h>

/* render_interpolation.c's */
float render_interpolation_fraction(void);
void render_interpolation_object_point(long object_index, union real_point3d *point);

/* the forms (Two Betrayals' tags) */
enum
{
	_form_human = 0,
	_form_elite,
	NUMBER_OF_FORMS
};

static char const *const form_tag_names[NUMBER_OF_FORMS] =
{
	"characters\\floodcombat_human\\floodcombat_human",
	"characters\\floodcombat elite\\floodcombat elite",
};

/* what a form does, and its animation of it (the first found of the names:
the forms' graphs have no strafing, and the elite's no run backward) */
enum
{
	_form_action_idle = 0,
	_form_action_run,
	_form_action_run_back,
	_form_action_crouch_run,
	_form_action_airborne,
	_form_action_land_soft,
	_form_action_land_hard,
	_form_action_melee,
	NUMBER_OF_FORM_ACTIONS
};

static char const *const form_animation_names[NUMBER_OF_FORMS][NUMBER_OF_FORM_ACTIONS][2] =
{
	{
		{ "stand unarmed idle%0", "stand pistol idle%0" },
		{ "stand pistol move-front", NULL },
		{ "stand pistol move-back", NULL },
		{ "crouch pistol move-front", "stand pistol move-front" },
		{ "stand pistol airborne", NULL },
		{ "stand unarmed land-soft", "stand pistol land-soft" },
		{ "stand unarmed land-hard", "stand pistol land-hard" },
		{ "stand unarmed melee", "stand pistol melee" },
	},
	{
		{ "stand unarmed idle%0", "stand pistol idle" },
		{ "stand pistol move-front%0", "alert unarmed move-front" },
		{ NULL, NULL },
		{ "crouch unarmed move-front", "crouch pistol move-front" },
		{ "stand pistol airborne", NULL },
		{ "stand pistol land-hard", NULL },
		{ "stand pistol land-hard", NULL },
		{ "stand pistol melee%0", "stand pistol melee%1" },
	},
};

/* a Spartan this fast (world units a tick: its run) plays a run's frames
one a tick; slower or faster, its frames as much slower or faster (within
these) */
#define FORM_RUN_SPEED 0.075f
#define FORM_RUN_RATE_MINIMUM 0.5f
#define FORM_RUN_RATE_MAXIMUM 1.6f
/* slower than this (world units a tick) stands */
#define FORM_MOVING_SPEED 0.015f
/* how far a form turns in a tick at most (radians), and how many ticks it
takes to blend into a new animation */
#define FORM_TURN_RATE 0.35f
#define FORM_BLEND_TICKS 6
/* the zombies' bodies not drawn (the oldest forgotten past it) */
#define FORM_HIDDEN_BODIES 64

struct form_state
{
	/* the Spartan drawn as this form (NONE: none) */
	long unit_index;
	short form;
	short action;
	short animation_index;
	/* the frame now and at the tick before (a loop's past its end once
	wrapped: drawn wrapped) */
	real frame;
	real previous_frame;
	/* the animation blended from, frozen at its frame, and the ticks of
	the blend left */
	short from_animation_index;
	real from_frame;
	short blend_ticks;
	/* which way it faces, now and at the tick before (radians) */
	real yaw;
	real previous_yaw;
};

static struct
{
	/* each form's model and animation graph (NONE: the level has not) */
	long model_index[NUMBER_OF_FORMS];
	long animation_graph_index[NUMBER_OF_FORMS];
	short animation_index[NUMBER_OF_FORMS][NUMBER_OF_FORM_ACTIONS];
	/* by absolute player index */
	struct form_state players[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	real_matrix4x3 matrices[HALO_PORT_MAXIMUM_NETWORK_PLAYERS][MAXIMUM_NODES_PER_MODEL];
	long hidden[FORM_HIDDEN_BODIES];
	long hidden_next;
} forms;

static byte const form_region_permutations[MAXIMUM_REGIONS_PER_OBJECT];

/* ---- the level */

static short form_find_animation(
	long animation_graph_index,
	char const *const names[2])
{
	short index;

	for (index = 0; index < 2; index++)
	{
		short animation_index;

		if (!names[index])
			continue;
		animation_index = animation_graph_get_animation_by_name(animation_graph_index, names[index]);
		if (animation_index != NONE)
			return animation_index;
	}
	return NONE;
}

void floodfection_forms_reset(
	void)
{
	short form;
	long slot;

	for (form = 0; form < NUMBER_OF_FORMS; form++)
	{
		short action;
		long model_index = tag_loaded('mode', form_tag_names[form]);
		long animation_graph_index = tag_loaded('antr', form_tag_names[form]);

		forms.model_index[form] = NONE;
		forms.animation_graph_index[form] = NONE;
		if (model_index == NONE || animation_graph_index == NONE ||
			model_definition_get(model_index)->nodes.count > MAXIMUM_NODES_PER_MODEL)
		{
			continue;
		}
		for (action = 0; action < NUMBER_OF_FORM_ACTIONS; action++)
		{
			forms.animation_index[form][action] = form_find_animation(animation_graph_index,
				form_animation_names[form][action]);
		}
		/* (a form that cannot stand is not one) */
		if (forms.animation_index[form][_form_action_idle] == NONE)
			continue;
		for (action = 0; action < NUMBER_OF_FORM_ACTIONS; action++)
		{
			if (forms.animation_index[form][action] == NONE)
			{
				forms.animation_index[form][action] = action == _form_action_run_back ?
					forms.animation_index[form][_form_action_run] : forms.animation_index[form][_form_action_idle];
			}
		}
		forms.model_index[form] = model_index;
		forms.animation_graph_index[form] = animation_graph_index;
	}
	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; slot++)
		forms.players[slot].unit_index = NONE;
	for (slot = 0; slot < FORM_HIDDEN_BODIES; slot++)
		forms.hidden[slot] = NONE;
	forms.hidden_next = 0;
}

/* ---- each tick */

static struct animation const *form_animation(
	short form,
	short animation_index)
{
	struct animation_graph *graph = animation_graph_definition_get(forms.animation_graph_index[form]);

	return TAG_BLOCK_GET_ELEMENT(&graph->animations, animation_index, struct animation);
}

static boolean form_action_loops(
	short action)
{
	return action != _form_action_land_soft && action != _form_action_land_hard && action != _form_action_melee;
}

static real form_angle_difference(
	real from,
	real to)
{
	real difference = fmodf(to - from, 2.0f * _pi);

	if (difference > _pi)
		difference -= 2.0f * _pi;
	else if (difference < -_pi)
		difference += 2.0f * _pi;
	return difference;
}

static void form_hide_body(
	long unit_index)
{
	long index;

	for (index = 0; index < FORM_HIDDEN_BODIES; index++)
	{
		if (forms.hidden[index] == unit_index)
			return;
	}
	forms.hidden[forms.hidden_next] = unit_index;
	forms.hidden_next = (forms.hidden_next + 1) % FORM_HIDDEN_BODIES;
}

/* what the Spartan does, and which way its form runs */
static short form_choose_action(
	struct form_state const *state,
	struct biped_datum *biped,
	real *target_yaw)
{
	struct unit_datum *unit = (struct unit_datum *)biped;
	real_vector3d const *velocity = &biped->object.translational_velocity;
	real speed = sqrtf(velocity->i * velocity->i + velocity->j * velocity->j);
	real facing = atan2f(biped->object.forward.j, biped->object.forward.i);
	char unit_state = unit->unit.animation.state;

	*target_yaw = facing;
	/* (a strike plays to its end, as the Spartan's) */
	if (unit_state == _unit_state_melee_attack || unit_state == _unit_state_melee_airborne ||
		unit->unit.animation.action == _unit_state_melee_attack ||
		unit->unit.animation.action == _unit_state_melee_airborne)
	{
		return _form_action_melee;
	}
	if (state->action == _form_action_melee &&
		state->frame < form_animation(state->form, state->animation_index)->frame_count - 1)
	{
		return _form_action_melee;
	}
	if (TEST_FLAG(biped->biped.flags, _biped_airborne_bit))
		return _form_action_airborne;
	if (unit_state == _unit_state_land_hard)
		return _form_action_land_hard;
	if (unit_state == _unit_state_land_soft)
		return _form_action_land_soft;
	if (speed < FORM_MOVING_SPEED)
		return _form_action_idle;
	if (unit->unit.animation.base_seat_index == _unit_base_seat_crouch)
	{
		*target_yaw = atan2f(velocity->j, velocity->i);
		return _form_action_crouch_run;
	}
	/* (backward, the human form runs backward; else it turns to run) */
	if ((velocity->i * biped->object.forward.i + velocity->j * biped->object.forward.j) < -0.7f * speed &&
		forms.animation_index[state->form][_form_action_run_back] !=
			forms.animation_index[state->form][_form_action_run])
	{
		return _form_action_run_back;
	}
	*target_yaw = atan2f(velocity->j, velocity->i);
	return _form_action_run;
}

void floodfection_forms_update(
	long player_index,
	boolean zombie)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	struct player_datum *player = player_get(player_index);
	struct form_state *state;
	struct biped_datum *biped;
	struct animation const *animation;
	real_vector3d const *velocity;
	real target_yaw;
	real rate = 1.0f;
	short action;
	short form;

	if (!VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		return;
	state = &forms.players[slot];
	/* (a zombie's body, dead or left behind, goes undrawn) */
	if (state->unit_index != NONE && (state->unit_index != player->unit_index || !zombie))
	{
		struct unit_datum *previous = unit_try_and_get(state->unit_index);

		if (previous && TEST_FLAG(previous->object.damage_flags, _object_dead_bit))
			form_hide_body(state->unit_index);
		state->unit_index = NONE;
	}
	biped = player->unit_index != NONE && !player->quit_out_of_game ? biped_try_and_get(player->unit_index) : NULL;
	if (!zombie || !biped || biped->object.parent_object_index != NONE)
		return;
	if (TEST_FLAG(biped->object.damage_flags, _object_dead_bit))
	{
		if (state->unit_index == player->unit_index)
			form_hide_body(state->unit_index);
		state->unit_index = NONE;
		return;
	}
	if (state->unit_index != player->unit_index)
	{
		/* (one form or the other for each body, alike everywhere; the other
		when the level has not this one) */
		form = (short)((((unsigned long)player->unit_index * 2654435761UL) >> 16) & 1);
		if (forms.model_index[form] == NONE)
			form ^= 1;
		if (forms.model_index[form] == NONE)
			return;
		csmemset(state, 0, sizeof(*state));
		state->unit_index = player->unit_index;
		state->form = form;
		state->action = _form_action_idle;
		state->animation_index = forms.animation_index[form][_form_action_idle];
		state->from_animation_index = NONE;
		state->yaw = atan2f(biped->object.forward.j, biped->object.forward.i);
		state->previous_yaw = state->yaw;
	}
	form = state->form;
	action = form_choose_action(state, biped, &target_yaw);
	state->previous_yaw = state->yaw;
	state->yaw += PIN(form_angle_difference(state->yaw, target_yaw), -FORM_TURN_RATE, FORM_TURN_RATE);
	if (forms.animation_index[form][action] != state->animation_index ||
		(action != state->action && !form_action_loops(action)))
	{
		state->from_animation_index = state->animation_index;
		state->from_frame = state->frame;
		state->blend_ticks = FORM_BLEND_TICKS;
		state->animation_index = forms.animation_index[form][action];
		state->frame = 0.0f;
		state->previous_frame = 0.0f;
	}
	state->action = action;
	animation = form_animation(form, state->animation_index);
	velocity = &biped->object.translational_velocity;
	if (action == _form_action_run || action == _form_action_run_back || action == _form_action_crouch_run)
	{
		rate = PIN(sqrtf(velocity->i * velocity->i + velocity->j * velocity->j) / FORM_RUN_SPEED,
			FORM_RUN_RATE_MINIMUM, FORM_RUN_RATE_MAXIMUM);
	}
	if (state->frame >= (real)animation->frame_count)
		state->frame -= (real)animation->frame_count;
	state->previous_frame = state->frame;
	state->frame += rate;
	if (!form_action_loops(action))
		state->frame = MIN(state->frame, (real)MAX(animation->frame_count - 1, 0));
	if (state->blend_ticks > 0)
		state->blend_ticks--;
}

/* ---- each frame */

/* an animation's pose at a frame between two of its own (wrapped, or held
at its last) */
static void form_pose(
	struct model const *model,
	struct animation const *animation,
	real frame,
	boolean loops,
	struct real_orientation *orientations)
{
	struct real_orientation next[MAXIMUM_NODES_PER_MODEL];
	short count = animation->frame_count;
	short first;
	short second;
	real fraction;

	if (count <= 0)
	{
		model_get_node_orientations(model, orientations);
		return;
	}
	if (loops)
		frame = fmodf(frame, (real)count);
	frame = PIN(frame, 0.0f, (real)(count - 1) + (loops ? 0.999f : 0.0f));
	first = (short)frame;
	second = loops ? (short)((first + 1) % count) : (short)MIN(first + 1, count - 1);
	fraction = frame - (real)first;
	animation_get_node_orientations(model, animation, first, orientations);
	if (second == first || fraction <= 0.0f)
		return;
	animation_get_node_orientations(model, animation, second, next);
	floodfection_forms_blend(model->nodes.count, orientations, next, fraction);
	csmemcpy(orientations, next, model->nodes.count * sizeof(*orientations));
}

/* the target the way from the original by the fraction (into the target) */
void floodfection_forms_blend(
	short node_count,
	struct real_orientation *original,
	struct real_orientation *target,
	real fraction)
{
	short step = (short)(fraction * 256.0f) - 1;

	if (step < 0)
	{
		csmemcpy(target, original, node_count * sizeof(*target));
		return;
	}
	if (step < 255)
		interpolate_node_orientations(node_count, original, target, step, 256);
}

short floodfection_forms_render(
	long object_index,
	long *model_index,
	real_matrix4x3 const **node_matrices,
	byte const **region_permutations)
{
	long slot;
	long index;

	if (object_index == NONE)
		return _floodfection_form_render_normal;
	for (index = 0; index < FORM_HIDDEN_BODIES; index++)
	{
		if (forms.hidden[index] == object_index)
		{
			struct object_datum *object = object_try_and_get(object_index);

			if (object && TEST_FLAG(object->object.damage_flags, _object_dead_bit))
				return _floodfection_form_render_hidden;
		}
	}
	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; slot++)
	{
		struct form_state *state = &forms.players[slot];
		struct real_orientation pose[MAXIMUM_NODES_PER_MODEL];
		struct model *model;
		struct object_datum *object;
		real_point3d origin;
		real_vector3d forward;
		real t;
		real yaw;
		real frame;

		if (state->unit_index != object_index)
			continue;
		object = object_try_and_get(object_index);
		if (!object || TEST_FLAG(object->object.damage_flags, _object_dead_bit))
			return _floodfection_form_render_normal;
		model = model_definition_get(forms.model_index[state->form]);
		t = render_interpolation_fraction();
		frame = state->previous_frame + (state->frame - state->previous_frame) * t;
		form_pose(model, form_animation(state->form, state->animation_index), frame,
			form_action_loops(state->action), pose);
		if (state->blend_ticks > 0 && state->from_animation_index != NONE)
		{
			struct real_orientation from[MAXIMUM_NODES_PER_MODEL];

			form_pose(model, form_animation(state->form, state->from_animation_index), state->from_frame, FALSE,
				from);
			floodfection_forms_blend(model->nodes.count, from, pose,
				PIN(((real)(FORM_BLEND_TICKS - state->blend_ticks) + t) / (real)FORM_BLEND_TICKS, 0.0f, 1.0f));
		}
		yaw = state->previous_yaw + form_angle_difference(state->previous_yaw, state->yaw) * t;
		forward.i = cosf(yaw);
		forward.j = sinf(yaw);
		forward.k = 0.0f;
		origin = object->object.position;
		render_interpolation_object_point(object_index, &origin);
		model_node_matrices_from_orientations(model, forms.matrices[slot], pose, &origin, &forward, global_up3d);
		*model_index = forms.model_index[state->form];
		*node_matrices = forms.matrices[slot];
		*region_permutations = form_region_permutations;
		return _floodfection_form_render_form;
	}
	return _floodfection_form_render_normal;
}
