/*
PARTICLE_SYSTEMS.C

symbols in this file:
0008DCF0 0040:
	_particle_systems_initialize (0000)
0008DD30 0020:
	_particle_systems_initialize_for_new_map (0000)
0008DD50 0030:
	_particle_system_orphan (0000)
0008DD80 00b0:
	_particle_system_delete (0000)
0008DE30 0010:
	_particle_systems_dispose (0000)
0008DE40 0010:
	_particle_systems_disconnect_from_structure_bsp (0000)
0008DE50 0140:
	_particle_systems_reconnect_to_structure_bsp (0000)
0008DF90 00b0:
	_particle_system_next_type_state_index (0000)
0008E040 0090:
	_particle_system_next_particle_state_index (0000)
0008E0D0 0070:
	_particle_system_update_default (0000)
0008E140 0040:
	_particle_system_new_particle_default (0000)
0008E180 0190:
	_particle_system_update_particle_default (0000)
0008E310 0030:
	_particle_system_update_explosion (0000)
0008E340 0060:
	_particle_systems_dispose_from_old_map (0000)
0008E3A0 0350:
	_particle_system_new_particles (0000)
0008E6F0 0100:
	_randomize_particle_variables (0000)
0008E7F0 0680:
	_particle_system_update (0000)
0008EE70 0550:
	_particle_system_render (0000)
0008F3C0 0110:
	_particle_system_new_particle_explosion (0000)
0008F4D0 0170:
	_particle_system_new_particle_jet (0000)
0008F640 0120:
	_particle_system_initialize (0000)
0008F760 0080:
	_particle_systems_update (0000)
0008F7E0 00a0:
	_particle_systems_render (0000)
0008F880 00d0:
	_particle_system_new_unattached (0000)
0008F950 01a0:
	_particle_system_new_attached (0000)
0025A6B8 0020:
	_rdata_0025a6b8 (0000)
	_ground_error (0018)
0025A6D8 001a:
	??_C@_0BK@BEJKBDOP@particle?5system?5particles?$AA@ (0000)
0025A6F4 0011:
	??_C@_0BB@DDOBODPO@particle?5systems?$AA@ (0000)
0025A708 0066:
	??_C@_0GG@NBDBKOPN@creation_function_index?$DO?$DN0?5?$CG?$CG?5cr@ (0000)
0025A770 0009:
	??_C@_08PKKGOGAD@particle?$AA@ (0000)
0025A77C 002a:
	??_C@_0CK@PCEEHMOG@c?3?2halo?2SOURCE?2effects?2particle_@ (0000)
0025A7A8 0092:
	??_C@_0JC@KDIJANOF@type_state_definition?9?$DOparticle_@ (0000)
0025A840 0081:
	??_C@_0IB@LHKMAGCA@system_definition?9?$DOsystem_update@ (0000)
0025A8C4 002c:
	??_C@_0CM@MKEIGADN@particle_systems?5?$CG?$CG?5particle_sys@ (0000)
*/

/* ---------- headers */

#include "effects/particle_systems.h"
#include "effects/particle_system_definitions.h"

#include "math/real_math.h"
#include "memory/data.h"
#include "objects/object_lights.h"
#include "objects/objects.h"
#include "physics/point_physics.h"
#include "render/render.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"

/* ---------- constants */

enum
{
	MAXIMUM_PARTICLE_SYSTEMS = 64,
	PARTICLE_SYSTEM_DATUM_SIZE = 0x158,
	MAXIMUM_SYSTEM_PARTICLES = 512,
	SYSTEM_PARTICLE_DATUM_SIZE = 0x80,
};

/* ---------- macros */

/* ---------- structures */

typedef void (*particle_system_update_proc)(
	struct particle_system_datum *system,
	real delta_time);

typedef void (*particle_system_particle_update_proc)(
	struct particle_system_datum const *system,
	short type_index,
	real delta_time,
	struct ps_particle_datum *particle);

typedef void (*particle_system_particle_creation_proc)(
	struct particle_system_datum const *system,
	short type_index,
	struct ps_particle_datum *particle,
	struct object_marker const *marker);

/* ---------- prototypes */

static void particle_system_delete(
	long system_index);
static boolean particle_system_initialize(
	long system_index);
void particle_system_update_default(
	struct particle_system_datum *system,
	real delta_time);
void particle_system_update_explosion(
	struct particle_system_datum *system,
	real delta_time);
void particle_system_new_particle_default(
	struct particle_system_datum const *system,
	short type_index,
	struct ps_particle_datum *particle,
	struct object_marker const *marker);
void particle_system_update(
	real delta_time,
	long system_index);
void particle_system_render(
	long system_index);
static void particle_system_next_type_state_index(
	struct particle_system_datum const *system,
	struct particle_type *type,
	struct particle_system_type const *type_definition);
static void particle_system_next_particle_state_index(
	struct ps_particle_datum *particle,
	struct particle_system_type const *type_definition);
void particle_system_new_particle_explosion(
	struct particle_system_datum const *system,
	short type_index,
	struct ps_particle_datum *particle,
	struct object_marker const *marker);
void particle_system_update_particle_default(
	struct particle_system_datum const *system,
	short type_index,
	real delta_time,
	struct ps_particle_datum *particle);
void particle_system_new_particle_jet(
	struct particle_system_datum const *system,
	short type_index,
	struct ps_particle_datum *particle,
	struct object_marker const *marker);
static void particle_system_new_particles(
	struct particle_system_datum *system,
	short type_index,
	real delta_time);
static void randomize_particle_variables(
	struct particle_system_type *type_definition,
	struct particle_system_type_particle_state_interpolated_randomized_variables *variables,
	short state_index);

/* ---------- globals */

static particle_system_update_proc const particle_system_update_functions[] =
{
	particle_system_update_default,
	particle_system_update_explosion,
};

static particle_system_particle_creation_proc const particle_system_particle_creation_functions[] =
{
	particle_system_new_particle_default,
	particle_system_new_particle_explosion,
	particle_system_new_particle_jet,
};

static particle_system_particle_update_proc const particle_system_particle_update_functions[] =
{
	particle_system_update_particle_default,
};

/* ---------- public code */

void particle_systems_initialize(
	void)
{
	particle_systems = game_state_data_new("particle systems", MAXIMUM_PARTICLE_SYSTEMS, PARTICLE_SYSTEM_DATUM_SIZE);
	system_particles = game_state_data_new("particle system particles", MAXIMUM_SYSTEM_PARTICLES, SYSTEM_PARTICLE_DATUM_SIZE);

	return;
}

void particle_systems_initialize_for_new_map(
	void)
{
	data_make_valid(particle_systems);
	data_make_valid(system_particles);

	return;
}

void particle_system_orphan(
	long system_index)
{
	struct particle_system_datum *system = particle_system_get(system_index);

	SET_FLAG(system->flags, _particle_system_active_bit, FALSE);
	system->object_index = NONE;

	return;
}

void particle_systems_dispose_from_old_map(
	void)
{
	if (particle_systems && particle_systems->valid)
	{
		long system_index;

		for (system_index = data_next_index(particle_systems, NONE);
			system_index != NONE;
			system_index = data_next_index(particle_systems, system_index))
		{
			particle_system_delete(system_index);
		}

		data_make_invalid(particle_systems);
		data_make_invalid(system_particles);
	}

	return;
}

void particle_systems_dispose(
	void)
{
	return;
}

void particle_systems_disconnect_from_structure_bsp(
	void)
{
	return;
}

void particle_systems_reconnect_to_structure_bsp(
	void)
{
	long system_index;

	for (system_index = data_next_index(particle_systems, NONE);
		system_index != NONE;
		system_index = data_next_index(particle_systems, system_index))
	{
		struct particle_system_datum *system = particle_system_get(system_index);
		struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);
		short type_index;

		if (system->object_index != NONE)
		{
			object_get_location(system->object_index, &system->location);
		}
		else
		{
			scenario_location_from_point(&system->location, &system->position);

			if (system->location.cluster_index == NONE)
			{
				particle_system_delete(system_index);
				continue;
			}
		}

		for (type_index = 0; type_index < definition->types.count; type_index++)
		{
			long *particle_index_reference = &system->types[type_index].first_particle_index;

			while (*particle_index_reference != NONE)
			{
				struct ps_particle_datum *particle = ps_particle_get(*particle_index_reference);

				scenario_location_from_point(&particle->location, &particle->position);

				if (particle->location.cluster_index == NONE)
				{
					datum_delete(system_particles, *particle_index_reference);

					/* datum_delete clears the identifier but retains datum storage. */
					*particle_index_reference = particle->next_particle_index;
				}
				else
				{
					particle_index_reference = &particle->next_particle_index;
				}
			}
		}
	}

	return;
}

static void particle_system_next_type_state_index(
	struct particle_system_datum const *system,
	struct particle_type *type,
	struct particle_system_type const *type_definition)
{
	boolean states_moving_forward = type->states_moving_forward;
	short state_index = type->state_index;
	short step = states_moving_forward ? 1 : -1;
	short next_state_index = state_index + step;

	type->transition_state_index = next_state_index;

	if (next_state_index < 0 || next_state_index >= type_definition->type_states.count)
	{
		unsigned long flags = type_definition->flags;
		long state_count;

		if (!TEST_FLAG(flags, _particle_system_type_type_states_loop_bit) ||
			system->object_index == NONE ||
			(state_count = type_definition->type_states.count) <= 0)
		{
			type->state_index = NONE;
			type->transition_state_index = NONE;
		}
		else if (TEST_FLAG(
			flags,
			_particle_system_type_type_states_loop_forward_backward_bit))
		{
			long bounced_state_index = state_index - step;

			if (bounced_state_index >= 0)
			{
				if (bounced_state_index > state_count - 1)
					bounced_state_index = state_count - 1;
			}
			else
			{
				bounced_state_index = 0;
			}

			type->transition_state_index = bounced_state_index;
			type->states_moving_forward = !states_moving_forward;
		}
		else
		{
			type->transition_state_index = 0;
		}
	}

	return;
}

static void particle_system_next_particle_state_index(
	struct ps_particle_datum *particle,
	struct particle_system_type const *type_definition)
{
	boolean states_moving_forward = particle->states_moving_forward;
	short state_index = particle->state_index;
	short step = states_moving_forward ? 1 : -1;
	short next_state_index = state_index + step;

	particle->transition_state_index = next_state_index;

	if (next_state_index < 0 || next_state_index >= type_definition->particle_states.count)
	{
		unsigned long flags = type_definition->flags;
		long state_count;

		if (TEST_FLAG(flags, _particle_system_type_particle_states_loop_bit) &&
			(state_count = type_definition->particle_states.count) > 0)
		{
			if (TEST_FLAG(
				flags,
				_particle_system_type_particle_states_loop_forward_backward_bit))
			{
				long bounced_state_index = state_index - step;

				if (bounced_state_index >= 0)
				{
					if (bounced_state_index > state_count - 1)
						bounced_state_index = state_count - 1;
				}
				else
				{
					bounced_state_index = 0;
				}

				particle->transition_state_index = bounced_state_index;
				particle->states_moving_forward = !states_moving_forward;
			}
			else
			{
				particle->transition_state_index = 0;
			}
		}
		else
		{
			particle->state_index = NONE;
			particle->transition_state_index = NONE;
		}
	}

	return;
}

long particle_system_new_unattached(
	long definition_index,
	real_point3d const *position,
	real_vector3d const *velocity,
	real_argb_color const *color,
	real scale)
{
	long system_index = datum_new(particle_systems);

	if (system_index != NONE)
	{
		struct particle_system_datum *system = particle_system_get(system_index);
		real_rgb_color diffuse;

		system->definition_index = definition_index;
		system->object_index = NONE;
		system->position = *position;
		system->velocity = *velocity;
		system->color = *color;
		system->scale = scale;
		SET_FLAG(system->flags, _particle_system_active_bit, TRUE);

		light_particle(&system->position, &system->lighting, &diffuse, FALSE);

		if (!particle_system_initialize(system_index))
		{
			datum_delete(particle_systems, system_index);
			return NONE;
		}
	}

	return system_index;
}

long particle_system_new_attached(
	long definition_index,
	long object_index,
	short attachment_index)
{
	long system_index = datum_new(particle_systems);

	if (system_index != NONE)
	{
		struct particle_system_datum *system = particle_system_get(system_index);
		struct object_datum *object = object_get(object_index);
		struct object_attachment_definition *attachment = TAG_BLOCK_GET_ELEMENT(
			&object_definition_get(object->definition_index)->object.attachments,
			attachment_index,
			struct object_attachment_definition);
		struct object_marker marker;

		system->definition_index = definition_index;
		system->object_index = object_index;
		system->attachment_index = attachment_index;
		system->function_index = attachment->primary_scale_function_reference - 1;

		if (attachment->change_color_reference)
		{
			system->color.rgb = object->object.outgoing_change_colors[attachment->change_color_reference];
			system->color.alpha = 1.0f;
		}
		else
		{
			system->color = *global_real_argb_white;
		}

		object_get_marker_by_name(object_index, attachment->marker_name, &marker, 1);
		system->position = marker.matrix.position;
		object_get_velocities(object_index, &system->velocity, NULL);
		scale_vector3d(&system->velocity, TICKS_PER_SECOND, &system->velocity);
		system->lighting = *global_real_rgb_white;
		SET_FLAG(
			system->flags,
			_particle_system_active_bit,
			object_get_function_value(object_index, system->function_index, &system->scale));

		if (!particle_system_initialize(system_index))
		{
			datum_delete(particle_systems, system_index);
			return NONE;
		}
	}

	return system_index;
}

/* ---------- private code */

static void particle_system_delete(
	long system_index)
{
	struct particle_system_datum *system = particle_system_get(system_index);
	struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);
	short type_index;

	for (type_index = 0; type_index < definition->types.count; type_index++)
	{
		struct particle_type *type = &system->types[type_index];
		long particle_index = type->first_particle_index;

		while (particle_index != NONE)
		{
			long next_particle_index = ps_particle_get(particle_index)->next_particle_index;

			datum_delete(system_particles, particle_index);
			particle_index = next_particle_index;
		}
	}

	datum_delete(particle_systems, system_index);

	return;
}

static boolean particle_system_initialize(
	long system_index)
{
	struct particle_system_datum *system = particle_system_get(system_index);
	struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);
	boolean success = TRUE;
	short type_index;

	scenario_location_from_point(&system->location, &system->position);
	SET_FLAG(system->flags, _particle_system_initializing_bit, TRUE);

	for (type_index = 0; type_index < definition->types.count; type_index++)
	{
		struct particle_type *type = &system->types[type_index];
		struct particle_system_type *type_definition = TAG_BLOCK_GET_ELEMENT(
			&definition->types,
			type_index,
			struct particle_system_type);

		if (type_definition->type_states.count)
		{
			type->state_index = 0;
			type->transition_state_index = NONE;
			type->states_moving_forward = TRUE;
			type->particle_count = 0;
			type->first_particle_index = NONE;

			if (type_definition->type_states.count > 0)
			{
				struct particle_system_type_state *state_definition = TAG_BLOCK_GET_ELEMENT(
					&type_definition->type_states, 0, struct particle_system_type_state);
				real duration = real_local_random_range(
					state_definition->duration_lower_bound,
					state_definition->duration_upper_bound);

				type->time_left_in_state = duration;
				type->state_length = duration;
			}
		}
		else
		{
			success = FALSE;
		}
	}

	if (success)
	{
		particle_system_update(0.001f, system_index);
	}

	return success;
}

void particle_system_update_default(
	struct particle_system_datum *system,
	real delta_time)
{
	struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);

	if (system->object_index == NONE && definition->system_update_point_physics.index != NONE)
	{
		point_physics_update(
			0,
			point_physics_definition_get(definition->system_update_point_physics.index),
			&system->location,
			NONE,
			&system->position,
			&system->velocity,
			NULL,
			NULL,
			NULL,
			1.0f,
			delta_time);
	}

	return;
}

void particle_system_new_particle_default(
	struct particle_system_datum const *system,
	short type_index,
	struct ps_particle_datum *particle,
	struct object_marker const *marker)
{
	particle->position = marker->matrix.position;
	particle->velocity = system->velocity;

	return;
}

void particle_system_update_explosion(
	struct particle_system_datum *system,
	real delta_time)
{
	particle_system_definition_get(system->definition_index);
	particle_system_update_default(system, delta_time);

	return;
}

static void particle_system_new_particles(
	struct particle_system_datum *system,
	short type_index,
	real delta_time)
{
	struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);
	struct particle_type *type = &system->types[type_index];
	struct particle_system_type *type_definition = TAG_BLOCK_GET_ELEMENT(
		&definition->types,
		type_index,
		struct particle_system_type);
	boolean initializing = TEST_FLAG(system->flags, _particle_system_initializing_bit);
	struct particle_system_type_state *state_definition;
	short target_particle_count;
	struct object_marker markers[8];
	short marker_count;

	if (initializing)
	{
		state_definition = NULL;
		if (TEST_FLAG(type_definition->flags, _particle_system_type_initial_count_scales_bit))
		{
			target_particle_count = (short)(
				(real)type_definition->initial_particle_count*system->scale + 0.5f);
		}
		else
		{
			target_particle_count = type_definition->initial_particle_count;
		}
	}
	else
	{
		state_definition = TAG_BLOCK_GET_ELEMENT(
			&type_definition->type_states,
			type->state_index,
			struct particle_system_type_state);

		{
			real particle_count = delta_time*type->variables.particle_creation_rate;
			short whole_particle_count = (short)(long)particle_count;

			type->fractional_particle_count += particle_count - (real)whole_particle_count;
			target_particle_count = type->particle_count + whole_particle_count;

			if (type->fractional_particle_count > 1.0f)
			{
				target_particle_count++;
				type->fractional_particle_count -= 1.0f;
			}
		}
	}

	if (type->particle_count < target_particle_count)
	{
		if (system->object_index != NONE)
		{
			struct object_datum *object = object_get(system->object_index);

			marker_count = object_get_marker_by_name(
				system->object_index,
				TAG_BLOCK_GET_ELEMENT(
					&object_definition_get(object->definition_index)->object.attachments,
					system->attachment_index,
					struct object_attachment_definition)->marker_name,
				markers,
				NUMBEROF(markers));
			object_get_location(system->object_index, &system->location);
		}
		else
		{
			marker_count = 1;
			markers[0].matrix.position = system->position;
			markers[0].matrix.forward = *global_zero_vector3d;
		}

		if (system->location.cluster_index != NONE)
		{
			short particles_created = 0;

			while (type->particle_count < target_particle_count)
			{
				long particle_index;

				if (!marker_count || particles_created >= 128)
					break;

				particle_index = datum_new(system_particles);

				if (particle_index == NONE)
					break;

				{
					struct ps_particle_datum *particle = ps_particle_get(particle_index);
					short creation_function_index = initializing
						? type_definition->initial_particle_creation_physics
						: state_definition->particle_creation_physics;

					match_assert(
						"c:\\halo\\SOURCE\\effects\\particle_systems.c",
						0x1DC,
						particle);

					particle->valid = TRUE;
					particle->state_index = NONE;
					particle->transition_state_index = NONE;
					particle->states_moving_forward = TRUE;
					particle->sprite_index = -1.0f;
					particle->rotation = real_seed_random_range(
						get_global_local_random_seed_address(),
						0.0f,
						_pi*2.0f);

					match_assert(
						"c:\\halo\\SOURCE\\effects\\particle_systems.c",
						0x1E8,
						creation_function_index>=0 &&
						creation_function_index<NUMBER_OF_PARTICLE_SYSTEM_TYPE_CREATION_PHYSICS);

					particle_system_particle_creation_functions[creation_function_index](
						system,
						type_index,
						particle,
						&markers[seed_random_range(
							get_global_local_random_seed_address(),
							0,
							marker_count)]);

					scenario_location_from_point(&particle->location, &particle->position);

					if (particle->location.cluster_index != NONE)
					{
						particle->next_particle_index = type->first_particle_index;
						type->particle_count++;
						type->first_particle_index = particle_index;
					}
					else
					{
						datum_delete(system_particles, particle_index);
					}
				}

				particles_created++;
			}
		}
	}

	if ((real)type->particle_count < type->variables.minimum_particle_count)
	{
		type->time_left_in_state *= 0.30000001f;
	}

	return;
}

void particle_systems_update(
	real delta_time)
{
	long system_index;

	match_assert(
		"c:\\halo\\SOURCE\\effects\\particle_systems.c",
		0x232,
		particle_systems && particle_systems->valid);

	for (system_index = data_next_index(particle_systems, NONE);
		system_index != NONE;
		system_index = data_next_index(particle_systems, system_index))
	{
		particle_system_update(delta_time, system_index);
	}

	return;
}

void particle_system_update_particle_default(
	struct particle_system_datum const *system,
	short type_index,
	real delta_time,
	struct ps_particle_datum *particle)
{
	struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);
	struct particle_system_type *type_definition = TAG_BLOCK_GET_ELEMENT(
		&definition->types,
		type_index,
		struct particle_system_type);
	struct particle_type const *type = &system->types[type_index];
	struct particle_system_type_particle_state *state_definition = TAG_BLOCK_GET_ELEMENT(
		&type_definition->particle_states,
		particle->state_index,
		struct particle_system_type_particle_state);
	struct point_physics_definition *physics;
	struct point_physics_definition interpolated_physics;
	real radius;
	unsigned long collision_flags;

	if (particle->transition_state_index == NONE)
	{
		radius = state_definition->variables.radius*
			type->variables.particle_state_multipliers.radius*
			type_definition->variables.radius;
		physics = point_physics_definition_get(state_definition->point_physics.index);
	}
	else
	{
		struct particle_system_type_particle_state *transition_state_definition = TAG_BLOCK_GET_ELEMENT(
			&type_definition->particle_states,
			particle->transition_state_index,
			struct particle_system_type_particle_state);
		real t = particle->time_left_in_state/particle->state_length;

		if (t < 0.0f)
		{
			t = 0.0f;
		}
		else if (t > 1.0f)
		{
			t = 1.0f;
		}

		radius = (state_definition->variables.radius*t +
			transition_state_definition->variables.radius*(1.0f - t))*
			type->variables.particle_state_multipliers.radius*
			type_definition->variables.radius;
		physics = point_physics_definition_interpolate(
			point_physics_definition_get(state_definition->point_physics.index),
			point_physics_definition_get(transition_state_definition->point_physics.index),
			t,
			&interpolated_physics);
	}

	collision_flags = point_physics_update(
		0,
		physics,
		&particle->location,
		NONE,
		&particle->position,
		&particle->velocity,
		NULL,
		NULL,
		NULL,
		radius,
		delta_time);

	if ((TEST_FLAG(collision_flags, _point_physics_in_air_bit) &&
			TEST_FLAG(type_definition->flags, _particle_system_type_dies_in_air_bit)) ||
		(TEST_FLAG(collision_flags, _point_physics_in_water_bit) &&
			TEST_FLAG(type_definition->flags, _particle_system_type_dies_in_water_bit)) ||
		(TEST_FLAG(collision_flags, _point_physics_collided_with_structure_bit) &&
			TEST_FLAG(type_definition->flags, _particle_system_type_dies_on_ground_bit)))
	{
		particle->valid = FALSE;
	}

	return;
}

void particle_system_new_particle_explosion(
	struct particle_system_datum const *system,
	short type_index,
	struct ps_particle_datum *particle,
	struct object_marker const *marker)
{
	struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);
	struct particle_system_type *type_definition = TAG_BLOCK_GET_ELEMENT(
		&definition->types,
		type_index,
		struct particle_system_type);
	real xy_spread = TAG_BLOCK_GET_ELEMENT(
		&type_definition->physics_constants,
		_explosion_type_definition_physics_constant_xy_spread,
		struct particle_system_physics_constant)->k;
	real z_spread = TAG_BLOCK_GET_ELEMENT(
		&type_definition->physics_constants,
		_explosion_type_definition_physics_constant_z_spread,
		struct particle_system_physics_constant)->k;
	real intensity = TAG_BLOCK_GET_ELEMENT(
		&type_definition->physics_constants,
		_explosion_type_definition_physics_constant_intensity,
		struct particle_system_physics_constant)->k;

	local_random_direction3d(&particle->velocity);
	particle->velocity.i *= xy_spread;
	particle->velocity.j *= xy_spread;
	particle->velocity.k *= z_spread;

	if (system->physics_state.explosion.grounded)
	{
		particle->velocity.k = (real)fabs(particle->velocity.k);
	}

	particle->position.x = marker->matrix.position.x + particle->velocity.i;
	particle->position.y = marker->matrix.position.y + particle->velocity.j;
	particle->position.z = marker->matrix.position.z + particle->velocity.k;

	particle->axis.i = particle->velocity.i;
	particle->axis.j = particle->velocity.j;
	particle->axis.k = 0.0f;

	particle->velocity.i = particle->velocity.i*intensity + system->velocity.i;
	particle->velocity.j = particle->velocity.j*intensity + system->velocity.j;
	particle->velocity.k = particle->velocity.k*intensity + system->velocity.k;

	rotate_vector_about_axis(&particle->axis, global_up3d, 1.0f, 0.0f);

	return;
}

void particle_system_new_particle_jet(
	struct particle_system_datum const *system,
	short type_index,
	struct ps_particle_datum *particle,
	struct object_marker const *marker)
{
	struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);
	struct particle_system_type *type_definition = TAG_BLOCK_GET_ELEMENT(
		&definition->types,
		type_index,
		struct particle_system_type);
	real velocity = TAG_BLOCK_GET_ELEMENT(
		&type_definition->physics_constants,
		_jet_type_definition_physics_constant_velocity,
		struct particle_system_physics_constant)->k;
	real spread_fraction = TAG_BLOCK_GET_ELEMENT(
		&type_definition->physics_constants,
		_jet_type_definition_physics_constant_spread_fraction,
		struct particle_system_physics_constant)->k;
	real rotates_up = TAG_BLOCK_GET_ELEMENT(
		&type_definition->physics_constants,
		_jet_type_definition_physics_constant_rotates_up,
		struct particle_system_physics_constant)->k;
	real spread_scale = spread_fraction*(velocity*(1.0f/TICKS_PER_SECOND));
	real directed_scale = (1.0f - spread_fraction)*(velocity*(1.0f/TICKS_PER_SECOND));
	real_vector3d random_direction;

	local_random_direction3d(&random_direction);

	particle->velocity.i = directed_scale*marker->matrix.forward.i + random_direction.i*spread_scale + system->velocity.i;
	particle->velocity.j = directed_scale*marker->matrix.forward.j + random_direction.j*spread_scale + system->velocity.j;
	particle->velocity.k = directed_scale*marker->matrix.forward.k + random_direction.k*spread_scale + system->velocity.k;
	particle->position = marker->matrix.position;

	if (rotates_up)
	{
		cross_product3d(&particle->velocity, global_up3d, &particle->axis);
	}
	else
	{
		cross_product3d(&marker->matrix.forward, &particle->velocity, &particle->axis);
	}

	return;
}

static void randomize_particle_variables(
	struct particle_system_type *type_definition,
	struct particle_system_type_particle_state_interpolated_randomized_variables *variables,
	short state_index)
{
	struct particle_system_type_particle_state *state_definition = TAG_BLOCK_GET_ELEMENT(
		&type_definition->particle_states,
		state_index,
		struct particle_system_type_particle_state);
	real range[3];

	range[2] = real_seed_random_range(
		get_global_local_random_seed_address(),
		0.0f,
		1.0f);

	range[1] = state_definition->animation_rate_upper_bound;
	range[0] = state_definition->animation_rate_lower_bound;
	variables->animation_rate = real_seed_random_range(
		get_global_local_random_seed_address(),
		range[0],
		range[1]);
	range[1] = state_definition->rotation_rate_upper_bound;
	range[0] = state_definition->rotation_rate_lower_bound;
	variables->rotation_rate = real_seed_random_range(
		get_global_local_random_seed_address(),
		range[0],
		range[1]);
	range[1] = state_definition->scale_upper_bound;
	range[0] = state_definition->scale_lower_bound;
	variables->scale = real_seed_random_range(
		get_global_local_random_seed_address(),
		range[0],
		range[1]);
	range[1] = state_definition->color_upper_bound.alpha;
	range[0] = state_definition->color_lower_bound.alpha;
	variables->color.alpha = real_seed_random_range(
		get_global_local_random_seed_address(),
		range[0],
		range[1]);
	variables->color.red =
		(state_definition->color_upper_bound.red - state_definition->color_lower_bound.red)*range[2] +
		state_definition->color_lower_bound.red;
	variables->color.green =
		(state_definition->color_upper_bound.green - state_definition->color_lower_bound.green)*range[2] +
		state_definition->color_lower_bound.green;
	variables->color.blue =
		(state_definition->color_upper_bound.blue - state_definition->color_lower_bound.blue)*range[2] +
		state_definition->color_lower_bound.blue;

	return;
}

void particle_system_update(
	real delta_time,
	long system_index)
{
	struct particle_system_datum *system = particle_system_get(system_index);
	struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);
	short live_type_count = 0;
	short type_index;

	if (system->object_index != NONE)
	{
		struct object_datum *object = object_get(system->object_index);

		if (TEST_FLAG(object->object.flags, _object_connected_to_map_bit) &&
			object_get_function_value(system->object_index, system->function_index, &system->scale))
		{
			SET_FLAG(system->flags, _particle_system_active_bit, TRUE);
		}
		else
		{
			SET_FLAG(system->flags, _particle_system_active_bit, FALSE);
		}

		object_get_origin(system->object_index, &system->position);
		object_get_velocities(system->object_index, &system->velocity, NULL);
		system->velocity.i *= TICKS_PER_SECOND;
		system->velocity.j *= TICKS_PER_SECOND;
		system->velocity.k *= TICKS_PER_SECOND;
	}

	assert(definition->system_update_physics >= 0 &&
		definition->system_update_physics < NUMBER_OF_PARTICLE_SYSTEM_UPDATE_PHYSICS);
	particle_system_update_functions[definition->system_update_physics](system, delta_time);

	for (type_index = 0; type_index < definition->types.count; type_index++)
	{
		struct particle_system_type *type_definition = TAG_BLOCK_GET_ELEMENT(
			&definition->types,
			type_index,
			struct particle_system_type);
		struct particle_type *type = &system->types[type_index];

		if (TEST_FLAG(type_definition->flags, _particle_system_type_disabled_bit))
			continue;

		type->time_left_in_state -= delta_time;

		if (type->state_index != NONE)
		{
			struct particle_system_type_state *current_state_definition = NULL;
			short transition_state_index = NONE;

			while (type->state_index != NONE)
			{
				current_state_definition = TAG_BLOCK_GET_ELEMENT(
					&type_definition->type_states,
					type->state_index,
					struct particle_system_type_state);
				transition_state_index = type->transition_state_index;

				if (type->time_left_in_state >= 0.0f)
					break;

				{
					real lower_bound;
					real upper_bound;

					if (transition_state_index == NONE)
					{
						particle_system_next_type_state_index(system, type, type_definition);
						lower_bound = current_state_definition->transition_time_lower_bound;
						upper_bound = current_state_definition->transition_time_upper_bound;
					}
					else
					{
						struct particle_system_type_state *transition_state_definition = TAG_BLOCK_GET_ELEMENT(
							&type_definition->type_states,
							transition_state_index,
							struct particle_system_type_state);

						type->state_index = transition_state_index;
						type->transition_state_index = NONE;
						lower_bound = transition_state_definition->duration_lower_bound;
						upper_bound = transition_state_definition->duration_upper_bound;
					}

					{
						real duration = real_seed_random_range(
							get_global_local_random_seed_address(),
							lower_bound,
							upper_bound);

						type->time_left_in_state += duration;
						type->state_length = duration;
					}
				}
			}

			if (type->state_index != NONE)
			{
				if (transition_state_index == NONE)
				{
					csmemcpy(
						&type->variables,
						&current_state_definition->variables,
						sizeof(type->variables));
				}
				else
				{
					struct particle_system_type_state *transition_state_definition = TAG_BLOCK_GET_ELEMENT(
						&type_definition->type_states,
						transition_state_index,
						struct particle_system_type_state);
					real t = type->time_left_in_state/type->state_length;
					real *destination = (real *)&type->variables;
					real const *current = (real const *)&current_state_definition->variables;
					real const *transition = (real const *)&transition_state_definition->variables;
					short variable_index;

					if (t < 0.0f)
						t = 0.0f;
					else if (t > 1.0f)
						t = 1.0f;

					for (variable_index = 0; variable_index < 10; variable_index++)
					{
						destination[variable_index] =
							current[variable_index]*t + transition[variable_index]*(1.0f - t);
					}
				}

				if (TEST_FLAG(type_definition->flags, _particle_system_type_tint_by_effect_color_bit))
				{
					type->variables.particle_state_randomized_multipliers.color.alpha *= system->color.alpha;
					type->variables.particle_state_randomized_multipliers.color.red *= system->color.red;
					type->variables.particle_state_randomized_multipliers.color.green *= system->color.green;
					type->variables.particle_state_randomized_multipliers.color.blue *= system->color.blue;
				}
				if (TEST_FLAG(type_definition->flags, _particle_system_type_minimum_count_scales_bit))
					type->variables.minimum_particle_count *= system->scale;
				if (TEST_FLAG(type_definition->flags, _particle_system_type_creation_rate_scales_bit))
					type->variables.particle_creation_rate *= system->scale;
				if (TEST_FLAG(type_definition->flags, _particle_system_type_scale_scales_bit))
					type->variables.particle_state_randomized_multipliers.scale *= system->scale;
				if (TEST_FLAG(type_definition->flags, _particle_system_type_animation_rate_scales_bit))
					type->variables.particle_state_randomized_multipliers.animation_rate *= system->scale;
				if (TEST_FLAG(type_definition->flags, _particle_system_type_rotation_rate_scales_bit))
					type->variables.particle_state_randomized_multipliers.rotation_rate *= system->scale;
			}
		}

		if (type->state_index != NONE)
		{
			struct ps_particle_datum *previous_particle = NULL;
			long particle_index;

			if (TEST_FLAG(system->flags, _particle_system_active_bit))
				particle_system_new_particles(system, type_index, delta_time);

			for (particle_index = type->first_particle_index;
				particle_index != NONE;)
			{
				long current_particle_index = particle_index;
				struct ps_particle_datum *particle = ps_particle_get(particle_index);

				particle->time_left_in_state -= delta_time;

				if (particle->state_index == NONE && type_definition->particle_states.count > 0)
				{
					struct particle_system_type_particle_state *first_state_definition = TAG_BLOCK_GET_ELEMENT(
						&type_definition->particle_states,
						0,
						struct particle_system_type_particle_state);
					real duration = real_seed_random_range(
						get_global_local_random_seed_address(),
						first_state_definition->duration_lower_bound,
						first_state_definition->duration_upper_bound);

					particle->state_index = 0;
					particle->time_left_in_state = duration;
					particle->state_length = duration;
					randomize_particle_variables(
						type_definition,
						&particle->randomized_variables,
						particle->state_index);
				}

				if (!particle->valid)
					particle->state_index = NONE;

				while (particle->state_index != NONE)
				{
					struct particle_system_type_particle_state *particle_state_definition = TAG_BLOCK_GET_ELEMENT(
						&type_definition->particle_states,
						particle->state_index,
						struct particle_system_type_particle_state);
					short transition_state_index;
					real lower_bound;
					real upper_bound;

					if (particle->time_left_in_state >= 0.0f)
						break;

					if (particle->transition_state_index == NONE)
					{
						particle_system_next_particle_state_index(particle, type_definition);
						lower_bound = particle_state_definition->transition_time_lower_bound;
						upper_bound = particle_state_definition->transition_time_upper_bound;
					}
					else
					{
						struct particle_system_type_particle_state *transition_state_definition;

						particle->state_index = particle->transition_state_index;
						particle->transition_state_index = NONE;
						transition_state_definition = TAG_BLOCK_GET_ELEMENT(
							&type_definition->particle_states,
							particle->state_index,
							struct particle_system_type_particle_state);
						lower_bound = transition_state_definition->duration_lower_bound;
						upper_bound = transition_state_definition->duration_upper_bound;
					}

					transition_state_index = particle->transition_state_index;

					{
						real duration = real_seed_random_range(
							get_global_local_random_seed_address(),
							lower_bound,
							upper_bound);

						particle->time_left_in_state += duration;
						particle->state_length = duration;
					}

					if (transition_state_index == NONE)
					{
						csmemcpy(
							&particle->randomized_variables,
							&particle->transition_randomized_variables,
							sizeof(particle->randomized_variables));
					}
					else
					{
						randomize_particle_variables(
							type_definition,
							&particle->transition_randomized_variables,
							transition_state_index);
					}
				}

				if (particle->state_index == NONE)
				{
					long next_particle_index = particle->next_particle_index;

					if (previous_particle)
						previous_particle->next_particle_index = next_particle_index;
					else
						type->first_particle_index = next_particle_index;

					datum_delete(system_particles, current_particle_index);
					particle_index = next_particle_index;
					type->particle_count--;
				}
				else
				{
					struct particle_system_type_state *type_state_definition = TAG_BLOCK_GET_ELEMENT(
						&type_definition->type_states,
						type->state_index,
						struct particle_system_type_state);

					if (particle->transition_state_index == NONE)
					{
						particle->rotation += particle->randomized_variables.rotation_rate*
							type->variables.particle_state_randomized_multipliers.rotation_rate*
							delta_time;
						particle->sprite_index += particle->randomized_variables.animation_rate*
							type->variables.particle_state_randomized_multipliers.animation_rate*
							delta_time;
					}
					else
					{
						real t = particle->time_left_in_state/particle->state_length;
						real inverse_t;
						real animation_rate;
						real rotation_rate;

						if (t < 0.0f)
							t = 0.0f;
						else if (t > 1.0f)
							t = 1.0f;

						inverse_t = 1.0f - t;
						animation_rate = particle->transition_randomized_variables.animation_rate*inverse_t +
							particle->randomized_variables.animation_rate*t;
						rotation_rate = particle->transition_randomized_variables.rotation_rate*inverse_t +
							particle->randomized_variables.rotation_rate*t;
						particle->rotation += rotation_rate*
							type->variables.particle_state_randomized_multipliers.rotation_rate*
							delta_time;
						particle->sprite_index += animation_rate*
							type->variables.particle_state_randomized_multipliers.animation_rate*
							delta_time;
					}

					assert(type_state_definition->particle_update_physics >= 0 &&
						type_state_definition->particle_update_physics < NUMBER_OF_PARTICLE_SYSTEM_TYPE_UPDATE_PHYSICS);
					particle_system_particle_update_functions[type_state_definition->particle_update_physics](
						system,
						type_index,
						delta_time,
						particle);

					previous_particle = particle;
					particle_index = particle->next_particle_index;
				}
			}

			live_type_count++;
		}
	}

	SET_FLAG(system->flags, _particle_system_initializing_bit, FALSE);

	if (live_type_count == 0 && system->object_index == NONE)
		particle_system_delete(system_index);

	return;
}

void particle_systems_render(
	void)
{
	if (render_particle_systems_enabled)
	{
		long system_index;

		match_assert(
			"c:\\halo\\SOURCE\\effects\\particle_systems.c",
			0x3D0,
			particle_systems && particle_systems->valid);

		for (system_index = data_next_index(particle_systems, NONE);
			system_index != NONE;
			system_index = data_next_index(particle_systems, system_index))
		{
			struct particle_system_datum const *system = particle_system_get(system_index);

			if (system->location.cluster_index != NONE &&
				scenario_location_potentially_visible_local(&system->location))
			{
				particle_system_render(system_index);
			}
		}
	}

	return;
}
