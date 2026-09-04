/*
BIPEDS.C

symbols in this file:
0018FFD0 0010:
	_animation_left_foot_frame_index (0000)
0018FFE0 0010:
	_animation_right_foot_frame_index (0000)
0018FFF0 0010:
	_bipeds_initialize (0000)
00190000 0010:
	_bipeds_initialize_for_new_map (0000)
00190010 0010:
	_bipeds_dispose_from_old_map (0000)
00190020 0010:
	_bipeds_dispose (0000)
00190030 0030:
	_biped_place (0000)
00190060 0010:
	_biped_delete (0000)
00190070 0060:
	_biped_reset (0000)
001900D0 0030:
	_biped_disconnect_from_structure_bsp (0000)
00190100 00c0:
	_biped_get_physics_pill (0000)
001901C0 0020:
	_biped_stop_melee_attack (0000)
001901E0 0080:
	_biped_start_limp_body_physics (0000)
00190260 0050:
	_biped_stop_limp_body_physics (0000)
001902B0 00f0:
	_code_001902b0 (0000)
001903A0 00b0:
	_code_001903a0 (0000)
00190450 01d0:
	_code_00190450 (0000)
00190620 0050:
	_biped_flying_through_air (0000)
00190670 0110:
	_code_00190670 (0000)
00190780 0110:
	_code_00190780 (0000)
00190890 0060:
	_biped_adjust_placement (0000)
001908F0 00c0:
	_biped_export_function_values (0000)
001909B0 01a0:
	_biped_get_sight_position (0000)
00190B50 0150:
	_biped_get_autoaim_pill (0000)
00190CA0 04d0:
	_biped_fix_position (0000)
00191170 0110:
	_biped_render_debug (0000)
00191280 0180:
	_biped_find_ground_surface (0000)
00191400 0030:
	_biped_approximate_surface_index (0000)
00191430 01c0:
	_biped_find_pathfinding_surface_index (0000)
001915F0 00f0:
	_biped_exit_seat_end (0000)
001916E0 0140:
	_code_001916e0 (0000)
00191820 01b0:
	_code_00191820 (0000)
001919D0 0130:
	_code_001919d0 (0000)
00191B00 01b0:
	_code_00191b00 (0000)
00191CB0 01a0:
	_code_00191cb0 (0000)
00191E50 0220:
	_biped_find_nearby_support_surface (0000)
00192070 0100:
	_biped_verify_object_vectors (0000)
00192170 0160:
	_code_00192170 (0000)
001922D0 00b0:
	_code_001922d0 (0000)
00192380 0080:
	_code_00192380 (0000)
00192400 0200:
	_code_00192400 (0000)
00192600 01b0:
	_biped_build_flying_axes (0000)
001927B0 1500:
	_code_001927b0 (0000)
00193CB0 0550:
	_biped_snap_facing (0000)
00194200 00c0:
	_biped_new (0000)
001942C0 0020:
	_biped_preprocess_node_orientations (0000)
001942E0 01e0:
	_biped_accelerate (0000)
001944C0 06b0:
	_code_001944c0 (0000)
00194B70 0ff0:
	_code_00194b70 (0000)
00195B60 00d0:
	_code_00195b60 (0000)
00195C30 0460:
	_biped_update (0000)
002A30B0 0144:
	_rdata_002a30b0 (0000)
002A31F4 000d:
	??_C@_0N@HOJCMADM@biped_update?$AA@ (0000)
002A3208 0043:
	??_C@_0ED@IAGIKAMA@WARNING?3?5biped?5?$CFs?5?$CI?$CFs?$CJ?5is?5in?5a?5b@ (0000)
002A324C 0004:
	__real@c4fa0000 (0000)
002A3250 0039:
	??_C@_0DJ@HLOPCOPC@WARNING?3?5biped?5?$CFs?5?$CI?$CFs?$CJ?5fell?5outs@ (0000)
002A328C 001e:
	??_C@_0BO@EMCOEGJH@c?3?2halo?2SOURCE?2units?2bipeds?4c?$AA@ (0000)
002A32AC 0025:
	??_C@_0CF@NNLMJPLM@desired_facing?5?$CG?$CG?5desired_gun_of@ (0000)
002A32D8 004b:
	??_C@_0EL@JGLDGCHJ@?$CIestimate_mode?5?$CB?$DN?5_unit_estimate@ (0000)
002A3328 004c:
	??_C@_0EM@DCHFFGOM@?$CIestimate_mode?5?$DN?$DN?5_unit_estimate@ (0000)
002A3374 0023:
	??_C@_0CD@INFHIELC@fixed_location?4cluster_index?$CB?$DNNO@ (0000)
002A3398 0021:
	??_C@_0CB@NBHLJDN@final_position?5?$HM?$HM?5?$CBdont_teleport@ (0000)
002A33BC 0012:
	??_C@_0BC@BGKMEKLE@pathfinding_point?$AA@ (0000)
002A33D0 003f:
	??_C@_0DP@JMJFKKMO@couldn?8t?5teleport?5the?5biped?5out?5@ (0000)
002A3410 0004:
	__real@44e10000 (0000)
002A3414 0004:
	__real@42700000 (0000)
002A3418 0042:
	??_C@_0EC@JBODJGAG@biped?5?$CFs?5?$CI?$CFs?$CFs?$CFs?$CJ?3?5?$CFs?5vector?5fai@ (0000)
002A345C 0006:
	??_C@_05MNHFFADL@climb?$AA@ (0000)
002A3464 000f:
	??_C@_0P@MMKEDIOE@player?9physics?$AA@ (0000)
002A3474 0006:
	??_C@_05JAAJDALH@?1dead?$AA@ (0000)
002A347C 0009:
	??_C@_08LOLHEFGJ@?1limping?$AA@ (0000)
002A3488 000e:
	??_C@_0O@HAMEHAGE@post?9airborne?$AA@ (0000)
002A3498 0033:
	??_C@_0DD@FLLKBPKC@TEST_FLAG?$CIbiped?9?$DObiped?4flags?0?5_b@ (0000)
002A34CC 000d:
	??_C@_0N@CBHHKMKF@post?9landing?$AA@ (0000)
002A34DC 000e:
	??_C@_0O@MGELMNBM@post?9slipping?$AA@ (0000)
002A34EC 0004:
	__real@3f36db6e (0000)
002A34F0 0053:
	??_C@_0FD@MEMEKANL@?$CFs?0?5?$CFs?0?5?$CFs?3?5assert_valid_real_ve@ (0000)
002A3544 000f:
	??_C@_0P@EFHIAMDK@forward_vector?$AA@ (0000)
002A3554 000c:
	??_C@_0M@MGINPDCA@left_vector?$AA@ (0000)
002A3560 000a:
	??_C@_09MDHMCNCN@up_vector?$AA@ (0000)
002A356C 002b:
	??_C@_0CL@JEOBBAMK@forward_vector?5?$CG?$CG?5left_vector?5?$CG?$CG@ (0000)
002A3598 0017:
	??_C@_0BH@LOKJEHJA@?$CGphysics?9?$DOnew_velocity?$AA@ (0000)
002A35B0 0017:
	??_C@_0BH@NMIMKOHN@?$CGphysics?9?$DOnew_position?$AA@ (0000)
002A35C8 0004:
	__real@322bcc76 (0000)
002A35CC 0004:
	__real@3d5a740e (0000)
002A35D0 0013:
	??_C@_0BD@LFOFBHEC@?$CGphysics?9?$DOvelocity?$AA@ (0000)
002A35E4 0013:
	??_C@_0BD@NHMAPOKP@?$CGphysics?9?$DOposition?$AA@ (0000)
002A35F8 0017:
	??_C@_0BH@OMEJIIDE@post?9normal?9snapfacing?$AA@ (0000)
002A3610 001a:
	??_C@_0BK@LJFJPFOM@post?9deadplane?9snapfacing?$AA@ (0000)
002A362C 0019:
	??_C@_0BJ@LPEPIIHF@pre?9deadplane?9snapfacing?$AA@ (0000)
002A3648 0016:
	??_C@_0BG@BILNKKA@post?9climb?9snapfacing?$AA@ (0000)
002A3660 0015:
	??_C@_0BF@BJAPGJCB@post?9bank?9snapfacing?$AA@ (0000)
002A3678 0017:
	??_C@_0BH@HMDDCPPK@?$CGbiped?9?$DOobject?4forward?$AA@ (0000)
002A3690 0011:
	??_C@_0BB@HNGPOHPO@preprocess?9nodes?$AA@ (0000)
002A36A4 000b:
	??_C@_0L@LGBFNPNE@post?9accel?$AA@ (0000)
002A36B0 000a:
	??_C@_09LBPFKLCO@pre?9accel?$AA@ (0000)
002A36BC 0011:
	??_C@_0BB@EHADHKAI@post?9moving?9turn?$AA@ (0000)
002A36D0 0013:
	??_C@_0BD@LNBPGJPJ@post?9standing?9turn?$AA@ (0000)
002A36E4 000e:
	??_C@_0O@NMGDOCHP@post?9fly?9turn?$AA@ (0000)
002A36F4 0004:
	__real@35b5b61c (0000)
002A36F8 0004:
	__real@3bef2eb7 (0000)
002A36FC 000b:
	??_C@_0L@EMJEBHAK@post?9dying?$AA@ (0000)
002A3708 0014:
	??_C@_0BE@OBCJD@post?9dying?9airborne?$AA@ (0000)
002A371C 0011:
	??_C@_0BB@JBPNJIHD@post?9limp?9noodle?$AA@ (0000)
002A3730 000c:
	??_C@_0M@KACGONJH@post?9update?$AA@ (0000)
002A373C 0018:
	??_C@_0BI@KEKHPNBM@post?9dead?1air?1land?1slip?$AA@ (0000)
002A3754 000c:
	??_C@_0M@PKIECHFJ@post?9moving?$AA@ (0000)
002A3760 000d:
	??_C@_0N@CDBMOEKH@post?9turning?$AA@ (0000)
002A3770 000c:
	??_C@_0M@JKCHFPFF@pre?9turning?$AA@ (0000)
002A377C 000b:
	??_C@_0L@PNEJBGOB@pre?9update?$AA@ (0000)
00314F50 0600:
	_rider_ejection (0000)
004CCB10 0004:
	_debug_biped_physics (0000)
	_debug_biped_skip_update (0001)
	_debug_biped_skip_collision (0002)
	_debug_biped_limp_body_disable (0003)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "models/model_animation_definitions.h"
#include "units/biped_limp_noodle.h"
#include "units/biped_definitions.h"
#include "units/bipeds.h"
#include "physics/breakable_surfaces.h"
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "physics/physics.h"
#include "render/render_debug.h"
#include "scenario/scenario.h"

/* ---------- constants */

enum
{
	_biped_airborne_bit = 0,
	_biped_limp_body_physics_active_bit = 5,
};

/* ---------- macros */

#define BIPED_CLIMBING_SNAP_ANGLE ((real)(10.0*M_PI/180.0))

/* ---------- structures */

struct scenario_object_datum
{
	short palette_entry_index;
	short name_index;
	word placement_flags;
	short variant_number;
	real_point3d position;
	real_euler_angles3d rotation;
	word on_bsp_flags;
	word misc_flags;
	unsigned long unused;
};

struct scenario_object_permutation
{
	unsigned long change_colors[4];
	byte region_permutations[8];
	unsigned long unused[2];
};

struct scenario_unit_datum
{
	real body_vitality;
	unsigned long flags;
};

struct scenario_biped_datum
{
	struct scenario_object_datum object;
	struct scenario_object_permutation permutation;
	struct scenario_unit_datum unit;
};

/* ---------- globals */

boolean debug_biped_physics;
boolean debug_biped_skip_update;
boolean debug_biped_skip_collision;
boolean debug_biped_limp_body_disable;

extern boolean debug_objects_biped_autoaim_pills;
extern boolean debug_objects_biped_physics_pills;

/* ---------- public code */

char animation_left_foot_frame_index(
	struct animation const *animation)
{
	return animation->private_left_foot_frame_index;
}

char animation_right_foot_frame_index(
	struct animation const *animation)
{
	return animation->private_right_foot_frame_index;
}

void bipeds_initialize(
	void)
{
	return;
}
void bipeds_initialize_for_new_map(
	void)
{
	return;
}

void bipeds_dispose_from_old_map(
	void)
{
	return;
}

void bipeds_dispose(
	void)
{
	return;
}

void biped_place(
	long biped_index,
	struct scenario_object_datum *scenario_object)
{
	struct scenario_biped_datum *scenario_biped = (struct scenario_biped_datum *)scenario_object;

	unit_place(biped_index, &scenario_biped->unit);
	object_add_scenario_permutation(biped_index, &scenario_biped->permutation);

	return;
}

void biped_delete(
	long biped_index)
{
	return;
}

void biped_reset(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	csmemset(&biped->biped, 0, sizeof(biped->biped));
	biped->biped.ground_plane = depths_of_hell;
	biped->biped.last_falling_communication_time = NONE;

	return;
}

void biped_disconnect_from_structure_bsp(
	long biped_index)
{
	struct biped_datum *biped;

	biped = biped_get(biped_index);
	biped->biped.support_surface_index = NONE;
	biped->biped.pathfinding_surface_index = NONE;
	biped->biped.last_pathfinding_surface_index = NONE;

	return;
}

void biped_get_physics_pill(
	long biped_index,
	real_point3d *base,
	real *height,
	real *width)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);

	object_get_origin(biped_index, base);
	if (!TEST_FLAG(definition->biped.flags, _biped_pill_centered_at_origin_bit))
		base->z += definition->biped.collision_radius;

	if (!TEST_FLAG(definition->biped.flags, _biped_spherical_bit) &&
		(biped->unit.player_index!=NONE ||
		TEST_FLAG(biped->object.flags, _object_movie_star_bit)))
	{
		*height = definition->biped.collision_height_standing +
			(definition->biped.collision_height_crouching -
			definition->biped.collision_height_standing)*biped->biped.crouch -
			2.f*definition->biped.collision_radius;
	}
	else
	{
		*height = 0.f;
	}
	*width = definition->biped.collision_radius;

	return;
}

void biped_stop_melee_attack(
	long biped_index)
{
	struct biped_datum *biped;

	biped = biped_try_and_get(biped_index);
	if (biped)
		biped->biped.player_melee_ticks = 0;

	return;
}

void biped_start_limp_body_physics(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	boolean uses_limp_body_physics = TEST_FLAG(
		biped_definition_get(biped->definition_index)->biped.flags,
		_biped_uses_limp_body_physics_bit);

	if (!debug_biped_limp_body_disable &&
		uses_limp_body_physics &&
		TEST_FLAG(biped->object.flags, _object_at_rest_bit) &&
		!TEST_FLAG(biped->biped.flags, _biped_airborne_bit) &&
		!TEST_FLAG(biped->biped.flags, _biped_limp_body_physics_active_bit))
	{
		biped->biped.limp_body_current_relaxation_iterations = 0;
		biped->biped.limp_body_max_relaxation_iterations =
			(byte)biped_limp_noodle_get_max_relaxation_iterations(biped_index);
		SET_FLAG(biped->object.flags, _object_do_not_recompute_node_matrices_bit, TRUE);
		SET_FLAG(biped->biped.flags, _biped_limp_body_physics_active_bit, TRUE);
	}

	return;
}

void biped_stop_limp_body_physics(
	long biped_index)
{
	struct biped_datum *biped;
	struct biped_definition *definition;

	biped = biped_get(biped_index);
	definition = biped_definition_get(biped->definition_index);
	if (TEST_FLAG(definition->biped.flags, _biped_uses_limp_body_physics_bit) &&
		TEST_FLAG(biped->biped.flags, _biped_limp_body_physics_active_bit))
	{
		SET_FLAG(biped->object.flags, _object_do_not_recompute_node_matrices_bit, FALSE);
		SET_FLAG(biped->biped.flags, _biped_limp_body_physics_active_bit, FALSE);
	}

	return;
}

boolean biped_flying_through_air(
	long biped_index)
{
	struct biped_datum *biped;
	struct biped_definition *definition;

	biped = biped_get(biped_index);
	definition = biped_definition_get(biped->definition_index);

	return biped->biped.airborne_ticks>3 &&
		(!TEST_FLAG(definition->biped.flags, _biped_flying_bit) ||
		TEST_FLAG(biped->object.damage_flags, _object_dead_bit));
}

void biped_export_function_values(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *biped_definition = biped_definition_get(biped->definition_index);
	real *function_values = biped->object.incoming_function_values;
	short *function_modes = biped_definition->biped.function_modes;
	long function_index;

	for (function_index = NUMBER_OF_INCOMING_OBJECT_FUNCTIONS; function_index; --function_index, ++function_modes, ++function_values)
	{
		if (*function_modes!=_biped_function_none)
		{
			real function_value = 0.0f;

			switch (*function_modes)
			{
			case _biped_function_flying_speed:
				function_value = magnitude3d(&biped->object.translational_velocity)/(biped_definition->biped.flying_velocity/(real)TICKS_PER_SECOND);
				function_value = PIN(function_value, 0.0f, 1.0f);
				break;
			}

			*function_values = function_value;
		}
	}

	return;
}

void biped_exit_seat_end(
	long biped_index,
	long seat_object_index)
{
	struct biped_datum *biped = biped_get(biped_index);

	biped_definition_get(biped->definition_index);

	biped->object.forward.k = 0.f;
	if (normalize3d(&biped->object.forward)==0.f)
		biped->object.forward = *global_forward3d;
	biped->object.up = *global_up3d;
	SET_FLAG(biped->biped.flags, _biped_airborne_bit, TRUE);

	if (!biped_fix_position(biped_index, seat_object_index, NULL, 0, 2.f, TRUE, FALSE, TRUE))
	{
		real_point3d center;
		real radius;

		object_get_bounding_sphere(seat_object_index, &center, &radius);
		if (!biped_fix_position(biped_index, seat_object_index, &center, 0, radius, TRUE, FALSE, FALSE))
			error(_error_silent, "couldn't teleport the biped out far enough from the vehicle...");
	}

	return;
}

void biped_render_debug(
	long biped_index)
{
	real_point3d base;
	real_vector3d height_vector;
	real height;
	real width;

	if (debug_objects_biped_physics_pills)
	{
		biped_get_physics_pill(biped_index, &base, &height, &width);
		if (height>0.f)
		{
			scale_vector3d(global_up3d, height, &height_vector);
			render_debug_pill(TRUE, &base, &height_vector, width, global_real_argb_white);
		}
		else
		{
			render_debug_sphere(TRUE, &base, width, global_real_argb_white);
		}
	}

	if (debug_objects_biped_autoaim_pills)
	{
		biped_get_autoaim_pill(biped_index, &base, &height_vector, &width);
		if (magnitude_squared3d(&height_vector)>0.f)
			render_debug_pill(TRUE, &base, &height_vector, width, global_real_argb_red);
		else
			render_debug_sphere(TRUE, &base, width, global_real_argb_red);
	}

	return;
}

void biped_build_flying_axes(
	real_vector3d const *forward_vector,
	real_vector3d *left_vector,
	real_vector3d *up_vector)
{
	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 2993, forward_vector && left_vector && up_vector);

	*up_vector = *global_up3d;
	cross_product3d(up_vector, forward_vector, left_vector);
	if (normalize3d(left_vector)==0.f)
	{
		*up_vector = *global_forward3d;
		cross_product3d(up_vector, forward_vector, left_vector);
		normalize3d(left_vector);
	}

	cross_product3d(forward_vector, left_vector, up_vector);
	normalize3d(up_vector);

	match_vassert("c:\\halo\\SOURCE\\units\\bipeds.c", 3008,
		valid_real_vector3d_axes3(forward_vector, left_vector, up_vector),
		csprintf(
			temporary,
			"%s, %s, %s: assert_valid_real_vector3d_axes3(%f, %f, %f / %f, %f, %f / %f, %f, %f)",
			"forward_vector",
			"left_vector",
			"up_vector",
			forward_vector->i, forward_vector->j, forward_vector->k,
			up_vector->i, up_vector->j, up_vector->k,
			left_vector->i, left_vector->j, left_vector->k));

	return;
}

/* ---------- private code */

static long biped_find_ground_surface(
	long object_index,
	real_vector3d const *direction,
	real distance,
	real_point3d *point,
	real_vector3d *normal)
{
	struct biped_datum *biped = biped_get(object_index);
	struct collision_bsp *collision_bsp = global_collision_bsp_get();
	long surface_index = NONE;
	real_vector3d vector;
	real_point3d origin;
	struct collision_bsp_test_vector_result result;

	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 1146, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_bipeds;

	object_get_origin(object_index, &origin);
	/* Preserve January's inline schedule without owning point_from_line3d here. */
	origin.x = global_up3d->i*0.4f + origin.x;
	origin.y = global_up3d->j*0.4f + origin.y;
	origin.z = global_up3d->k*0.4f + origin.z;
	scale_vector3d(direction, distance, &vector);

	if (collision_bsp_test_vector(
		FLAG(_collision_test_front_facing_surfaces_bit),
		collision_bsp,
		0,
		NULL,
		&origin,
		&vector,
		REAL_MAX,
		&result))
	{
		surface_index = result.surface_index;
		if (point)
		{
			real_point3d const *line_point = &origin;
			real_vector3d const *line_vector = &vector;
			real line_t = result.t;

			point->x = line_vector->i*line_t + line_point->x;
			point->y = line_vector->j*line_t + line_point->y;
			point->z = line_vector->k*line_t + line_point->z;
		}
		if (normal)
			*normal = result.plane->n;
	}

	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 1168, global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return surface_index;
}

short biped_approximate_surface_index(
	long biped_index,
	real_point3d *point)
{
	return (short)biped_find_ground_surface(biped_index, global_down3d, 2.f, point, NULL);
}

long biped_find_pathfinding_surface_index(
	long biped_index,
	real_point3d *pathfinding_point)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);

	if (TEST_FLAG(definition->biped.flags, _biped_flying_bit) &&
		!TEST_FLAG(biped->object.damage_flags, _object_dead_bit))
	{
		biped->biped.pathfinding_surface_index = NONE;
		object_get_origin(biped_index, pathfinding_point);
	}
	else if (biped->biped.pathfinding_surface_index==NONE)
	{
		long time = game_time_get();

		if (time>biped->biped.last_pathfinding_attempt_time)
		{
			struct collision_bsp *collision_bsp = global_collision_bsp_get();
			real_point3d point = biped->biped.pathfinding_point;

			biped->biped.last_pathfinding_attempt_time = time;
			if (biped->biped.support_surface_index!=NONE)
			{
				real_point2d closest_point;

				collision_surface_find_closest_point2d(
					collision_bsp,
					biped->biped.support_surface_index,
					_z,
					TRUE,
					(real_point2d const *)&biped->biped.pathfinding_point,
					&closest_point);
				collision_surface_project_point2d(
					collision_bsp,
					biped->biped.support_surface_index,
					_z,
					TRUE,
					&closest_point,
					&point);
				biped->biped.pathfinding_surface_index = biped->biped.support_surface_index;
			}
			else if (biped->biped.last_pathfinding_surface_index!=NONE &&
				collision_surface_test_point2d(
					collision_bsp,
					biped->biped.last_pathfinding_surface_index,
					_z,
					TRUE,
					(real_point2d const *)&biped->biped.pathfinding_point))
			{
				biped->biped.pathfinding_surface_index = biped->biped.last_pathfinding_surface_index;
				collision_surface_project_point2d(
					collision_bsp,
					biped->biped.last_pathfinding_surface_index,
					_z,
					TRUE,
					(real_point2d const *)&biped->biped.pathfinding_point,
					&point);
				biped->biped.pathfinding_surface_index = biped->biped.last_pathfinding_surface_index;
			}

			if (biped->biped.pathfinding_surface_index==NONE)
				biped->biped.pathfinding_surface_index = biped_find_ground_surface(biped_index, global_down3d, 2.f, &point, NULL);

			if (biped->biped.pathfinding_surface_index!=NONE)
			{
				biped->biped.pathfinding_point = point;
				biped->biped.last_pathfinding_surface_index = biped->biped.pathfinding_surface_index;
			}
		}
	}

	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 1255, pathfinding_point);
	*pathfinding_point = biped->biped.pathfinding_point;

	return biped->biped.pathfinding_surface_index;
}

static void biped_verify_object_vectors(
	long biped_index,
	char const *state)
{
	struct biped_datum *biped = biped_get(biped_index);

	if (!valid_real_vector3d_axes2(&biped->object.forward, &biped->object.up))
	{
		struct biped_definition *definition = biped_definition_get(biped->definition_index);

		match_vassert("c:\\halo\\SOURCE\\units\\bipeds.c", 1373, FALSE,
			csprintf(
				temporary,
				"biped %s (%s%s%s): %s vector failure: (%f, %f, %f) / (%f, %f, %f)",
				tag_name_strip_path(tag_get_name(biped->definition_index)),
				TEST_FLAG(definition->biped.flags, _biped_flying_bit)
					? "flying"
					: TEST_FLAG(definition->biped.flags, _biped_uses_player_physics_bit)
						? "player-physics"
						: TEST_FLAG(definition->biped.flags, _biped_climbs_anything_bit)
							? "climb"
							: "normal",
				TEST_FLAG(biped->object.damage_flags, _object_dead_bit) ? "/dead" : "",
				TEST_FLAG(biped->biped.flags, _biped_limp_body_physics_active_bit) ? "/limping" : "",
				state,
				biped->object.forward.i, biped->object.forward.j, biped->object.forward.k,
				biped->object.up.i, biped->object.up.j, biped->object.up.k));
	}

	return;
}

void biped_preprocess_node_orientations(
	long biped_index,
	struct real_orientation *node_orientations)
{
	biped_verify_object_vectors(biped_index, "preprocess-nodes");

	return;
}

static void biped_find_nearby_support_surface(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct collision_bsp *collision_bsp = global_collision_bsp_get();
	real_point3d base;
	real height;
	real width;
	struct collision_bsp_test_sphere_result result;

	biped_get_physics_pill(biped_index, &base, &height, &width);

	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 3999, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_bipeds;

	if (collision_bsp_test_sphere(
		collision_bsp,
		MAXIMUM_BREAKABLE_SURFACES_PER_MAP,
		breakable_surface_flags_get(),
		&base,
		width+0.05f,
		&result))
	{
		long support_surface_index = NONE;
		real closest_distance = REAL_MAX;
		real_plane3d support_plane;
		short surface_index;

		for (surface_index = 0; surface_index<result.surface_count; ++surface_index)
		{
			long plane_designator = TAG_BLOCK_GET_ELEMENT(
				&collision_bsp->surfaces,
				result.surface_indices[surface_index],
				struct collision_surface)->plane_designator;
			real_plane3d const *plane = TAG_BLOCK_GET_ELEMENT(
				&collision_bsp->bsp3d.planes,
				plane_designator & LONG_MAX,
				real_plane3d);
			real_plane3d surface_plane;
			real distance;

			if (TEST_FLAG(plane_designator, 31))
			{
				surface_plane.n.i = -plane->n.i;
				surface_plane.n.j = -plane->n.j;
				surface_plane.n.k = -plane->n.k;
				surface_plane.d = -plane->d;
			}
			else
			{
				surface_plane = *plane;
			}

			distance = plane3d_distance_to_point(&surface_plane, &base);
			if (distance<closest_distance)
			{
				closest_distance = distance;
				support_surface_index = result.surface_indices[surface_index];
				support_plane = surface_plane;
			}
		}

		if (support_surface_index!=NONE)
		{
			biped->biped.support_surface_index = support_surface_index;
			biped->biped.ground_plane = support_plane;
			biped->object.up = support_plane.n;
		}
	}

	match_assert("c:\\halo\\SOURCE\\units\\bipeds.c", 4034, global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return;
}

void biped_snap_facing(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);

	if (TEST_FLAG(definition->biped.flags, _biped_flying_bit) &&
		!TEST_FLAG(biped->object.damage_flags, _object_dead_bit))
	{
		real_vector3d left;
		real_vector3d up;
		real sine;
		real cosine;

		match_assert_valid_real_normal3d("c:\\halo\\SOURCE\\units\\bipeds.c", 4051, &biped->object.forward);

		cross_product3d(&biped->object.forward, global_up3d, &left);
		cross_product3d(&left, &biped->object.forward, &up);
		if (normalize3d(&up)==0.f)
		{
			up = *global_forward3d;
			left = *global_left3d;
		}

		cosine = (real)cos(biped->biped.bank);
		sine = (real)sin(biped->biped.bank);
		up.i *= cosine;
		up.j *= cosine;
		up.k *= cosine;
		normalize3d(&left);
		biped->object.up.i = left.i*sine + up.i;
		biped->object.up.j = left.j*sine + up.j;
		biped->object.up.k = left.k*sine + up.k;

		biped_verify_object_vectors(biped_index, "post-bank-snapfacing");
	}
	else if (TEST_FLAG(definition->biped.flags, _biped_climbs_anything_bit) &&
		!TEST_FLAG(biped->object.damage_flags, _object_dead_bit))
	{
		real_vector3d normal;
		real_vector3d vector;
		real_vector3d axis;
		real_vector3d forward;

		if (biped->biped.support_surface_index==NONE)
		{
			normal = biped->object.up;
		}
		else
		{
			boolean tilt_toward_surface = TRUE;

			normal = biped->biped.ground_plane.n;
			cross_product3d(&biped->object.up, &normal, &axis);
			if (normalize3d(&axis)==0.f)
			{
				if (dot_product3d(&normal, &biped->object.up)>0.f)
					tilt_toward_surface = FALSE;
				else
					axis = biped->object.forward;
			}

			if (tilt_toward_surface)
			{
				real_vector3d tilted_up = biped->object.up;

				rotate_vector_about_axis(
					&tilted_up,
					&axis,
					(real)sin(BIPED_CLIMBING_SNAP_ANGLE),
					(real)cos(BIPED_CLIMBING_SNAP_ANGLE));
				cross_product3d(&tilted_up, &normal, &vector);
				if (dot_product3d(&axis, &vector)>0.f)
					normal = tilted_up;
			}
		}

		cross_product3d(&biped->object.forward, &normal, &vector);
		cross_product3d(&normal, &vector, &forward);
		if (normalize3d(&forward)==0.f)
		{
			cross_product3d(&normal, &biped->object.up, &vector);
			cross_product3d(&normal, &vector, &forward);
			if (normalize3d(&forward)==0.f)
			{
				normal = *global_up3d;
				forward = *global_forward3d;
			}
		}

		biped->object.forward = forward;
		biped->object.up = normal;

		biped_verify_object_vectors(biped_index, "post-climb-snapfacing");
	}
	else if (TEST_FLAG(biped->object.damage_flags, _object_dead_bit) &&
		!TEST_FLAG(biped->biped.flags, _biped_airborne_bit))
	{
		real cosine_of_angle;

		biped_verify_object_vectors(biped_index, "pre-deadplane-snapfacing");

		cosine_of_angle = dot_product3d(&biped->object.up, &biped->biped.ground_plane.n);
		if (!(fabs(cosine_of_angle - 1.f)<_real_epsilon))
		{
			real angle = (real)acos(cosine_of_angle);

			if (angle!=0.f)
			{
				real_vector3d axis;

				cross_product3d(&biped->object.up, &biped->biped.ground_plane.n, &axis);
				if (normalize3d(&axis)!=0.f)
				{
					real cosine = (real)cos(angle);
					real sine = (real)sin(angle);

					rotate_vector_about_axis(&biped->object.up, &axis, sine, cosine);
					rotate_vector_about_axis(&biped->object.forward, &axis, sine, cosine);
					normalize3d(&biped->object.up);
					normalize3d(&biped->object.forward);
				}
			}
		}

		biped_verify_object_vectors(biped_index, "post-deadplane-snapfacing");
	}
	else
	{
		biped->object.forward.k = 0.f;
		if (normalize3d(&biped->object.forward)==0.f)
			biped->object.forward = *global_forward3d;
		biped->object.up = *global_up3d;

		biped_verify_object_vectors(biped_index, "post-normal-snapfacing");
	}

	return;
}

boolean biped_new(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);

	biped->biped.ground_plane = depths_of_hell;
	biped->biped.jump_recovery_timer = 127;
	biped->biped.support_surface_index = NONE;
	biped->biped.pathfinding_surface_index = NONE;
	object_get_origin(biped_index, &biped->biped.pathfinding_point);
	biped->biped.last_pathfinding_surface_index = NONE;
	biped->biped.last_pathfinding_attempt_time = NONE;
	biped->biped.impact_target_object_index = NONE;
	if (TEST_FLAG(definition->biped.flags, _biped_climbs_anything_bit))
		biped_find_nearby_support_surface(biped_index);
	biped_snap_facing(biped_index);
	biped->biped.elevator_object_index = NONE;
	biped->biped.elevator_ticks = 0;

	return TRUE;
}
