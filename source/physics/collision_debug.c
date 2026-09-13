/*
COLLISION_DEBUG.C

symbols in this file:
00139530 1060:
	_collision_debug_render (0000)
0028B2A8 0060:
	_rdata_0028b2a8 (0000)
0028B308 000a:
	??_C@_09PCOGIILP@count?$DM?$DN14?$AA@ (0000)
0028B314 0031:
	??_C@_0DB@EBGLDBDO@plane?5?$CD?$CFd?$CFs?$HMnsurface?5?$CD?$CFd?$CFs?$CFs?$CFs?$CFs@ (0000)
0028B348 0004:
	__real@42652ee0 (0000)
0028B34C 0009:
	??_C@_08JNJLEJIF@?5negated?$AA@ (0000)
0028B358 000b:
	??_C@_0L@DMHIANBO@?5two?9sided?$AA@ (0000)
0028B364 000b:
	??_C@_0L@OFKNNBE@?5invisible?$AA@ (0000)
0028B370 000b:
	??_C@_0L@JOKAPIDA@?5climbable?$AA@ (0000)
0028B37C 000b:
	??_C@_0L@KCGACEBO@?5breakable?$AA@ (0000)
0028B388 000c:
	??_C@_0M@CIBLPKPA@phantom?5bsp?$AA@ (0000)
0028B394 0004:
	__real@3e162051 (0000)
0028B398 0004:
	__real@bd962051 (0000)
0028B3A0 0008:
	__real@401921fb60000000 (0000)
0028B3A8 0029:
	??_C@_0CJ@OKHLJDEM@c?3?2halo?2SOURCE?2physics?2collision@ (0000)
0030CD44 0010:
	_collision_debug_flag_front_facing_surfaces (0000)
	_collision_debug_flag_ignore_invisible_surfaces (0001)
	_collision_debug_flag_structure (0002)
	_collision_debug_flag_media (0003)
	_collision_debug_flag_objects (0004)
	_collision_debug_length (0008)
	_collision_debug_ignore_object_index (000c)
00456EC0 7122:
	_bss_00456ec0 (0000)
	_collision_debug_width (7118)
	_collision_debug_height (711c)
	_collision_debug_phantom_bsp (7120)
	_collision_debug_phantom_bsp_found (7121)
*/

/* ---------- headers */

#include "cseries.h"
#include "collision_debug.h"
#include "breakable_surfaces.h"
#include "collision_bsp.h"
#include "collision_bsp_definitions.h"
#include "collision_features.h"
#include "collisions.h"
#include "collision_usage.h"
#include "objects.h"
#include "tag_files/tag_files.h"
#include "collision_model_definitions.h"
#include "collision_models.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "memory/data.h"
#include "render/render.h"
#include "render/render_cameras_internal.h"
#include "render/render_debug.h"
#include "scenario/scenario.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	MAXIMUM_COLLISION_DEBUG_RESULTS = 14,
	MAXIMUM_COLLISION_DEBUG_PATH_POINTS = MAXIMUM_COLLISION_DEBUG_RESULTS + 2,
	COLLISION_DEBUG_SPRAY_COLUMNS = 40,
	COLLISION_DEBUG_SPRAY_ROWS = 30,
	COLLISION_DEBUG_SPRAY_COUNT = COLLISION_DEBUG_SPRAY_COLUMNS * COLLISION_DEBUG_SPRAY_ROWS,
};

enum collision_surface_flags
{
	_collision_surface_two_sided_bit = 0,
	_collision_surface_invisible_bit,
	_collision_surface_climbable_bit,
	_collision_surface_breakable_bit,
};

/* ---------- macros */

/* ---------- structures */

struct collision_debug_spray_cache
{
	real_vector3d normals[COLLISION_DEBUG_SPRAY_COUNT];
	real_point3d points[COLLISION_DEBUG_SPRAY_COUNT];
	unsigned long hit_flags[BIT_VECTOR_SIZE_IN_LONGS(COLLISION_DEBUG_SPRAY_COUNT)];
};

typedef char collision_debug_spray_cache_size_assert[
	sizeof(struct collision_debug_spray_cache) == 0x7118 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

extern boolean collision_debug;
extern boolean collision_debug_features;
extern unsigned long collision_debug_flags;
extern real_point3d collision_debug_phantom_bsp_point;
extern real_point3d collision_debug_point;
extern boolean collision_debug_repeat;
extern boolean collision_debug_spray;
extern real_vector3d collision_debug_vector;

extern boolean collision_debug_flag_back_facing_surfaces;
extern boolean collision_debug_flag_ignore_breakable_surfaces;
extern boolean collision_debug_flag_ignore_two_sided_surfaces;
extern boolean collision_debug_flag_objects_bipeds;
extern boolean collision_debug_flag_objects_controls;
extern boolean collision_debug_flag_objects_equipment;
extern boolean collision_debug_flag_objects_light_fixtures;
extern boolean collision_debug_flag_objects_machines;
extern boolean collision_debug_flag_objects_placeholders;
extern boolean collision_debug_flag_objects_projectiles;
extern boolean collision_debug_flag_objects_scenery;
extern boolean collision_debug_flag_objects_vehicles;
extern boolean collision_debug_flag_objects_weapons;
extern boolean collision_debug_flag_skip_passthrough_bipeds;
extern boolean collision_debug_flag_try_to_keep_location_valid;
extern boolean collision_debug_flag_use_vehicle_physics;

static const real_point3d collision_debug_cube_vertices[NUMBER_OF_VERTICES_PER_CUBE] =
{
	{ -1.0f, -1.0f, -1.0f },
	{ -1.0f, -1.0f,  1.0f },
	{ -1.0f,  1.0f, -1.0f },
	{ -1.0f,  1.0f,  1.0f },
	{  1.0f, -1.0f, -1.0f },
	{  1.0f, -1.0f,  1.0f },
	{  1.0f,  1.0f, -1.0f },
	{  1.0f,  1.0f,  1.0f },
};

boolean collision_debug_flag_front_facing_surfaces = TRUE;
boolean collision_debug_flag_ignore_invisible_surfaces = TRUE;
boolean collision_debug_flag_structure = TRUE;
boolean collision_debug_flag_media = TRUE;
boolean collision_debug_flag_objects = TRUE;
real collision_debug_length = 100.0f;
long collision_debug_ignore_object_index = NONE;

static struct collision_debug_spray_cache collision_debug_spray_globals = { 0 };
real collision_debug_width = 0.0f;
real collision_debug_height = 0.0f;
boolean collision_debug_phantom_bsp = FALSE;
boolean collision_debug_phantom_bsp_found = FALSE;

/* ---------- public code */

void collision_debug_render(
	void)
{
	char buffer[1000];

	match_assert(
		"c:\\halo\\SOURCE\\physics\\collision_debug.c",
		76,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_debugging;

	if (collision_debug || collision_debug_spray || collision_debug_features || collision_debug_phantom_bsp)
	{
		unsigned long flags;
		real length;
		real_point3d debug_point;
		real_vector3d debug_vector;
		long ignore_object_index;

		flags = collision_debug_flag_front_facing_surfaces != FALSE;
		SET_FLAG(flags, _collision_test_back_facing_surfaces_bit, collision_debug_flag_back_facing_surfaces);
		SET_FLAG(flags, _collision_test_ignore_two_sided_surfaces_bit, collision_debug_flag_ignore_two_sided_surfaces);
		SET_FLAG(flags, _collision_test_ignore_invisible_surfaces_bit, collision_debug_flag_ignore_invisible_surfaces);
		SET_FLAG(flags, _collision_test_ignore_breakable_surfaces_bit, collision_debug_flag_ignore_breakable_surfaces);
		SET_FLAG(flags, _collision_test_structure_bit, collision_debug_flag_structure);
		SET_FLAG(flags, _collision_test_media_bit, collision_debug_flag_media);
		SET_FLAG(flags, _collision_test_objects_bit, collision_debug_flag_objects);
		SET_FLAG(flags, _collision_test_objects_bipeds_bit, collision_debug_flag_objects_bipeds);
		SET_FLAG(flags, _collision_test_objects_vehicles_bit, collision_debug_flag_objects_vehicles);
		SET_FLAG(flags, _collision_test_objects_weapons_bit, collision_debug_flag_objects_weapons);
		SET_FLAG(flags, _collision_test_objects_equipment_bit, collision_debug_flag_objects_equipment);
		SET_FLAG(flags, _collision_test_objects_projectiles_bit, collision_debug_flag_objects_projectiles);
		SET_FLAG(flags, _collision_test_objects_scenery_bit, collision_debug_flag_objects_scenery);
		SET_FLAG(flags, _collision_test_objects_machines_bit, collision_debug_flag_objects_machines);
		SET_FLAG(flags, _collision_test_objects_controls_bit, collision_debug_flag_objects_controls);
		SET_FLAG(flags, _collision_test_objects_light_fixtures_bit, collision_debug_flag_objects_light_fixtures);
		SET_FLAG(flags, _collision_test_objects_placeholders_bit, collision_debug_flag_objects_placeholders);
		SET_FLAG(flags, _collision_test_try_to_keep_location_valid_bit, collision_debug_flag_try_to_keep_location_valid);
		SET_FLAG(flags, _collision_test_skip_passthrough_bipeds_bit, collision_debug_flag_skip_passthrough_bipeds);
		SET_FLAG(flags, _collision_test_use_vehicle_physics_bit, collision_debug_flag_use_vehicle_physics);

		if (collision_debug_flags)
			flags = collision_debug_flags;

		if (!collision_debug_repeat)
		{
			long player_index = local_player_get_player_index(render.local_player_index);

			if (player_index == NONE)
				collision_debug_ignore_object_index = NONE;
			else
				collision_debug_ignore_object_index = player_get(local_player_get_player_index(render.local_player_index))->unit_index;

			collision_debug_point = render.camera.position;
			collision_debug_vector = render.camera.forward;
		}

		length = (real)fabs(collision_debug_length);
		debug_point = collision_debug_point;
		debug_vector.i = collision_debug_vector.i * length;
		debug_vector.j = collision_debug_vector.j * length;
		debug_vector.k = collision_debug_vector.k * length;
		ignore_object_index = collision_debug_ignore_object_index;

		if (collision_debug_phantom_bsp)
		{
			struct collision_bsp_test_vector_result collision;

			if (collision_bsp_test_vector(
				flags,
				global_collision_bsp_get(),
				MAXIMUM_BREAKABLE_SURFACES_PER_MAP,
				breakable_surface_flags_get(),
				&debug_point,
				&debug_vector,
				1.0f,
				&collision))
			{
				real_point3d point;
				struct collision_feature_list features;

				point.x = debug_point.x + debug_vector.i * collision.t;
				point.y = debug_point.y + debug_vector.j * collision.t;
				point.z = debug_point.z + debug_vector.k * collision.t;

				if (!collision_get_features_in_sphere(
					flags,
					&point,
					0.01f,
					0.0f,
					0.01f,
					ignore_object_index,
					&features))
				{
					collision_debug_phantom_bsp_found = TRUE;
					collision_debug_phantom_bsp_point = point;
				}
			}

			if (collision_debug_phantom_bsp_found)
			{
				real yaw;
				real pitch;
				real roll;
				real_matrix4x3 rotation;
				real_point3d points[8];
				short point_index;

				yaw = (real)fmod((real)game_time_get() * 0.104719758f, 6.2831854820251465);
				pitch = (real)fmod((real)game_time_get() * -0.073303826f, 6.2831854820251465);
				roll = (real)fmod((real)game_time_get() * 0.146607652f, 6.2831854820251465);

				render_debug_string_at_point(
					TRUE,
					&collision_debug_phantom_bsp_point,
					"phantom bsp",
					global_real_argb_pink);
				matrix4x3_rotation_from_angles(&rotation, yaw, pitch, roll);

				for (point_index = 0; point_index < NUMBEROF(points); point_index++)
				{
					real_vector3d transformed;

					matrix4x3_transform_vector(
						&rotation,
						(real_vector3d const *)&collision_debug_cube_vertices[point_index],
						&transformed);
					points[point_index].x = collision_debug_phantom_bsp_point.x + transformed.i * 0.15f;
					points[point_index].y = collision_debug_phantom_bsp_point.y + transformed.j * 0.15f;
					points[point_index].z = collision_debug_phantom_bsp_point.z + transformed.k * 0.15f;
				}

				render_debug_line(TRUE, &points[0], &points[1], global_real_argb_pink);
				render_debug_line(TRUE, &points[1], &points[3], global_real_argb_pink);
				render_debug_line(TRUE, &points[3], &points[2], global_real_argb_pink);
				render_debug_line(TRUE, &points[2], &points[0], global_real_argb_pink);
				render_debug_line(TRUE, &points[4], &points[5], global_real_argb_pink);
				render_debug_line(TRUE, &points[5], &points[7], global_real_argb_pink);
				render_debug_line(TRUE, &points[7], &points[6], global_real_argb_pink);
				render_debug_line(TRUE, &points[6], &points[4], global_real_argb_pink);
				render_debug_line(TRUE, &points[0], &points[4], global_real_argb_pink);
				render_debug_line(TRUE, &points[1], &points[5], global_real_argb_pink);
				render_debug_line(TRUE, &points[2], &points[6], global_real_argb_pink);
				render_debug_line(TRUE, &points[3], &points[7], global_real_argb_pink);
			}
		}

		if (collision_debug)
		{
			if (collision_debug_length <= 0.0f)
			{
				debug_point.x += debug_vector.i;
				debug_point.y += debug_vector.j;
				debug_point.z += debug_vector.k;

				if (collision_debug_width <= 0.0f)
				{
					if (collision_test_point(flags, &debug_point, ignore_object_index))
						render_debug_point(TRUE, &debug_point, 0.1f, global_real_argb_red);
					else
						render_debug_point(TRUE, &debug_point, 0.1f, global_real_argb_green);
				}
				else
				{
					if (collision_test_sphere(&debug_point, collision_debug_width, ignore_object_index))
						render_debug_sphere(TRUE, &debug_point, collision_debug_width, global_real_argb_red);
					else
						render_debug_sphere(TRUE, &debug_point, collision_debug_width, global_real_argb_green);
				}
			}
			else if (collision_debug_width <= 0.0f)
			{
				struct collision_result collision;

				if (collision_test_vector(flags, &debug_point, &debug_vector, ignore_object_index, &collision))
				{
					struct collision_bsp *bsp = NULL;
					real_matrix4x3 const *matrix = NULL;
					struct collision_surface const *surface = NULL;

					render_debug_vector(TRUE, &debug_point, &debug_vector, collision.t, global_real_argb_red);
					render_debug_point(TRUE, &collision.point, 0.125f, global_real_argb_red);
					render_debug_vector(TRUE, &collision.point, &collision.plane.n, 0.25f, global_real_argb_red);

					if (collision.type == _collision_result_structure)
					{
						bsp = global_collision_bsp_get();
					}
					else if (collision.type == _collision_result_object && collision.node_index != NONE)
					{
						struct collision_model_instance instance;

						if (collision_model_instance_new(&instance, collision.object_index))
						{
							struct collision_node *node = TAG_BLOCK_GET_ELEMENT(
								&instance.model->nodes,
								collision.node_index,
								struct collision_node);
							bsp = TAG_BLOCK_GET_ELEMENT(&node->bsps, collision.bsp_index, struct collision_bsp);
							matrix = &instance.matrices[collision.node_index];
						}
					}

					if (bsp)
					{
						surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, collision.surface_index, struct collision_surface);
						render_debug_collision_surface(bsp, collision.surface_index, matrix, global_real_argb_red);

						/*
						 * Original January bug: the destination is 1000 bytes, but the
						 * historical call advertises 2048. Preserve it for byte matching.
						 * A safe non-matching build should pass sizeof(buffer) instead.
						 */
						_snprintf(
							buffer,
							2048,
							"plane #%d%s|nsurface #%d%s%s%s%s|n%s|n%f degrees",
							surface->plane_designator & LONG_MAX,
							surface->plane_designator < 0 ? " negated" : "",
							collision.surface_index,
							TEST_FLAG(collision.flags, _collision_surface_two_sided_bit) ? " two-sided" : "",
							TEST_FLAG(collision.flags, _collision_surface_invisible_bit) ? " invisible" : "",
							TEST_FLAG(collision.flags, _collision_surface_climbable_bit) ? " climbable" : "",
							TEST_FLAG(collision.flags, _collision_surface_breakable_bit) ? " breakable" : "",
							material_get_name(collision.material_type),
							(real)acos(collision.plane.n.k) * 57.295776f);
						render_debug_string(TRUE, buffer);
					}
				}
				else
				{
					real_point3d endpoint;
					endpoint.x = debug_point.x + debug_vector.i;
					endpoint.y = debug_point.y + debug_vector.j;
					endpoint.z = debug_point.z + debug_vector.k;
					render_debug_line(TRUE, &debug_point, &endpoint, global_real_argb_green);
					render_debug_point(TRUE, &endpoint, 0.125f, global_real_argb_green);
				}
			}
			else
			{
				short count;
				short collision_index;
				real_point3d clipped_position;
				real_vector3d clipped_velocity;
				struct collision_plane collisions[MAXIMUM_COLLISION_DEBUG_PATH_POINTS];

				clipped_position = debug_point;
				clipped_velocity = debug_vector;

				if (collision_debug_height > 0.0f)
				{
					count = collision_move_pill(
						flags,
						&debug_point,
						&debug_vector,
						collision_debug_height,
						collision_debug_width,
						ignore_object_index,
						&clipped_position,
						&clipped_velocity,
						MAXIMUM_COLLISION_DEBUG_RESULTS,
						collisions);
				}
				else
				{
					count = collision_move_sphere(
						flags,
						&debug_point,
						&debug_vector,
						collision_debug_width,
						ignore_object_index,
						&clipped_position,
						&clipped_velocity,
						MAXIMUM_COLLISION_DEBUG_RESULTS,
						collisions);
				}

				render_debug_vector(TRUE, &debug_point, &debug_vector, 1.0f, global_real_argb_blue);
				match_assert(
					"c:\\halo\\SOURCE\\physics\\collision_debug.c",
					297,
					count <= MAXIMUM_COLLISION_DEBUG_RESULTS);

				memmove(&collisions[1], &collisions[0], count * sizeof(collisions[0]));
				collisions[0].point = debug_point;
				memset(&collisions[0].plane, 0, sizeof(collisions[0].plane));
				collisions[count + 1].point = clipped_position;
				memset(&collisions[count + 1].plane, 0, sizeof(collisions[0].plane));

				for (collision_index = 0; collision_index < count + 2; collision_index++)
				{
					render_debug_point(TRUE, &collisions[collision_index].point, 0.0625f, global_real_argb_red);
					if (collision_index > 0)
						render_debug_line(TRUE, &collisions[collision_index - 1].point, &collisions[collision_index].point, global_real_argb_red);
					render_debug_vector(TRUE, &collisions[collision_index].point, &collisions[collision_index].plane.n, 0.125f, global_real_argb_red);
				}

				render_debug_vector(TRUE, &clipped_position, &clipped_velocity, 1.0f, global_real_argb_green);
			}
		}

		if (collision_debug_spray)
		{
			if (collision_debug_length <= 0.0f)
			{
				real_point3d point;
				real minimum_x = render.camera.position.x + render.camera.forward.i + render.camera.forward.i - 0.125f;
				real maximum_x = render.camera.position.x + render.camera.forward.i + render.camera.forward.i + 0.125f;
				real minimum_y = render.camera.position.y + render.camera.forward.j + render.camera.forward.j - 0.125f;
				real maximum_y = render.camera.position.y + render.camera.forward.j + render.camera.forward.j + 0.125f;
				real minimum_z = render.camera.position.z + render.camera.forward.k + render.camera.forward.k - 0.125f;
				real maximum_z = render.camera.position.z + render.camera.forward.k + render.camera.forward.k + 0.125f;

				for (point.z = minimum_z; point.z < maximum_z; point.z += 0.0625f)
				{
					for (point.y = minimum_y; point.y < maximum_y; point.y += 0.0625f)
					{
						for (point.x = minimum_x; point.x < maximum_x; point.x += 0.0625f)
						{
							real_argb_color const *color = collision_test_point(flags, &point, NONE)
								? global_real_argb_red
								: global_real_argb_green;
							render_debug_point(TRUE, &point, 0.03125f, color);
						}
					}
				}
			}
			else
			{
				if (!collision_debug_repeat)
				{
					long row;
					long column;
					long spray_index;
					long rows_remaining;
					long columns_remaining;
					real_rectangle2d projection_bounds;
					real_vector3d horizontal_step;
					real_vector3d vertical_step;
					real_vector3d base_vector;

					render_frustum_get_projection_bounds(&render.frustum, &projection_bounds);
					horizontal_step.i = (projection_bounds.x1 - projection_bounds.x0) * collision_debug_length * 0.025f;
					horizontal_step.j = 0.0f;
					horizontal_step.k = 0.0f;
					vertical_step.i = 0.0f;
					vertical_step.j = (projection_bounds.y1 - projection_bounds.y0) * collision_debug_length * (1.0f / 30.0f);
					vertical_step.k = 0.0f;
					base_vector.i = projection_bounds.x0 * collision_debug_length;
					base_vector.j = projection_bounds.y0 * collision_debug_length;
					base_vector.k = -collision_debug_length;

					matrix4x3_transform_vector(&render.frustum.view_to_world, &horizontal_step, &horizontal_step);
					matrix4x3_transform_vector(&render.frustum.view_to_world, &vertical_step, &vertical_step);
					matrix4x3_transform_vector(&render.frustum.view_to_world, &base_vector, &base_vector);

					row = 0;
					rows_remaining = COLLISION_DEBUG_SPRAY_ROWS;
					do
					{
						spray_index = row * COLLISION_DEBUG_SPRAY_COLUMNS;
						column = 0;
						columns_remaining = COLLISION_DEBUG_SPRAY_COLUMNS;
						do
						{
							boolean hit;
							struct collision_result collision;
							real_vector3d direction;

							direction.i = base_vector.i + horizontal_step.i * column + vertical_step.i * row;
							direction.j = base_vector.j + horizontal_step.j * column + vertical_step.j * row;
							direction.k = base_vector.k + horizontal_step.k * column + vertical_step.k * row;

							hit = collision_test_vector(flags, &render.camera.position, &direction, ignore_object_index, &collision);
							if (hit)
							{
								BIT_VECTOR_SET_FLAG(collision_debug_spray_globals.hit_flags, spray_index, TRUE);
								collision_debug_spray_globals.points[spray_index] = collision.point;
								collision_debug_spray_globals.normals[spray_index] = collision.plane.n;
							}
							else
							{
								BIT_VECTOR_SET_FLAG(collision_debug_spray_globals.hit_flags, spray_index, FALSE);
							}

							column++;
							spray_index++;
						} while (--columns_remaining);

						row++;
					} while (--rows_remaining);
				}

				{
					long row;
					long spray_index;
					long rows_remaining;
					long columns_remaining;

					row = 0;
					rows_remaining = COLLISION_DEBUG_SPRAY_ROWS;
					do
					{
						spray_index = row * COLLISION_DEBUG_SPRAY_COLUMNS;
						columns_remaining = COLLISION_DEBUG_SPRAY_COLUMNS;
						do
						{
							if (BIT_VECTOR_TEST_FLAG(collision_debug_spray_globals.hit_flags, spray_index))
							{
								render_debug_point(TRUE, &collision_debug_spray_globals.points[spray_index], 0.03125f, global_real_argb_red);
								render_debug_vector(
									TRUE,
									&collision_debug_spray_globals.points[spray_index],
									&collision_debug_spray_globals.normals[spray_index],
									0.0625f,
									global_real_argb_red);
							}

							spray_index++;
						} while (--columns_remaining);

						row++;
					} while (--rows_remaining);
				}
			}
		}

		if (collision_debug_features)
		{
			real radius;
			real_point3d center;
			struct collision_feature_list features;

			center.x = collision_debug_point.x + collision_debug_vector.i * 0.5f;
			center.y = collision_debug_point.y + collision_debug_vector.j * 0.5f;
			center.z = collision_debug_point.z + collision_debug_vector.k * 0.5f + collision_debug_height * 0.5f;
			radius = (real)sqrt(
				collision_debug_vector.i * collision_debug_vector.i
				+ collision_debug_vector.j * collision_debug_vector.j
				+ collision_debug_vector.k * collision_debug_vector.k) * 0.5f
				+ collision_debug_height * 0.5f
				+ collision_debug_width;

			if (collision_get_features_in_sphere(
				flags,
				&center,
				radius,
				collision_debug_height,
				collision_debug_width,
				collision_debug_ignore_object_index,
				&features))
			{
				render_debug_collision_features(&features);
			}
		}
	}

	match_assert(
		"c:\\halo\\SOURCE\\physics\\collision_debug.c",
		443,
		global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return;
}

/* ---------- private code */
