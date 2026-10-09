/*
VR_RENDER.C

Stereo frames in the VR build (halo_vr.h): the eyes' windows and cameras,
built from the game's camera and the headset's pose (port/linux/src/vr.h).
*/

#ifdef HALO_VR

#include "cseries.h"
#include "math/real_math.h"
#include "cutscene/cinematics.h"
#include "render/render_cameras.h"
#include "game/players.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "game/game.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "physics/collision_features.h"
#include "camera/director.h"
#include "units/units.h"
#include "models/model_animation_definitions.h"
#include "units/unit_definitions.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"

#include "halo_vr.h"
#include "network_vr_pose.h"
#include "vr_action_blend.h"
#include "vr_gun_anchor.h"
#include "../src/vr.h"
#include "../src/port_config.h"

/* port/linux/src/platform.h (a variadic call needs its prototype in scope
on the Android guest's ABI) */
void platform_log(const char *format, ...);
/* networking/network_game_globals.c */
boolean network_game_distributed_client(void);
static boolean vr_body_setting(void);

int vr_render_pass = _vr_render_pass_none;

/* Script print text still reaches the run log, without a floating green
terminal overlay in VR. vr.script_messages restores the overlay for diagnosis. */
boolean vr_render_script_message(char const *message)
{
	if (!vr_active())
		return FALSE;
	platform_log("script: %s", message ? message : "");
	return !config_boolean("vr.script_messages");
}

static struct
{
	boolean stereo, cinema, cinematic_view;
	boolean cinematic_previous;
	real cinematic_heading, cinematic_head_yaw, cinematic_last_yaw;
	real_point3d cinematic_position, cinematic_last_position;
	/* what each window of this frame is, and which eye's image it
	completes (NONE: none) */
	short pass_of_window[5];
	short eye_of_window[5];
	/* a cutscene eye's frustum turned in, in the bounds' units */
	real cinema_shift[2];
	real_rectangle2d eye_bounds[2];
	/* the HUD pass is drawn from the head (head_camera), over the panel's
	field */
	boolean hud_from_head;
	real_rectangle2d hud_bounds;
	/* the head's yaw (radians), for the motion sensor */
	boolean head_yaw_valid;
	real head_yaw;
	/* the scope's pass this frame: its sight (VR_SCOPE_*, 0 none), field and
	viewport */
	int scope_shape;
	real_rectangle2d scope_bounds;
	rectangle2d scope_viewport;
	/* the game's camera this frame, the head posed from it */
	struct render_camera head_camera;
	real_point3d game_camera_position;
	/* the camera the first-person weapon was posed from this frame */
	struct render_camera weapon_camera;
	boolean weapon_camera_valid;
	/* how far the camera was pulled back out of a wall this frame
	(weapon_out_of_walls); the anchored gun keeps it */
	real_vector3d weapon_pullback;
	/* the local player's seat (vr_update_seat): in a vehicle, its kind, and
	the heading the view turns with (the vehicle's, plus the seat's own turn
	from it) */
	struct
	{
		boolean seated, driver, gunner;
		long unit_index, vehicle_index;
		short seat_index;
		real offset, heading;
	} seat;
	/* where the hand's shots start (vr_render_hand_origin) */
	/* test22: the reticle's ray this frame (where the next shot goes) */
	real_point3d reticle_origin;
	real_vector3d reticle_direction;
	double reticle_time;
	boolean hand_origin_valid;
	long hand_origin_unit;
	real_point3d hand_origin;
} vr_render;

/* vr.vehicle_view and vr.vehicle_steering */
enum
{
	_vr_steering_stick,
	_vr_steering_head,
	_vr_steering_right,
	_vr_steering_left,
};

static char const *vr_vehicle_profile(char const *name);

static boolean vr_first_person_vehicles(
	void)
{
	static int first_person = -1, generation = -1;

	/* (read again when the pause menu changes it) */
	if (first_person < 0 || generation != vr_settings_generation())
	{
		first_person = strcmp(config_string("vr.vehicle_view"), "first_person") == 0;
		generation = vr_settings_generation();
	}
	return first_person && vr_active();
}

static int vr_vehicle_steering(
	void)
{
	static int steering = -1, generation = -1;

	if (steering < 0 || generation != vr_settings_generation())
	{
		generation = vr_settings_generation();
		char const *setting = config_string("vr.vehicle_steering");

		steering = !strcmp(setting, "head") ? _vr_steering_head :
			!strcmp(setting, "left") ? _vr_steering_left :
			!strcmp(setting, "stick") ? _vr_steering_stick : _vr_steering_right;
	}
	return steering;
}

/* Mounted guns have their own input choice; driver steering is unchanged. */
static int vr_turret_aim(void)
{
	static int aim = -1, generation = -1;
	if (aim < 0 || generation != vr_settings_generation())
	{
		char const *setting = config_string("vr.turret_aim");
		generation = vr_settings_generation();
		aim = !strcmp(setting, "head") ? _vr_steering_head :
			!strcmp(setting, "left") ? _vr_steering_left :
			!strcmp(setting, "stick") ? _vr_steering_stick : _vr_steering_right;
	}
	return aim;
}

static int vr_vehicle_aim_source(void)
{
	static int previous_role = -1, previous_seat = NONE, previous_source = -99, previous_generation = -1;
	static long previous_vehicle = NONE;
	int role, mode, source, generation = vr_settings_generation();
	const char *setting, *effective;

	if (!vr_render.seat.seated)
	{
		previous_role = 0;
		previous_seat = NONE;
		previous_vehicle = NONE;
		previous_source = -99;
		previous_generation = generation;
		return 1; /* existing handheld-weapon aim */
	}
	role = vr_render.seat.driver ? 1 : vr_render.seat.gunner ? 2 : 3;
	if (role == 3)
	{
		mode = _vr_steering_head;
		setting = "native passenger head aim";
		source = 0;
	}
	else
	{
		mode = role == 1 ? vr_vehicle_steering() : vr_turret_aim();
		setting = config_string(role == 1 ? "vr.vehicle_steering" : "vr.turret_aim");
		source = mode == _vr_steering_stick ? -1 : mode == _vr_steering_right ? 2 :
			mode == _vr_steering_left ? 3 : 0;
	}
	effective = source == -1 ? "native stick" : source == 2 ? "right controller" :
		source == 3 ? "left controller" : "head";
	if (role != previous_role || vr_render.seat.seat_index != previous_seat ||
		vr_render.seat.vehicle_index != previous_vehicle || source != previous_source || generation != previous_generation)
	{
		static const char *const roles[] = { "on foot", "driver", "gunner/turret", "passenger" };
		platform_log("vr: mounted aim selection: role %s, seat %d, vehicle object %ld; setting %s, effective source %s (%d)",
			roles[role], vr_render.seat.seat_index, vr_render.seat.vehicle_index, setting, effective, source);
		previous_role = role;
		previous_seat = vr_render.seat.seat_index;
		previous_vehicle = vr_render.seat.vehicle_index;
		previous_source = source;
		previous_generation = generation;
	}
	return source;
}

static real vr_yaw(
	real_vector3d const *forward)
{
	return (real)atan2(forward->j, forward->i);
}

/* vr.diag_drive_seconds: this long into play, the local player is seated
as the driver of the nearest vehicle (for checking vehicles unattended;
debug.network_test_vehicle does the same in network tests) */
static void vr_diag_drive(
	long unit_index)
{
	static real seconds = -1.0f;
	static long first_tick = NONE;
	struct object_iterator vehicles;
	long nearest_index = NONE;
	real nearest_distance = 0.0f;
	short seat_index;

	if (seconds < 0.0f)
		seconds = (real)config_real("vr.diag_drive_seconds");
	if (seconds <= 0.0f || unit_index == NONE || object_get(unit_index)->object.parent_object_index != NONE)
		return;
	if (first_tick == NONE)
		first_tick = game_time_get();
	if (game_time_get() - first_tick < (long)(seconds * TICKS_PER_SECOND))
		return;
	seconds = 0.0f;
	object_iterator_new(&vehicles, _object_mask_vehicle, 0);
	while (object_iterator_next(&vehicles))
	{
		real distance = distance_squared3d(&object_get(unit_index)->object.position,
			&object_get(vehicles.index)->object.position);

		if (nearest_index == NONE || distance < nearest_distance)
		{
			nearest_index = vehicles.index;
			nearest_distance = distance;
		}
	}
	if (nearest_index == NONE)
	{
		platform_log("vr: diag drive: no vehicle");
		return;
	}
	for (seat_index = 0; seat_index < unit_definition_get(object_get(nearest_index)->definition_index)->unit.seats.count;
		seat_index++)
	{
		if (unit_seat_is_driver(nearest_index, seat_index) && unit_enter_seat(unit_index, nearest_index, seat_index))
		{
			platform_log("vr: diag drive: driving vehicle %lx", nearest_index);
			return;
		}
	}
	platform_log("vr: diag drive: cannot drive vehicle %lx", nearest_index);
}

/* the local player's seat this frame */
static void vr_update_seat(
	long unit_index)
{
	struct unit_datum *unit = NULL, *vehicle = NULL;
	struct unit_definition *definition;
	long vehicle_index = NONE;
	short seat_index = NONE;

	/* Co-op replication and map teardown may invalidate either salted handle
	between the input tick and the render. Validate before inspecting a tag. */
	if (object_header_data && object_header_data->valid && unit_index != NONE)
		unit = unit_try_and_get(unit_index);
	if (unit)
	{
		vehicle_index = unit->object.parent_object_index;
		seat_index = unit->unit.parent_seat_index;
		if (vehicle_index != NONE && seat_index >= 0)
			vehicle = unit_try_and_get(vehicle_index);
	}
	definition = vehicle ? unit_definition_get(vehicle->definition_index) : NULL;
	if (!definition || seat_index < 0 || seat_index >= definition->unit.seats.count)
	{
		vr_render.seat.seated = vr_render.seat.driver = vr_render.seat.gunner = FALSE;
		vr_render.seat.unit_index = unit ? unit_index : NONE;
		vr_render.seat.vehicle_index = NONE;
		vr_render.seat.seat_index = NONE;
		vr_vehicle_seat(unit ? unit_index : NONE, NONE, NONE, 0);
		return;
	}
	if (!vr_render.seat.seated || vr_render.seat.vehicle_index != vehicle_index ||
		vr_render.seat.seat_index != seat_index || vr_render.seat.unit_index != unit_index)
	{
		struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(&definition->unit.seats, seat_index, struct unit_seat);
		unsigned long flags = seat->flags;
		vr_render.seat.driver = TEST_FLAG(flags, _unit_seat_driver_bit);
		vr_render.seat.gunner = TEST_FLAG(flags, _unit_seat_gunner_bit);
		/* A passenger may face sideways relative to the vehicle. Keep the
		seat's authored entry orientation; headset motion never rewrites it. */
		vr_render.seat.offset = vr_render.seat.driver || vr_render.seat.gunner ? 0.0f :
			vr_yaw(&unit->object.forward) - vr_yaw(&vehicle->object.forward);
		vr_render.seat.vehicle_index = vehicle_index;
		vr_render.seat.seat_index = seat_index;
		vr_render.seat.unit_index = unit_index;
		platform_log("vr: seat: %s seat %d as %s, vehicle yaw %.1f, seat offset %.1f, view %s",
			tag_get_name(vehicle->definition_index), seat_index,
			vr_render.seat.driver ? "driver" : vr_render.seat.gunner ? "gunner" : "passenger",
			vr_yaw(&vehicle->object.forward) * 57.29578f, vr_render.seat.offset * 57.29578f,
			vr_first_person_vehicles() ? "first person" : "third person");
	}
	vr_render.seat.seated = TRUE;
	vr_render.seat.heading = vr_yaw(&vehicle->object.forward) + vr_render.seat.offset;
	vr_vehicle_seat(unit_index, vehicle_index, seat_index,
		vr_render.seat.driver ? 1 : vr_render.seat.gunner ? 2 : 0);
}

/* Seat object handles belong to one map. A map can draw before the first
 * control tick refreshes this cache (notably after a Pelican cinematic). */
void vr_render_reset_vehicle_view(void)
{
	vr_head_look_reset();
	if (vr_render.seat.seated)
		platform_log("vr: cleared vehicle camera cache");
	csmemset(&vr_render.seat, 0, sizeof(vr_render.seat));
	vr_render.seat.unit_index = NONE;
	vr_render.seat.vehicle_index = NONE;
	vr_render.seat.seat_index = NONE;
	vr_vehicle_seat(NONE, NONE, NONE, 0);
}

/* the view rides in the seat (vr.vehicle_view "first_person") */
static boolean vr_seat_view(
	void)
{
	struct unit_datum *unit, *vehicle;
	if (!vr_render.seat.seated || !vr_first_person_vehicles())
		return FALSE;
	/* try-and-get still requires a valid pool. Do not ask it during teardown.
	 * Validate both salted handles and their relationship before the strict
	 * marker lookup in view_anchor; object/seat deletion can occur mid-map too. */
	if (!object_header_data || !object_header_data->valid)
	{
		vr_render_reset_vehicle_view();
		return FALSE;
	}
	unit = unit_try_and_get(vr_render.seat.unit_index);
	vehicle = unit_try_and_get(vr_render.seat.vehicle_index);
	if (!unit || !vehicle || vr_render.seat.seat_index == NONE ||
		unit->object.parent_object_index != vr_render.seat.vehicle_index ||
		unit->unit.parent_seat_index != vr_render.seat.seat_index)
	{
		vr_render_reset_vehicle_view();
		return FALSE;
	}
	return TRUE;
}

/* port/linux/game/render_interpolation.c: an object's nodes as this frame
draws them (between the last two ticks), NULL outside a frame */
real_matrix4x3 *render_interpolation_object_node_matrices(long object_index);

/* test25: first-person seats: the see-through parts of the vehicle the
player sits in, as the renderer draws them (rasterizer_xbox_transparent_
geometry.c): each kind said once a vehicle; its glass not drawn from the
seat. The owner's video (Silent Cartographer, a Warthog) showed its
windshield as a bright white sheet from the driver's seat, a view the game
was never made for; outside the seat it is drawn as ever. The caller must
pass the draw's object_index; source_object_index is an effect owner and is
zero for ordinary surfaces, even when the real vehicle handle is valid. */
boolean vr_render_seat_transparent(long object_index, short shader_type, short glass_type)
{
	static char const *const names[] = { "screen", "effect", "decal", "environment", "model", "generic",
		"chicago", "water", "glass", "meter", "plasma" };
	static long logged_vehicle = NONE;
	static unsigned long logged_types;
	static int generation = -1;
	static boolean hide_warthog_glass = TRUE;
	boolean hide_glass;
	char const *profile;

	if (object_index == NONE || !vr_render.seat.seated || vr_render.seat.vehicle_index == NONE ||
		!vr_first_person_vehicles() || !vr_seat_view() || !object_try_and_get(object_index) ||
		(object_index != vr_render.seat.vehicle_index &&
			object_get_ultimate_parent(object_index) != vr_render.seat.vehicle_index))
	{
		return FALSE;
	}
	if (generation != vr_settings_generation())
	{
		generation = vr_settings_generation();
		hide_warthog_glass = config_boolean("vr.vehicle_warthog_hide_glass");
		logged_vehicle = NONE;
	}
	profile = vr_vehicle_profile(tag_get_name(object_get(vr_render.seat.vehicle_index)->definition_index));
	/* Test25 already hid seated glass. This option changes only Warthogs,
	including variants using that tag name, in the occupied first-person view. */
	hide_glass = !profile || strcmp(profile, "warthog") || hide_warthog_glass;
	if (logged_vehicle != vr_render.seat.vehicle_index)
	{
		logged_vehicle = vr_render.seat.vehicle_index;
		logged_types = 0;
	}
	if (shader_type >= 0 && shader_type < 32 && !(logged_types & (1ul << shader_type)))
	{
		logged_types |= 1ul << shader_type;
		platform_log("vr: first-person seat: the vehicle draws see-through %s parts%s",
			shader_type < (short)NUMBEROF(names) ? names[shader_type] : "other",
			shader_type == glass_type && hide_glass ? " (its glass is hidden from the seat)" : "");
	}
	return shader_type == glass_type && hide_glass;
}

/* test25: vr.vehicle_tilt: a first-person driver's view tilted with the
vehicle (pitch and roll; its heading is the seat's already), eased over
0.12 s against the jolts of rough ground and limited to 60 degrees, about
the eyes' anchor. Level (0) is the view as it was; gunners and passengers
keep it level, their aim being the head's */
static struct
{
	real_vector3d up;
	double time;
	long vehicle_index;
	boolean valid;
} vr_tilt;

static void vr_vehicle_tilt_view(
	real_point3d const *anchor,
	real_point3d *position,
	real_vector3d *forward,
	real_vector3d *up)
{
	real amount = vr_vehicle_tilt();
	struct object_datum *vehicle;
	real_matrix4x3 *nodes;
	real_vector3d target, tilted, axis, *vectors[3], offset;
	double now;
	real c, s;
	int i;

	if (!(amount > 0.0f) || !vr_seat_view() || !vr_render.seat.driver ||
		!(vehicle = object_try_and_get(vr_render.seat.vehicle_index)))
	{
		vr_tilt.valid = FALSE;
		return;
	}
	nodes = render_interpolation_object_node_matrices(vr_render.seat.vehicle_index);
	target = nodes ? nodes[0].up : vehicle->object.up;
	if (!(normalize3d(&target) > 0.0f))
		return;
	now = vr_pose_time();
	if (!vr_tilt.valid || vr_tilt.vehicle_index != vr_render.seat.vehicle_index)
	{
		vr_tilt.up = target;
		vr_tilt.vehicle_index = vr_render.seat.vehicle_index;
		vr_tilt.valid = TRUE;
	}
	else if (now != vr_tilt.time)
	{
		double seconds = now - vr_tilt.time;
		real t = (real)(1.0 - exp(-(seconds > 0.0 && seconds < 0.5 ? seconds : 0.0) / 0.12));

		vr_tilt.up.i += (target.i - vr_tilt.up.i) * t;
		vr_tilt.up.j += (target.j - vr_tilt.up.j) * t;
		vr_tilt.up.k += (target.k - vr_tilt.up.k) * t;
		if (!(normalize3d(&vr_tilt.up) > 0.0f))
			vr_tilt.up = target;
	}
	vr_tilt.time = now;
	/* the share of the tilt: between straight up and the vehicle's up */
	tilted.i = vr_tilt.up.i * amount;
	tilted.j = vr_tilt.up.j * amount;
	tilted.k = 1.0f + (vr_tilt.up.k - 1.0f) * amount;
	if (!(normalize3d(&tilted) > 0.0f))
		return;
	/* about the axis up x tilted, by the angle between them (Rodrigues) */
	axis.i = -tilted.j;
	axis.j = tilted.i;
	axis.k = 0.0f;
	s = (real)sqrt(axis.i * axis.i + axis.j * axis.j);
	c = tilted.k;
	if (s < 0.0001f)
		return;
	axis.i /= s;
	axis.j /= s;
	if (c < 0.5f)
	{
		c = 0.5f;
		s = 0.8660254f;
	}
	offset.i = position->x - anchor->x;
	offset.j = position->y - anchor->y;
	offset.k = position->z - anchor->z;
	vectors[0] = &offset;
	vectors[1] = forward;
	vectors[2] = up;
	for (i = 0; i < 3; i++)
	{
		real_vector3d v = *vectors[i], cross;
		real along = axis.i * v.i + axis.j * v.j + axis.k * v.k;

		cross.i = axis.j * v.k - axis.k * v.j;
		cross.j = axis.k * v.i - axis.i * v.k;
		cross.k = axis.i * v.j - axis.j * v.i;
		vectors[i]->i = v.i * c + cross.i * s + axis.i * along * (1.0f - c);
		vectors[i]->j = v.j * c + cross.j * s + axis.j * along * (1.0f - c);
		vectors[i]->k = v.k * c + cross.k * s + axis.k * along * (1.0f - c);
	}
	position->x = anchor->x + offset.i;
	position->y = anchor->y + offset.j;
	position->z = anchor->z + offset.k;
}

/* test22: the seat's heading as this frame draws the vehicle. The seat's
heading (vr_update_seat) is taken in the 30 Hz control tick from the
vehicle's latest orientation, while the vehicle is drawn between its last
two ticks: turning, the eyes stepped ahead of the drawn interior each tick
and its frame shook against the view. The latest heading is turned by how
far the drawn root node's yaw is behind the simulated one's, so the view
turns with the vehicle as drawn, at the headset's rate */
static real vr_seat_frame_heading(
	void)
{
	struct object_datum *vehicle;
	real_matrix4x3 *drawn, *simulated;
	real heading, behind;

	if (vr_render.seat.vehicle_index == NONE || !(vehicle = object_try_and_get(vr_render.seat.vehicle_index)))
		return vr_render.seat.heading;
	heading = vr_yaw(&vehicle->object.forward) + vr_render.seat.offset;
	drawn = render_interpolation_object_node_matrices(vr_render.seat.vehicle_index);
	if (!drawn || vehicle->object.node_matrices.size < (long)sizeof(real_matrix4x3))
		return heading;
	simulated = (real_matrix4x3 *)object_header_block_get(vr_render.seat.vehicle_index, &vehicle->object.node_matrices);
	if (!simulated)
		return heading;
	behind = vr_yaw(&drawn[0].forward) - vr_yaw(&simulated[0].forward);
	while (behind > _pi)
		behind -= 2.0f * _pi;
	while (behind < -_pi)
		behind += 2.0f * _pi;
	return isfinite(behind) && fabsf(behind) < 0.5f ? heading + behind : heading;
}

/* test22: the seat's anchor held to the vehicle as drawn. The player's head
marker moves with the driver's animations (steering, bumps) as well as
with the vehicle; in the vehicle's own frame (its drawn root node) it is
eased over a quarter second, then carried back with the vehicle, so the
interior stays put against the view while the vehicle's own motion (its
bounce, its turns) is kept whole. A new seat, a gap or a jump of half a
metre takes the head's place at once */
static void vr_seat_steady_anchor(
	real_point3d *anchor)
{
	static struct
	{
		long vehicle_index, unit_index;
		short seat_index;
		real local[3];
		double time;
		boolean valid;
	} steady;
	struct object_datum *vehicle;
	real_matrix4x3 *nodes, root;
	real_vector3d relative;
	real local[3];
	real *axes[3];
	double now = vr_pose_time();
	real snap = 0.5f * vr_units_per_metre();
	int axis;

	if (vr_render.seat.vehicle_index == NONE || !(vehicle = object_try_and_get(vr_render.seat.vehicle_index)) || vehicle->object.node_matrices.size < (long)sizeof(real_matrix4x3) ||
		!(nodes = object_get_node_matrices(vr_render.seat.vehicle_index)))
	{
		steady.valid = FALSE;
		return;
	}
	root = nodes[0];
	axes[0] = root.forward.n;
	axes[1] = root.left.n;
	axes[2] = root.up.n;
	relative.i = anchor->x - root.position.x;
	relative.j = anchor->y - root.position.y;
	relative.k = anchor->z - root.position.z;
	for (axis = 0; axis < 3; axis++)
		local[axis] = relative.i * axes[axis][0] + relative.j * axes[axis][1] + relative.k * axes[axis][2];
	if (!isfinite(local[0]) || !isfinite(local[1]) || !isfinite(local[2]))
	{
		steady.valid = FALSE;
		return;
	}
	if (!steady.valid || steady.vehicle_index != vr_render.seat.vehicle_index ||
		steady.unit_index != vr_render.seat.unit_index || steady.seat_index != vr_render.seat.seat_index ||
		now < steady.time || now - steady.time > 0.5 ||
		(local[0] - steady.local[0]) * (local[0] - steady.local[0]) +
		(local[1] - steady.local[1]) * (local[1] - steady.local[1]) +
		(local[2] - steady.local[2]) * (local[2] - steady.local[2]) > snap * snap)
	{
		memcpy(steady.local, local, sizeof(local));
		steady.vehicle_index = vr_render.seat.vehicle_index;
		steady.unit_index = vr_render.seat.unit_index;
		steady.seat_index = vr_render.seat.seat_index;
		steady.valid = TRUE;
	}
	else if (now > steady.time)
	{
		real ease = 1.0f - (real)exp(-(now - steady.time) / 0.25);

		for (axis = 0; axis < 3; axis++)
			steady.local[axis] += (local[axis] - steady.local[axis]) * ease;
	}
	steady.time = now;
	for (axis = 0; axis < 3; axis++)
		anchor->n[axis] = root.position.n[axis] + root.forward.n[axis] * steady.local[0] +
			root.left.n[axis] * steady.local[1] + root.up.n[axis] * steady.local[2];
}

/* the heading the eyes turn from: the seat's in a vehicle seen from it,
the headset's own while the head or hand aims, otherwise the game
camera's */
static void view_heading(
	struct render_camera const *camera,
	real_vector3d *heading)
{
	if (vr_render.cinematic_view)
	{
		heading->i = (real)cos(vr_render.cinematic_heading);
		heading->j = (real)sin(vr_render.cinematic_heading);
		heading->k = 0.0f;
	}
	else if (vr_seat_view())
	{
		real seat_heading = vr_seat_frame_heading();

		heading->i = (real)cos(seat_heading);
		heading->j = (real)sin(seat_heading);
		heading->k = 0.0f;
	}
	else if (!vr_aiming() || !vr_heading_forward(heading->n))
	{
		*heading = camera->forward;
	}
}

static char const *vr_vehicle_profile(char const *name)
{
	static char const *profiles[] = { "warthog", "ghost", "banshee", "scorpion", "pelican" };
	unsigned int i;
	if (name) for (i = 0; i < sizeof(profiles) / sizeof(profiles[0]); i++)
		if (strstr(name, profiles[i])) return profiles[i];
	return NULL; /* Custom vehicles and turrets still get the global offsets. */
}

static real vr_vehicle_offset(char const *profile, char const *axis)
{
	char key[96];
	double global, local = 0.0;
	snprintf(key, sizeof(key), "vr.vehicle_all_%s", axis);
	global = config_real(key);
	if (profile) {
		snprintf(key, sizeof(key), "vr.vehicle_%s_%s", profile, axis);
		local = config_real(key);
	}
	if (!isfinite(global)) global = 0;
	if (!isfinite(local)) local = 0;
	return (real)fmax(-0.50, fmin(0.50, global + local));
}

static void vr_vehicle_adjust_anchor(real_point3d *anchor)
{
	struct unit_datum *vehicle = unit_try_and_get(vr_render.seat.vehicle_index);
	char const *profile;
	real up, forward, right, c, s, scale;
	real_vector3d offset;
	struct collision_result collision;
	if (!vehicle) return;
	profile = vr_vehicle_profile(tag_get_name(vehicle->definition_index));
	up = vr_vehicle_offset(profile, "up");
	forward = vr_vehicle_offset(profile, "forward");
	right = vr_vehicle_offset(profile, "right");
	{
		real seat_heading = vr_seat_frame_heading();

		c = (real)cos(seat_heading); s = (real)sin(seat_heading);
	}
	scale = vr_units_per_metre();
	offset.i = (forward * c + right * s) * scale;
	offset.j = (forward * s - right * c) * scale;
	offset.k = up * scale;
	/* These offsets cannot push the anchor through the map. The vehicle's
	 * own hull is intentionally excluded: this is its interior seat view. */
	if (dot_product3d(&offset, &offset) < 0.00000001f) return;
	if (collision_test_vector(FLAG(_collision_test_structure_bit), anchor, &offset,
		vr_render.seat.unit_index, &collision)) {
		real fraction = (real)fmax(0.0, collision.t - 0.02);
		scale_vector3d(&offset, fraction, &offset);
	}
	anchor->x += offset.i; anchor->y += offset.j; anchor->z += offset.k;
}

/* where the eyes are placed from: in a vehicle seen from its seat, the
player's head there (the game's camera for a seat is the chase camera's
place); otherwise the game's camera */
static void view_anchor(
	struct render_camera const *camera,
	real_point3d *anchor)
{
	*anchor = camera->position;
	if (vr_render.cinematic_view)
	{
		*anchor = vr_render.cinematic_position;
		return;
	}
	if (vr_seat_view())
	{
		struct object_marker marker;

		if (object_get_marker_by_name(vr_render.seat.unit_index, "head", &marker, 1))
			*anchor = marker.matrix.position;
		else
			unit_get_camera_position(vr_render.seat.unit_index, anchor);
		vr_seat_steady_anchor(anchor);
		vr_vehicle_adjust_anchor(anchor);
	}
}

static void eye_camera(
	int eye,
	struct render_camera *camera,
	real_rectangle2d *bounds)
{
	real aspect = (real)(camera->viewport_bounds.x1 - camera->viewport_bounds.x0) /
		(real)(camera->viewport_bounds.y1 - camera->viewport_bounds.y0);
	real_point3d position, anchor;
	real_vector3d forward, up, heading;

	view_heading(camera, &heading);
	view_anchor(camera, &anchor);
	vr_eye_view(eye, anchor.n, heading.n, aspect, position.n, forward.n, up.n, bounds->n);
	vr_vehicle_tilt_view(&anchor, &position, &forward, &up);
	camera->position = position;
	camera->forward = forward;
	camera->up = up;
	/* the bounds are tangents of a 90-degree field */
	camera->vertical_field_of_view = _pi * 0.5f;
}

/* a cutscene: each eye's view of it, then the console window (letterbox,
titles) over it, for the 3D screen */
static short cinema_windows(
	struct render_window *windows)
{
	struct render_window player = windows[0], console = windows[1];
	int eye;

	for (eye = 0; eye < 2; eye++)
	{
		struct render_camera *camera;
		real offset, convergence, aspect, tangent;
		real_vector3d right;

		windows[eye * 2] = player;
		windows[eye * 2 + 1] = console;
		camera = &windows[eye * 2].rasterizer_camera;
		vr_cinema_eye(eye, &offset, &convergence);
		cross_product3d(&camera->forward, &camera->up, &right);
		normalize3d(&right);
		camera->position.x += right.i * offset;
		camera->position.y += right.j * offset;
		camera->position.z += right.k * offset;
		windows[eye * 2].render_camera = *camera;
		/* render_camera_build_frustum: x spans its bounds times the
		viewport's aspect and the field's tangent */
		aspect = (real)(camera->viewport_bounds.x1 - camera->viewport_bounds.x0) /
			(real)(camera->viewport_bounds.y1 - camera->viewport_bounds.y0);
		tangent = (real)tan(camera->vertical_field_of_view * 0.5f);
		vr_render.cinema_shift[eye] = convergence / (aspect * tangent);
		vr_render.pass_of_window[eye * 2] = eye ? _vr_render_pass_cinema_right_eye : _vr_render_pass_cinema_left_eye;
		vr_render.eye_of_window[eye * 2 + 1] = (short)eye;
	}
	vr_render.cinema = TRUE;
	return 4;
}

/* the sight of the weapon the local player holds (vr.h's VR_SCOPE_*), by
its tag's name */
static int scope_shape(
	short local_player_index)
{
	long player_index = local_player_get_player_index(local_player_index);
	long unit_index = player_index != NONE ? player_get(player_index)->unit_index : NONE;
	long weapon_index;
	char const *name;

	if (unit_index == NONE)
		return 0;
	weapon_index = unit_inventory_get_weapon(unit_index, unit_get(unit_index)->unit.current_weapon_index);
	if (weapon_index == NONE)
		return 0;
	name = tag_get_name(weapon_get(weapon_index)->definition_index);
	return name && strstr(name, "sniper") ? VR_SCOPE_SNIPER :
		name && strstr(name, "rocket") ? VR_SCOPE_ROCKET : VR_SCOPE_ROUND;
}

/* This is reached from the render-window path every frame. Log only a gate
transition so reports distinguish missing zoom/input from scope rendering
without adding per-frame log traffic. */
static void scope_path_log(int state, int zoom_level, int shape)
{
	static int previous_state = -1;
	static int previous_shape = -1;
	static const char *const names[] =
	{
		"waiting: game zoom is off (hold the off-hand index trigger)",
		"waiting: no current weapon", "rejected: invalid screen scale",
		"rejected: VR scope view gate (see vr: scope view gate)",
		"rejected: scope viewport is too small", "render path ready"
	};
	if (state == previous_state && (state != 5 || shape == previous_shape))
		return;
	if (state == 5)
		platform_log("vr: scope render path %s (zoom level %d, shape %d)", names[state], zoom_level, shape);
	else
		platform_log("vr: scope render path %s (zoom level %d)", names[state], zoom_level);
	previous_state = state;
	previous_shape = shape;
}

/* the scope's window, while the hand aims a zoomed weapon: the player's
window seen along the gun at the game's zoomed field, in a square of the
target as large as the scope's image. Its sight shows the middle of that
field (the PC mod HaloCEVR's: a disc half the screen's height, or the sniper
rifle's wide rectangle), so the frustum is cut to it. FALSE for none. */
static boolean scope_window(
	struct render_window *window,
	struct render_window const *player)
{
	static int logged_zoom = -999, logged_shape = -1, logged_width = -1, logged_height = -1;
	static int logged_two_handed = -1, logged_weapon_hand = -1;
	static real logged_fov = -1.0f, logged_scale_x = -1.0f, logged_scale_y = -1.0f;
	struct render_camera *camera = &window->rasterizer_camera;
	real_point3d position;
	real_vector3d forward, up, vector;
	struct collision_result collision;
	float scale[2];
	real tangent, half, aspect;
	int pixels, shape, zoom_level;
	short width, height;

	zoom_level = player_control_get_zoom_level(player->local_player_index);
	if (zoom_level == NONE)
	{
		scope_path_log(0, zoom_level, 0);
		return FALSE;
	}
	shape = scope_shape(player->local_player_index);
	if (!shape)
	{
		scope_path_log(1, zoom_level, 0);
		return FALSE;
	}
	if (!vr_screen_scale(scale) || scale[0] <= 0.0f || scale[1] <= 0.0f)
	{
		scope_path_log(2, zoom_level, shape);
		return FALSE;
	}
	if (!vr_scope_view(vr_render.game_camera_position.n, position.n, forward.n, up.n, &pixels))
	{
		scope_path_log(3, zoom_level, shape);
		return FALSE;
	}
	width = (short)MIN(640.0f, (real)pixels / scale[0] + 0.5f);
	height = (short)MIN(480.0f, (real)pixels / scale[1] + 0.5f);
	if (width < 16 || height < 16)
	{
		scope_path_log(4, zoom_level, shape);
		return FALSE;
	}
	/* out of walls, as the hand's shots are */
	vector_from_points3d(&vr_render.game_camera_position, &position, &vector);
	if (collision_test_vector(FLAG(_collision_test_structure_bit), &vr_render.game_camera_position, &vector,
		NONE, &collision))
	{
		position.x = vr_render.game_camera_position.x + vector.i * collision.t * 0.9f;
		position.y = vr_render.game_camera_position.y + vector.j * collision.t * 0.9f;
		position.z = vr_render.game_camera_position.z + vector.k * collision.t * 0.9f;
	}
	*window = *player;
	tangent = (real)tan(player->render_camera.vertical_field_of_view * 0.5f);
	half = tangent * (shape == VR_SCOPE_SNIPER ? 0.4033f : 0.5f);
	camera->position = position;
	camera->forward = forward;
	camera->up = up;
	camera->viewport_bounds.x0 = camera->viewport_bounds.y0 = 0;
	camera->viewport_bounds.x1 = width;
	camera->viewport_bounds.y1 = height;
	camera->window_bounds = camera->viewport_bounds;
	/* the bounds are tangents of a 90-degree field; x spans them times the
	viewport's aspect (render_camera_build_frustum) */
	camera->vertical_field_of_view = _pi * 0.5f;
	aspect = (real)width / (real)height;
	vr_render.scope_bounds.x0 = -half / aspect;
	vr_render.scope_bounds.x1 = half / aspect;
	vr_render.scope_bounds.y0 = -half;
	vr_render.scope_bounds.y1 = half;
	vr_render.scope_viewport = camera->viewport_bounds;
	vr_render.scope_shape = shape;
	window->render_camera = *camera;
	scope_path_log(5, zoom_level, shape);
	if (logged_zoom != zoom_level || logged_shape != shape || logged_width != width || logged_height != height ||
		logged_two_handed != vr_two_handed() || logged_weapon_hand != vr_weapon_hand() ||
		fabsf((float)(logged_fov - player->render_camera.vertical_field_of_view)) > 0.001f ||
		fabsf((float)(logged_scale_x - scale[0])) > 0.001f || fabsf((float)(logged_scale_y - scale[1])) > 0.001f)
	{
		platform_log("vr: scope camera: zoom %d, shape %d, weapon hand %s, aim %s, two-hand %s; viewport %dx%d, eye pixels %d, screen scale %.3f/%.3f, game FOV %.1f deg, scope vertical span %.1f deg",
			zoom_level, shape, vr_weapon_hand() ? "right" : "left", vr_hand_aiming() ? "hand" : "head",
			vr_two_handed() ? "locked" : "one hand", width, height, pixels, scale[0], scale[1],
			player->render_camera.vertical_field_of_view * 57.29578f, atan((double)half) * 2.0 * 57.29578);
		logged_zoom = zoom_level;
		logged_shape = shape;
		logged_width = width;
		logged_height = height;
		logged_two_handed = vr_two_handed();
		logged_weapon_hand = vr_weapon_hand();
		logged_fov = player->render_camera.vertical_field_of_view;
		logged_scale_x = scale[0];
		logged_scale_y = scale[1];
	}
	return TRUE;
}

/* test21: the engine's shift of a shot off its aim, so the reticle marks
where the shot flies: the primary trigger's first_person_weapon_offset
along the aim, Halo's left and up (trigger_create_projectiles in
weapons.c). FALSE when the shot is not aimed, or leaves from the gun's
model (uses weapon origin) rather than from the hand */
static boolean vr_shot_offset(
	long unit_index,
	boolean from_hand,
	real_vector3d const *forward,
	real_vector3d *offset)
{
	long weapon_index = unit_inventory_get_weapon(unit_index, unit_get(unit_index)->unit.current_weapon_index);
	struct weapon_definition *definition;
	struct weapon_trigger_definition *trigger;
	real_vector3d left, up;

	if (weapon_index == NONE)
		return FALSE;
	definition = weapon_definition_get(weapon_get(weapon_index)->definition_index);
	if (definition->weapon.triggers.count < 1)
		return FALSE;
	trigger = TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, 0, struct weapon_trigger_definition);
	if (TEST_FLAG(trigger->flags, _weapon_trigger_projectiles_cannot_be_aimed_bit) ||
		(TEST_FLAG(trigger->flags, _weapon_trigger_uses_weapon_origin_bit) && !from_hand))
		return FALSE;
	if (trigger->first_person_weapon_offset.x == 0.0f && trigger->first_person_weapon_offset.y == 0.0f &&
		trigger->first_person_weapon_offset.z == 0.0f)
		return FALSE;
	cross_product3d(global_up3d, forward, &left);
	if (normalize3d(&left) == 0.0f)
		left = *global_left3d;
	cross_product3d(forward, &left, &up);
	normalize3d(&up);
	for (int axis = 0; axis < 3; axis++)
		offset->n[axis] = forward->n[axis] * trigger->first_person_weapon_offset.x +
			left.n[axis] * trigger->first_person_weapon_offset.y + up.n[axis] * trigger->first_person_weapon_offset.z;
	return isfinite(offset->i) && isfinite(offset->j) && isfinite(offset->k);
}

short vr_render_windows(
	struct render_window *windows,
	short window_count)
{
	struct render_window player, console;
	int eye;

	vr_graphics_apply();
	vr_render.stereo = FALSE;
	vr_render.cinema = FALSE;
	vr_render.cinematic_view = window_count == 2 && cinematic_in_progress() && vr_cinema_immersive();
	vr_render.scope_shape = 0;
	for (eye = 0; eye < 5; eye++)
	{
		vr_render.pass_of_window[eye] = _vr_render_pass_none;
		vr_render.eye_of_window[eye] = NONE;
	}
	if (window_count == 2 &&
		windows[0].local_player_index != NONE &&
		!windows[0].console_window &&
		cinematic_in_progress() &&
		!vr_render.cinematic_view &&
		vr_cinema_begin())
	{
		vr_render.cinematic_previous = FALSE;
		return cinema_windows(windows);
	}
	if (window_count != 2 ||
		windows[0].local_player_index == NONE ||
		windows[0].console_window ||
		(cinematic_in_progress() && !vr_render.cinematic_view) ||
		/* the main menu's scene is around the player too, its menus on the
		HUD's panel ahead (vr.menu_3d), or behind them on the flat screen */
		(global_scenario_get()->type == _scenario_type_main_menu && !config_boolean("vr.menu_3d")) ||
		!vr_stereo_begin())
	{
		vr_render.cinematic_previous = FALSE;
		vr_render.cinematic_view = FALSE;
		return window_count;
	}
	if (vr_render.cinematic_view)
	{
		struct render_camera const *camera = &windows[0].render_camera;
		real yaw = vr_yaw(&camera->forward);
		real delta = (real)atan2(sin(yaw - vr_render.cinematic_last_yaw), cos(yaw - vr_render.cinematic_last_yaw));
		if (!vr_render.cinematic_previous ||
			distance_squared3d(&camera->position, &vr_render.cinematic_last_position) > 1.5f * 1.5f ||
			fabs(delta) > _pi / 6.0f)
		{
			vr_render.cinematic_head_yaw = vr_head_local_yaw();
			vr_view_blink(0.85f);
		}
		vr_render.cinematic_heading = yaw - vr_render.cinematic_head_yaw;
		vr_render.cinematic_position = vr_render.cinematic_last_position = camera->position;
		vr_render.cinematic_last_yaw = yaw;
	}
	vr_render.cinematic_previous = vr_render.cinematic_view;
	/* Facing input can be inhibited for a passenger or scripted/co-op seat.
	The camera still needs the actual current seat and positional tracking. */
	if (!vr_render.cinematic_view)
	{
		long player_index = local_player_get_player_index(windows[0].local_player_index);
		vr_update_seat(player_index != NONE ? player_get(player_index)->unit_index : NONE);
	}
	player = windows[0];
	console = windows[1];
	for (eye = 0; eye < 2; eye++)
	{
		windows[eye] = player;
		eye_camera(eye, &windows[eye].rasterizer_camera, &vr_render.eye_bounds[eye]);
		windows[eye].render_camera = windows[eye].rasterizer_camera;
	}
	view_anchor(&player.render_camera, &vr_render.game_camera_position);
	vr_render.head_camera = player.render_camera;
	vr_render.hud_from_head = FALSE;
	{
		real_point3d position;
		real_vector3d forward, up;

		real_vector3d heading;

		view_heading(&player.render_camera, &heading);
		if (vr_head_view(vr_render.game_camera_position.n, heading.n, position.n, forward.n, up.n))
		{
			real horizontal = (real)sqrt(forward.i * forward.i + forward.j * forward.j);

			vr_render.head_camera.position = position;
			vr_render.head_camera.forward = forward;
			vr_render.head_camera.up = up;
			vr_render.hud_from_head = TRUE;
			/* (looking straight up or down, the last yaw holds) */
			if (horizontal > 0.2f)
			{
				vr_render.head_yaw = vr_yaw(&forward);
				vr_render.head_yaw_valid = TRUE;
			}
		}
	}
	/* the HUD, drawn from the head over its head-locked panel's field: its
	markers (nav points, friends' names) land on what they mark, and with
	the head aiming its crosshair is where the head looks */
	windows[2] = player;
	windows[3] = console;
	if (vr_render.hud_from_head)
	{
		struct render_camera *camera = &windows[2].rasterizer_camera;

		camera->position = vr_render.head_camera.position;
		camera->forward = vr_render.head_camera.forward;
		camera->up = vr_render.head_camera.up;
		camera->vertical_field_of_view = _pi * 0.5f;
		windows[2].render_camera = *camera;
		vr_hud_bounds((real)(camera->viewport_bounds.x1 - camera->viewport_bounds.x0) /
			(real)(camera->viewport_bounds.y1 - camera->viewport_bounds.y0), vr_render.hud_bounds.n);
	}
	/* Reticle on the engine's pre-spread firing ray, as it is this frame.
	 * Network play keeps the camera origin; offline the shot starts at the
	 * hand unless a wall is between the camera and the hand (the rule of
	 * vr_render_hand_origin and unit_adjust_projectile_ray). Test20e: the
	 * unit's aiming vector, camera position and hand origin advance only on
	 * the game's 30 Hz ticks, so a reticle built from them stepped at 30 fps
	 * while the eyes and the gun moved at the headset's rate. The game takes
	 * its aim from this same hand ray every tick (vr_aim), so this frame's
	 * ray is the next shot's; where the two part by more than a tick's turn
	 * could explain (the game clamping its aim), the game's direction wins. */
	if (vr_hand_aiming() && !vr_render.cinematic_view)
	{
		real_point3d origin;
		real_vector3d direction, vector;
		struct collision_result collision;
		real distance = 128.0f;

		if (vr_hand_ray(vr_render.game_camera_position.n, origin.n, direction.n))
		{
			long player_index = local_player_get_player_index(player.local_player_index);
			long unit_index = player_index != NONE ? player_get(player_index)->unit_index : NONE;
			real_point3d hit;

			if (unit_index != NONE && object_get(unit_index)->object.parent_object_index == NONE)
			{
				real_point3d camera = vr_render.game_camera_position;
				real_vector3d aiming, to_hand, shift;
				boolean from_hand = TRUE;

				unit_get_aiming_vector(unit_index, &aiming);
				if (dot_product3d(&aiming, &direction) < 0.866f)
					direction = aiming;
				vector_from_points3d(&camera, &origin, &to_hand);
				if (game_connection() != _game_connection_local ||
					collision_test_vector(FLAG(_collision_test_structure_bit), &camera, &to_hand, unit_index, &collision))
				{
					origin = camera;
					from_hand = FALSE;
				}
				/* test21: the shot's own offset from its aim, then the
				engine's turn of the shot toward where the camera's line hits,
				within the weapon's cone (player_aim_projectile, sharing its
				code): the shot converges there, not on the hand's line (the
				impacts landed above and beside the reticle) */
				if (vr_shot_offset(unit_index, from_hand, &direction, &shift))
				{
					origin.x += shift.i;
					origin.y += shift.j;
					origin.z += shift.k;
				}
				aiming = direction;
				vr_aim_assist_converge(player_index, &camera, &aiming, &origin, &direction);
			}

			scale_vector3d(&direction, distance, &vector);
			if (collision_test_vector(_collision_test_for_projectiles_flags, &origin, &vector, unit_index,
				&collision))
			{
				distance *= collision.t;
			}
			point_from_line3d(&origin, &direction, distance, &hit);
			vr_set_reticle_world(vr_render.game_camera_position.n, hit.n);
			vr_render.reticle_origin = origin;
			vr_render.reticle_direction = direction;
			vr_render.reticle_time = vr_pose_time();
		}
	}
	/* Test31: mounted weapons fire along their actual muzzle/aimed ray,
	not the headset-centred HUD plane. This is independent of input layout. */
	if (!vr_render.cinematic_view && vr_render.seat.seated &&
		(vr_render.seat.gunner || vr_render.seat.driver))
	{
		long player_index = local_player_get_player_index(player.local_player_index);
		long unit_index = player_index != NONE ? player_get(player_index)->unit_index : NONE;
		long aiming = unit_index != NONE ? unit_get_aiming_unit_index(unit_index) : NONE;
		struct unit_datum *unit = aiming != NONE ? unit_try_and_get(aiming) : NULL;
		long weapon = unit ? unit_inventory_get_weapon(aiming, unit->unit.current_weapon_index) : NONE;
		real_point3d origin, hit;
		real_vector3d direction, vector;
		struct collision_result collision;
		real distance = 128.0f;
		if (unit && aiming != unit_index && weapon != NONE &&
			weapon_vr_preview_primary_ray(weapon, player_index, &origin, &direction))
		{
			scale_vector3d(&direction, distance, &vector);
			if (collision_test_vector(_collision_test_for_projectiles_flags, &origin, &vector, aiming, &collision))
				distance *= collision.t;
			point_from_line3d(&origin, &direction, distance, &hit);
			vr_set_reticle_world(vr_render.game_camera_position.n, hit.n);
			vr_render.reticle_origin = origin;
			vr_render.reticle_direction = direction;
			vr_render.reticle_time = vr_pose_time();
		}
	}
	vr_render.stereo = TRUE;
    if (!vr_render.cinematic_view && !vr_body_setting() && network_vr_pose_wanted()) {
        long p = local_player_get_player_index(player.local_player_index);
        long u = p != NONE ? player_get(p)->unit_index : NONE;
        if (u != NONE && object_get(u)->object.type == _object_type_biped &&
            object_get(u)->object.parent_object_index == NONE &&
            !TEST_FLAG(object_get(u)->object.damage_flags, _object_dead_bit))
            vr_render_body_matrices(u);
    }
	vr_render.pass_of_window[0] = _vr_render_pass_left_eye;
	vr_render.pass_of_window[1] = _vr_render_pass_right_eye;
	vr_render.pass_of_window[2] = _vr_render_pass_hud;
	vr_render.eye_of_window[0] = 0;
	vr_render.eye_of_window[1] = 1;
	/* zoomed: the scope's view before the HUD (window 2, within
	MAXIMUM_WINDOWS for the rasterizer's per-window state) */
	{
		struct render_window scope;

		if (!vr_render.cinematic_view && scope_window(&scope, &player))
		{
			windows[3] = windows[2];
			windows[4] = console;
			windows[2] = scope;
			vr_render.pass_of_window[2] = _vr_render_pass_scope;
			vr_render.pass_of_window[3] = _vr_render_pass_hud;
			return 5;
		}
	}
	return 4;
}

void vr_render_window_begin(
	short window_index)
{
	vr_render_pass = (vr_render.stereo || vr_render.cinema) && window_index >= 0 && window_index < 5 ?
		vr_render.pass_of_window[window_index] : _vr_render_pass_none;
	vr_pass_mark(vr_render_pass, 0);
}

void vr_render_window_end(
	short window_index)
{
	vr_pass_mark(vr_render_pass, 1);
	if ((vr_render.stereo || vr_render.cinema) && window_index >= 0 && window_index < 5 &&
		vr_render.eye_of_window[window_index] != NONE)
	{
		halo_vr_resolve_eye(vr_render.eye_of_window[window_index]);
	}
	if (VR_RENDER_SCOPE())
	{
		halo_vr_resolve_scope(vr_render.scope_viewport.x0, vr_render.scope_viewport.y0,
			vr_render.scope_viewport.x1, vr_render.scope_viewport.y1, vr_render.scope_shape);
	}
	vr_render_pass = _vr_render_pass_none;
}

void vr_render_frustum_bounds(
	real_rectangle2d *bounds)
{
	if (VR_RENDER_EYE())
	{
		*bounds = vr_render.eye_bounds[vr_render_pass];
	}
	else if (VR_RENDER_SCOPE())
	{
		*bounds = vr_render.scope_bounds;
	}
	else if (VR_RENDER_HUD() && vr_render.hud_from_head)
	{
		*bounds = vr_render.hud_bounds;
	}
	else if (vr_render_pass == _vr_render_pass_cinema_left_eye || vr_render_pass == _vr_render_pass_cinema_right_eye)
	{
		real shift = vr_render.cinema_shift[vr_render_pass == _vr_render_pass_cinema_right_eye];

		bounds->x0 += shift;
		bounds->x1 += shift;
	}
}

void vr_render_room_scale(
	long biped_index,
	real_point3d *position,
	real height,
	real width)
{
	static real diag_walk = -1.0f, asked = 0.0f, walked = 0.0f;
	static long diag_tick = NONE;
	short local_player_index = local_player_get_next(NONE);
	long player_index = local_player_index != NONE ? local_player_get_player_index(local_player_index) : NONE;
	struct collision_plane collisions[16];
	real_point3d clipped;
	real_vector3d step, clipped_velocity;
	float room_step[2];

	if (player_index == NONE || player_get(player_index)->unit_index != biped_index || !vr_active())
		return;
	/* only where the player could walk by the stick (a local game; no
	cutscene, script or death holding them) */
	if (!vr_aiming() || cinematic_in_progress() || game_connection() != _game_connection_local ||
		director_inhibited_input(local_player_index) ||
		object_get(biped_index)->object.parent_object_index != NONE ||
		TEST_FLAG(object_get(biped_index)->object.damage_flags, _object_dead_bit))
	{
		vr_room_hold();
		return;
	}
	if (!vr_room_step(room_step))
		return;
	step.i = room_step[0];
	step.j = room_step[1];
	step.k = 0.0f;
	/* (a second tick in a frame has the head where the first left it) */
	if (magnitude_squared3d(&step) < 1e-10f)
	{
		vr_room_moved();
		return;
	}
	/* the pill moved as walking moves it, sliding along what it meets */
	collision_move_pill(_collision_test_for_bipeds_living_flags, position, &step, height, width, biped_index,
		&clipped, &clipped_velocity, 16, collisions);
	vr_room_moved();
	/* vr.diag_walk_speed: how far the steps took the player each second */
	if (diag_walk < 0.0f)
		diag_walk = (real)config_real("vr.diag_walk_speed");
	if (diag_walk != 0.0f)
	{
		asked += magnitude3d(&step);
		walked += (real)sqrt((clipped.x - position->x) * (clipped.x - position->x) +
			(clipped.y - position->y) * (clipped.y - position->y));
		if (diag_tick == NONE || game_time_get() - diag_tick >= TICKS_PER_SECOND)
		{
			platform_log("vr: room-scale walked %.3f of %.3f units this second", walked, asked);
			diag_tick = game_time_get();
			asked = walked = 0.0f;
		}
	}
	*position = clipped;
}

int vr_render_motion_sensor_yaw(
	short local_player_index,
	real *yaw)
{
	if (local_player_index != local_player_get_next(NONE) || !vr_aiming() || !vr_render.head_yaw_valid)
		return FALSE;
	*yaw = vr_render.head_yaw;
	return TRUE;
}

int vr_render_unzoomed_view(
	void)
{
	return vr_render.stereo;
}

/* the gun kept out of the walls: its length (45 cm ahead of where it is
posed from) may not reach past a wall between the head and its muzzle; it
is drawn pulled back along itself, the wall's side of it at the wall (the
arms reaching for its grip follow) */
static void vr_pressure_buzz(int hand, real depth_metres);

static void weapon_out_of_walls(
	real_point3d *position,
	real_vector3d const *forward,
	real_vector3d *pullback)
{
	real units = vr_units_per_metre();
	real_point3d head = vr_render.head_camera.position, muzzle;
	real_vector3d vector;
	struct collision_result collision;

	muzzle.x = position->x + forward->i * 0.45f * units;
	muzzle.y = position->y + forward->j * 0.45f * units;
	muzzle.z = position->z + forward->k * 0.45f * units;
	vector_from_points3d(&head, &muzzle, &vector);
	if (collision_test_vector(FLAG(_collision_test_structure_bit), &head, &vector, NONE, &collision))
	{
		real length = sqrtf(vector.i * vector.i + vector.j * vector.j + vector.k * vector.k);
		real back = (1.0f - collision.t) * length + 0.03f * units;

		position->x -= forward->i * back;
		position->y -= forward->j * back;
		position->z -= forward->k * back;
		pullback->i = -forward->i * back;
		pullback->j = -forward->j * back;
		pullback->k = -forward->k * back;
		/* the gun pushed into the wall, felt in the hands holding it */
		if (units > 0.0f)
		{
			real depth = (1.0f - collision.t) * length / units;

			vr_pressure_buzz(vr_weapon_hand(), depth);
			if (vr_two_handed())
				vr_pressure_buzz(1 - vr_weapon_hand(), depth);
		}
	}
}

void vr_render_weapon_camera(
	struct render_camera *camera)
{
	real_point3d position;
	real_vector3d forward, up;

	/* in the hand when it aims, else with the head */
	vr_render.weapon_camera_valid = FALSE;
	vr_render.weapon_pullback = (real_vector3d){ 0.0f, 0.0f, 0.0f };
	if (vr_render.stereo && !vr_render.cinematic_view &&
		vr_weapon_view(vr_render.game_camera_position.n, position.n, forward.n, up.n))
	{
		weapon_out_of_walls(&position, &forward, &vr_render.weapon_pullback);
		camera->position = position;
		camera->forward = forward;
		camera->up = up;
		vr_render.weapon_camera = *camera;
		vr_render.weapon_camera_valid = TRUE;
	}
	else if (vr_render.stereo)
	{
		camera->position = vr_render.head_camera.position;
		camera->forward = vr_render.head_camera.forward;
		camera->up = vr_render.head_camera.up;
	}
}

boolean vr_script_can_see_point(long unit_index, const real_point3d *point, real field_of_view, boolean *result)
{
	short local = local_player_get_next(NONE);
	long player_index;
	real_point3d camera, eye;
	real_vector3d forward, direction;
	if (!vr_active() || cinematic_in_progress() || local == NONE || unit_index == NONE) return FALSE;
	player_index = local_player_get_player_index(local);
	if (player_index == NONE || player_get(player_index)->unit_index != unit_index || !unit_try_and_get(unit_index)) return FALSE;
	unit_get_camera_position(unit_index, &camera);
	if (!vr_script_head_view(camera.n, eye.n, forward.n)) return FALSE;
	vector_from_points3d(&eye, point, &direction);
	normalize3d(&direction);
	*result = dot_product3d(&direction, &forward) > cosine(field_of_view);
	return TRUE;
}

void vr_player_control_facing(
	short local_player_index)
{
	real_euler_angles2d const *angles;
	real_vector3d forward;
	long player_index, unit_index;
	boolean seated;

	if (local_player_index != local_player_get_next(NONE))
		return;
	if (cinematic_in_progress())
	{
		vr_room_hold();
		return;
	}
	player_index = local_player_get_player_index(local_player_index);
	unit_index = player_index != NONE ? player_get(player_index)->unit_index : NONE;
	vr_diag_drive(unit_index);
	vr_update_seat(unit_index);
	/* seated, the head leans from where the player stood (vr.roomscale) */
	if (vr_render.seat.seated)
		vr_room_hold();
	vr_set_zoom_level(player_control_get_zoom_level(local_player_index));
	seated = vr_render.seat.seated;
	angles = player_control_get_facing_angles(local_player_index);
	/* a driver steered by the stick (vr.vehicle_steering "stick"): the game
	takes the right stick as it would, and the head only looks */
	{
		/* Drivers and mounted gunners select independently; passengers keep
		their existing head-aim behavior. */
		int hand_may_aim = vr_vehicle_aim_source();
		real heading = vr_render.seat.heading;

		if (!vr_aim(angles->yaw, seated, hand_may_aim, vr_seat_view() ? &heading : NULL, forward.n))
		{
			vr_render.hand_origin_valid = FALSE;
			return;
		}
	}
	player_control_set_facing(local_player_index, &forward);

	/* where the hand's shots start: the hand, seen from the unit's eye,
	unless a wall is in between */
	vr_render.hand_origin_valid = FALSE;
	if (unit_index != NONE && vr_hand_aiming() && game_connection() == _game_connection_local)
	{
		real_point3d camera, origin;
		real_vector3d direction, vector;
		struct collision_result collision;

		unit_get_camera_position(unit_index, &camera);
		if (vr_hand_ray(camera.n, origin.n, direction.n))
		{
			vector_from_points3d(&camera, &origin, &vector);
			if (!collision_test_vector(FLAG(_collision_test_structure_bit), &camera, &vector, unit_index, &collision))
			{
				vr_render.hand_origin = origin;
				vr_render.hand_origin_unit = unit_index;
				vr_render.hand_origin_valid = TRUE;
			}
		}
	}
}

/* the buzz of a shot, by weapon (its tag's name): strength and seconds */
static void vr_weapon_buzz(
	long weapon_index,
	real *amplitude,
	real *seconds)
{
	static struct { char const *name; real amplitude, seconds; } const buzzes[] =
	{
		{ "pistol", 0.6f, 0.06f },        /* the plasma pistol is matched first */
		{ "shotgun", 1.0f, 0.12f },
		{ "sniper", 1.0f, 0.12f },
		{ "rocket", 1.0f, 0.20f },
		{ "fuel rod", 0.9f, 0.15f },
		{ "flamethrower", 0.3f, 0.05f },
		{ "needler", 0.3f, 0.04f },
		{ "plasma rifle", 0.3f, 0.04f },
		{ "assault rifle", 0.35f, 0.04f },
	};
	char const *name = tag_get_name(weapon_get(weapon_index)->definition_index);
	int index;

	*amplitude = 0.5f;
	*seconds = 0.05f;
	if (name && strstr(name, "plasma pistol"))
	{
		*amplitude = 0.4f;
		return;
	}
	for (index = 0; name && index < (int)(sizeof(buzzes) / sizeof(buzzes[0])); index++)
	{
		if (strstr(name, buzzes[index].name))
		{
			*amplitude = buzzes[index].amplitude;
			*seconds = buzzes[index].seconds;
			return;
		}
	}
}

void vr_render_shot_diagnostic(
	long weapon_index,
	long player_index,
	real_point3d const *origin,
	real_vector3d const *aimed,
	real_vector3d const *shot,
	int from_hand)
{
	static long logged_weapon = NONE;
	static double logged_time = -1000.0;
	double now = vr_pose_time();
	long unit_index;
	real turned, from_reticle = -1.0f, apart = -1.0f;
	int kind = vr_gun_class();

	if (!vr_active() || player_index == NONE || !aimed || !shot)
		return;
	unit_index = player_get(player_index)->unit_index;
	if (unit_index == NONE || player_get(player_index)->local_player_index == NONE ||
		(weapon_index == logged_weapon && now - logged_time < 10.0 && now >= logged_time))
		return;
	logged_weapon = weapon_index;
	logged_time = now;
	turned = (real)acos(PIN(dot_product3d(aimed, shot), -1.0f, 1.0f)) * 57.2957795f;
	if (now - vr_render.reticle_time >= 0.0 && now - vr_render.reticle_time < 0.1)
	{
		real dx = origin->x - vr_render.reticle_origin.x, dy = origin->y - vr_render.reticle_origin.y,
			dz = origin->z - vr_render.reticle_origin.z;

		from_reticle = (real)acos(PIN(dot_product3d(&vr_render.reticle_direction, shot), -1.0f, 1.0f)) * 57.2957795f;
		apart = (real)sqrt(dx * dx + dy * dy + dz * dz) / vr_units_per_metre();
	}
	platform_log("vr: shot (%s): from the %s; the engine turned it %.2f deg off the aim (its cone);"
		" reticle %.2f deg and %.2f m from it (-1: no reticle this frame); aim adjustment %s",
		vr_gun_class_label(kind), from_hand ? "hand" : "game's camera or gun", turned, from_reticle, apart,
		kind >= 0 ? "per gun (see the holding line)" : "none");
}

void vr_render_weapon_fired(
	long weapon_index,
	long player_index)
{
	static long last_tick = NONE, last_weapon = NONE;
	real amplitude, seconds;
	int hand;

	if (!vr_active() || player_index == NONE ||
		player_index != local_player_get_player_index(local_player_get_next(NONE)))
	{
		return;
	}
	/* a shotgun's pellets are one shot */
	if (last_tick == game_time_get() && last_weapon == weapon_index)
		return;
	last_tick = game_time_get();
	last_weapon = weapon_index;
	vr_weapon_buzz(weapon_index, &amplitude, &seconds);
	hand = vr_weapon_hand();
	vr_haptic(hand, amplitude, seconds);
	/* held in both hands, the kick is felt in both */
	if (vr_two_handed())
		vr_haptic(1 - hand, amplitude, seconds);
}

/* vr.diag_zoom_seconds: this long into play the local player zooms in,
switching first to a weapon that zooms (for checking the scope
unattended) */
static unsigned long vr_diag_zoom(
	short local_player_index)
{
	static real seconds = -1.0f;
	static long next_tick = NONE;
	static int switches = 0;
	long player_index = local_player_get_player_index(local_player_index);
	long unit_index = player_index != NONE ? player_get(player_index)->unit_index : NONE;
	long weapon_index;

	if (seconds < 0.0f)
		seconds = (real)config_real("vr.diag_zoom_seconds");
	if (seconds <= 0.0f || unit_index == NONE || cinematic_in_progress())
		return 0;
	if (next_tick == NONE)
		next_tick = game_time_get() + (long)(seconds * TICKS_PER_SECOND);
	if (game_time_get() < next_tick)
		return 0;
	weapon_index = unit_inventory_get_weapon(unit_index, unit_get(unit_index)->unit.current_weapon_index);
	if (weapon_index != NONE && weapon_definition_get(weapon_get(weapon_index)->definition_index)->weapon.zoom_level_count > 0)
	{
		seconds = 0.0f;
		platform_log("vr: diag zoom: zooming %s", tag_get_name(weapon_get(weapon_index)->definition_index));
		return VR_RENDER_ACTION_ZOOM;
	}
	if (++switches > 3)
	{
		seconds = 0.0f;
		platform_log("vr: diag zoom: no weapon that zooms");
		return 0;
	}
	/* a weapon switch takes its animation's time */
	next_tick = game_time_get() + 2 * TICKS_PER_SECOND;
	return VR_RENDER_ACTION_SWITCH_WEAPON;
}

unsigned long vr_render_actions(
	short local_player_index)
{
	long player_index, unit_index, weapon_index = NONE;

	if (local_player_index != local_player_get_next(NONE) || !vr_active())
		return 0;
	player_index = local_player_get_player_index(local_player_index);
	unit_index = player_index != NONE ? player_get(player_index)->unit_index : NONE;
	if (unit_index != NONE)
		weapon_index = unit_inventory_get_weapon(unit_index, unit_get(unit_index)->unit.current_weapon_index);
	vr_note_weapon(weapon_index);
	/* test21: the held gun's kind, for its aim adjustment (by its tag's
	name, looked up when the weapon changes) */
	{
		static long named_weapon = NONE;
		static int kind = -1;

		if (weapon_index != named_weapon)
		{
			named_weapon = weapon_index;
			kind = weapon_index != NONE ?
				vr_gun_class_of_name(tag_get_name(weapon_get(weapon_index)->definition_index)) : -1;
		}
		vr_set_gun_class(kind);
	}
	/* physical weapons (dropping and grabbing guns) change the inventory
	here, which a network game's other machines would not see: in a local
	game only, unless vr.physical_multiplayer */
	vr_set_physical_allowed(game_connection() == _game_connection_local ||
		config_boolean("vr.physical_multiplayer"));
	/* a hand's blows are damage this machine causes: not a network game's
	client (whose host decides damage), which keeps the swing gesture */
	vr_set_impact_melee_allowed(!network_game_distributed_client());
	/* test21: physical melee is off in any network game unless the player
	turns it on (vr.melee_multiplayer); the melee button always works */
	vr_set_network_game(game_connection() != _game_connection_local);
	{
		unsigned long actions = vr_take_actions() | vr_diag_zoom(local_player_index);
		/* test21: the horn's chain while driving, once a press: the stick
		click sends the crouch control; three ticks on, whether the vehicle
		has it from its driver (vehicles.c sounds the horn from it) */
		static boolean horn_down;
		static int horn_check;
		boolean horn = vr_render.seat.seated && vr_render.seat.driver && (actions & VR_RENDER_ACTION_CROUCH);

		if (horn && !horn_down)
		{
			platform_log("vr: horn: stick clicked while driving; the crouch control goes to the game");
			horn_check = 3;
		}
		else if (horn && horn_check > 0 && --horn_check == 0)
		{
			struct unit_datum *vehicle = vr_render.seat.vehicle_index != NONE ?
				unit_try_and_get(vr_render.seat.vehicle_index) : NULL;

			platform_log("vr: horn: the vehicle %s the crouch control from its driver",
				!vehicle ? "is gone; no check of" :
				TEST_FLAG(vehicle->unit.control_flags, _unit_control_crouch_modifier_bit) ? "has" : "does not have");
		}
		horn_down = horn;
		return actions;
	}
}

/* The visible gun and pickup reach follow the physical hand, independently
of the game's shot origin (which is pulled back out of walls). */
int vr_render_first_person_gun_hidden(void)
{
	return vr_render.stereo && vr_hand_aiming() && vr_hand_empty();
}

int vr_render_immersive_cutscene(void)
{
	return vr_render.stereo && vr_render.cinematic_view;
}

int vr_render_grab_point(long unit_index, real_point3d *point)
{
	real_point3d camera;
	real_vector3d forward, up;

	if (unit_index == NONE || !vr_active() || !vr_hand_empty())
		return FALSE;
	unit_get_camera_position(unit_index, &camera);
	return vr_hand_world(vr_weapon_hand(), camera.n, point->n, forward.n, up.n);
}

/* ---------- impact melee (vr.melee "impact")

Each tick each hand's path since the last tick is swept through the world
(the weapon hand's from points along the gun: the grip, its middle and, a
long gun, its far end; the other hand's from the fist); moving at
vr.melee_speed or more about the room, what it meets takes a blow
(unit_vr_impact_melee: the weapon's melee damage, every object, vehicles
shoved and broken, glass), harder the faster the swing. A hand that struck
rests 0.35 s, as a swing's one blow.

test26: a swing reaches on past where the hand was at the tick, as it
would carry on (5 hundredths of a second at its speed, at most 15 cm): the
bodies' collision holds the player off a character, so a natural swing at
one stopped short of it. And a gun strikes with its length, not its middle
alone. */
#define VR_MELEE_POINTS 3

void vr_render_impact_melee(
	short local_player_index)
{
	static long last_unit = NONE;
	static boolean last_valid[2][VR_MELEE_POINTS];
	static real_point3d last[2][VR_MELEE_POINTS];
	static real rest[2];
	real units = vr_units_per_metre();
	long player_index, unit_index;
	real_point3d camera;
	int hand;

	if (local_player_index != local_player_get_next(NONE) || !vr_active())
		return;
	player_index = local_player_get_player_index(local_player_index);
	unit_index = player_index != NONE ? player_get(player_index)->unit_index : NONE;
	if (unit_index == NONE || unit_index != last_unit || !vr_impact_melee() || !vr_aiming() ||
		cinematic_in_progress() || director_inhibited_input(local_player_index) ||
		object_get(unit_index)->object.parent_object_index != NONE ||
		TEST_FLAG(object_get(unit_index)->object.damage_flags, _object_dead_bit))
	{
		memset(last_valid, 0, sizeof(last_valid));
		rest[0] = rest[1] = 0.0f;
		last_unit = unit_index;
		return;
	}
	unit_get_camera_position(unit_index, &camera);
	for (hand = 0; hand < 2; hand++)
	{
		real_point3d grip;
		real_vector3d forward, up;
		real speed, along[VR_MELEE_POINTS] = { 0.0f, 0.0f, 0.0f };
		int point_count = 1, index;
		boolean armed = hand == vr_weapon_hand() && !vr_hand_empty() &&
			unit_get(unit_index)->unit.current_weapon_index != NONE;

		if (rest[hand] > 0.0f)
			rest[hand] -= 1.0f / 30.0f;
		if (!vr_hand_world(hand, camera.n, grip.n, forward.n, up.n))
		{
			for (index = 0; index < VR_MELEE_POINTS; index++)
				last_valid[hand][index] = FALSE;
			continue;
		}
		/* the gun's points along its length from the grip (metres): a
		pistol's short, a long gun's to its far end */
		if (armed)
		{
			int kind = vr_gun_class();
			boolean short_gun = kind == VR_GUN_PISTOL || kind == VR_GUN_PLASMA_PISTOL || kind == VR_GUN_NEEDLER;

			along[0] = 0.0f;
			along[1] = short_gun ? 0.12f : 0.2f;
			along[2] = short_gun ? 0.0f : 0.4f;
			point_count = short_gun ? 2 : 3;
		}
		speed = vr_hand_speed(hand);
		for (index = 0; index < point_count; index++)
		{
			real_point3d point = grip;
			real_vector3d sweep;
			real distance_squared;

			point.x += forward.i * along[index] * units;
			point.y += forward.j * along[index] * units;
			point.z += forward.k * along[index] * units;
			if (last_valid[hand][index] && rest[hand] <= 0.0f && speed >= vr_melee_speed())
			{
				vector_from_points3d(&last[hand][index], &point, &sweep);
				distance_squared = sweep.i * sweep.i + sweep.j * sweep.j + sweep.k * sweep.k;
				/* Tracking recovery/teleports are not a punch across the level. */
				if (isfinite(distance_squared) && distance_squared > 1e-8f && distance_squared < units * units)
				{
					real scale = speed / (2.0f * vr_melee_speed()) + 0.25f;
					real length = (real)sqrt(distance_squared);
					/* (the swing carried on: test26) */
					real reach = speed * 0.05f;
					real extend;

					reach = (reach > 0.15f ? 0.15f : reach) * units;
					extend = (length + reach) / length;
					sweep.i *= extend;
					sweep.j *= extend;
					sweep.k *= extend;
					scale = scale < 0.5f ? 0.5f : scale > 1.5f ? 1.5f : scale;
					if (unit_vr_impact_melee(unit_index, &last[hand][index], &sweep, scale, armed,
						(armed && index ? 0.05f : 0.035f) * units))
					{
						rest[hand] = 0.35f;
						vr_haptic(hand, 1.0f, 0.08f);
						platform_log("vr: %s hand struck at %.1f m/s (blow %.2f, %s)", hand ? "right" : "left", speed, scale,
							!armed ? "fist" : index == 0 ? "the gun's grip" : index == 1 ? "the gun's middle" : "the gun's end");
					}
				}
			}
			last[hand][index] = point;
			last_valid[hand][index] = TRUE;
		}
		for (index = point_count; index < VR_MELEE_POINTS; index++)
			last_valid[hand][index] = FALSE;
	}
}

real vr_player_sprint_scale(long unit_index)
{
	short local = local_player_get_next(NONE);
	long player = local != NONE ? local_player_get_player_index(local) : NONE;
	/* Stock servers negotiate no sprint capability. Keep their movement
	contract unchanged, including when this VR client happens to host. */
	if (game_connection() != _game_connection_local || player == NONE ||
		player_get(player)->unit_index != unit_index || cinematic_in_progress() ||
		director_inhibited_input(local))
		return 1.0f;
	return 1.0f + 0.5f * vr_sprint_effort();
}

real vr_player_collision_radius(long unit_index, real stock)
{
	static int generation = -1, enabled;
	short local = local_player_get_next(NONE);
	long player = local != NONE ? local_player_get_player_index(local) : NONE;
	real units = vr_units_per_metre();
	if (generation != vr_settings_generation())
	{
		generation = vr_settings_generation();
		enabled = config_boolean("vr.close_contact");
	}
	if (!enabled || !vr_active() || game_connection() != _game_connection_local ||
		player == NONE || player_get(player)->unit_index != unit_index ||
		object_get(unit_index)->object.parent_object_index != NONE || !isfinite(stock) || stock <= 0.0f)
		return stock;
	/* Keep a solid capsule, unchanged overall height and foot position.
	Never alter shared tags or another player's/network collider. */
	return MIN(stock, MAX(0.18f * units, stock - MIN(stock * 0.15f, 0.05f * units)));
}

real vr_render_units_per_metre(
	void)
{
	return vr_units_per_metre();
}

int vr_render_first_person_mirrored(
	void)
{
	return vr_render.stereo && vr_hand_aiming() && vr_weapon_hand() == 0;
}

int vr_render_first_person_vehicles(
	void)
{
	return vr_first_person_vehicles();
}

int vr_render_hide_first_person_weapon(
	void)
{
	/* driving or on a turret, the player's own gun is put away */
	return vr_render.cinematic_view || (vr_seat_view() && (vr_render.seat.driver || vr_render.seat.gunner));
}

int vr_render_hand_aiming(
	void)
{
	return vr_hand_aiming();
}

int vr_render_hand_origin(
	long unit_index,
	real_point3d *origin)
{
	if (!vr_render.hand_origin_valid || unit_index != vr_render.hand_origin_unit ||
		!vr_hand_aiming() || game_connection() != _game_connection_local)
		return FALSE;
	*origin = vr_render.hand_origin;
	return TRUE;
}

int vr_render_aiming(
	void)
{
	return vr_aiming();
}

/* ---------- the first-person arms (vr.arms)

The first-person weapon's animation poses the gun and the arms holding it
from the camera; with the hand aiming, its camera is placed for the gun to
sit in the right hand, which carries the arms along with the gun. "ik"
gives the arms shoulders where the body is and solves each arm's upper arm
and forearm to reach its hand: the right one the gun's grip as animated,
the left one the left controller (or, held near the gun, its grip as
animated). "hidden" shows the gun alone; "animated" leaves the arms as the
animation has them. */

/* as first_person_weapons.c and model_animations.c lay it out */
struct vr_animation_graph_node
{
	char name[TAG_STRING_LENGTH+1];
	short next_sibling_node_index;
	short first_child_node_index;
	short parent_node_index;
	word pad;
	unsigned long flags;
	real_vector3d base_vector;
	real range;
	long pad1;
};

enum
{
	_vr_arm_upper,
	_vr_arm_fore,
	_vr_arm_hand,
	NUMBER_OF_VR_ARM_BONES
};

static short vr_find_node(
	struct animation_graph *graph,
	char const *side,
	char const *bone)
{
	short index;

	for (index = 0; index < graph->nodes.count && index < MAXIMUM_NODES_PER_ANIMATION; index++)
	{
		struct vr_animation_graph_node const *node =
			TAG_BLOCK_GET_ELEMENT(&graph->nodes, index, struct vr_animation_graph_node);

		if (strstr(node->name, side) && strstr(node->name, bone))
			return index;
	}
	return NONE;
}

static real vr_length(real_vector3d const *v)
{
	return (real)sqrt(v->i * v->i + v->j * v->j + v->k * v->k);
}

static void vr_point_minus(real_point3d const *a, real_point3d const *b, real_vector3d *out)
{
	out->i = a->x - b->x;
	out->j = a->y - b->y;
	out->k = a->z - b->z;
}

/* the rotation (as a matrix applied to column vectors) taking direction
a to direction b, the shortest way */
static void vr_rotation_between(
	real_vector3d const *a,
	real_vector3d const *b,
	real rotation[3][3])
{
	real_vector3d u = *a, v = *b, axis;
	real c, s, t, length;

	normalize3d(&u);
	normalize3d(&v);
	cross_product3d(&u, &v, &axis);
	s = vr_length(&axis);
	c = u.i * v.i + u.j * v.j + u.k * v.k;
	if (s < 1e-6f)
	{
		memset(rotation, 0, sizeof(real) * 9);
		if (c >= 0.0f)
			rotation[0][0] = rotation[1][1] = rotation[2][2] = 1.0f;
		else
		{
			real_vector3d basis = { 1.0f, 0.0f, 0.0f };
			int row, column;
			if (fabs(u.i) > 0.8f) { basis.i = 0.0f; basis.j = 1.0f; }
			cross_product3d(&u, &basis, &axis);
			normalize3d(&axis);
			for (row = 0; row < 3; row++)
				for (column = 0; column < 3; column++)
					rotation[row][column] = 2.0f * axis.n[row] * axis.n[column] - (row == column ? 1.0f : 0.0f);
		}
		return;
	}
	length = s;
	axis.i /= length;
	axis.j /= length;
	axis.k /= length;
	t = 1.0f - c;
	rotation[0][0] = c + axis.i * axis.i * t;
	rotation[0][1] = axis.i * axis.j * t - axis.k * s;
	rotation[0][2] = axis.i * axis.k * t + axis.j * s;
	rotation[1][0] = axis.j * axis.i * t + axis.k * s;
	rotation[1][1] = c + axis.j * axis.j * t;
	rotation[1][2] = axis.j * axis.k * t - axis.i * s;
	rotation[2][0] = axis.k * axis.i * t - axis.j * s;
	rotation[2][1] = axis.k * axis.j * t + axis.i * s;
	rotation[2][2] = c + axis.k * axis.k * t;
}

static void vr_rotate_vector(real rotation[3][3], real_vector3d const *in, real_vector3d *out)
{
	real_vector3d v = *in;

	out->i = rotation[0][0] * v.i + rotation[0][1] * v.j + rotation[0][2] * v.k;
	out->j = rotation[1][0] * v.i + rotation[1][1] * v.j + rotation[1][2] * v.k;
	out->k = rotation[2][0] * v.i + rotation[2][1] * v.j + rotation[2][2] * v.k;
}

static void vr_multiply_rotations(real a[3][3], real b[3][3], real out[3][3])
{
	real r[3][3];
	int row, column;

	for (row = 0; row < 3; row++)
		for (column = 0; column < 3; column++)
			r[row][column] = a[row][0] * b[0][column] + a[row][1] * b[1][column] + a[row][2] * b[2][column];
	memcpy(out, r, sizeof(r));
}

/* a node moved from `from` to `to` and turned by `rotation` about itself:
the matrix of it, or of a node it carries (whose old matrix is `node`) */
static void vr_carry(
	real_matrix4x3 *node,
	real_point3d const *from,
	real_point3d const *to,
	real rotation[3][3])
{
	real_vector3d offset;

	vr_point_minus(&node->position, from, &offset);
	vr_rotate_vector(rotation, &offset, &offset);
	node->position.x = to->x + offset.i;
	node->position.y = to->y + offset.j;
	node->position.z = to->z + offset.k;
	vr_rotate_vector(rotation, &node->forward, &node->forward);
	vr_rotate_vector(rotation, &node->left, &node->left);
	vr_rotate_vector(rotation, &node->up, &node->up);
}

/* Bounded ancestry also handles graphs with missing or reordered nodes. */
static boolean vr_node_under(struct animation_graph *graph, short node, short root)
{
	short steps, count = MIN(graph->nodes.count, MAXIMUM_NODES_PER_ANIMATION);
	if (root < 0 || root >= count)
		return FALSE;
	for (steps = 0; steps < count && node >= 0 && node < count; steps++)
	{
		if (node == root)
			return TRUE;
		node = TAG_BLOCK_GET_ELEMENT(&graph->nodes, node, struct vr_animation_graph_node)->parent_node_index;
	}
	return FALSE;
}

static boolean vr_unit_vector(real_vector3d *v)
{
	real length = vr_length(v);
	if (!(length > 1e-5f && length < 1e8f))
		return FALSE;
	scale_vector3d(v, 1.0f / length, v);
	return TRUE;
}

static void vr_turn_subtree(struct animation_graph *graph, real_matrix4x3 *matrices,
	short root, real rotation[3][3])
{
	short index, count = MIN(graph->nodes.count, MAXIMUM_NODES_PER_ANIMATION);
	real_point3d pivot = matrices[root].position;
	for (index = 0; index < count; index++)
		if (vr_node_under(graph, index, root))
			vr_carry(&matrices[index], &pivot, &pivot, rotation);
}

/* Bend-plane history is per arm/rig, never shared with legs or the other eye.
 * Targets remain responsive; only the ambiguous elbow swivel is filtered. */
struct vr_limb_history {
    struct animation_graph *graph;
    double time;
    real_point3d shoulder;
    real_vector3d bend;
    boolean valid;
    boolean roll_valid;
    real roll;
};
static struct vr_limb_history vr_arm_history[4];

/* one arm: the shoulder, and the hand's target; bones[] the arm's nodes */
static void vr_solve_arm(
	struct animation_graph *graph,
	real_matrix4x3 *matrices,
	short bones[NUMBER_OF_VR_ARM_BONES],
	real_point3d shoulder,
	real_point3d const *target,
	real_vector3d const *pole,
	boolean hand_stays, struct vr_limb_history *history)
{
	real_matrix4x3 old[NUMBER_OF_VR_ARM_BONES];
	real rotation[NUMBER_OF_VR_ARM_BONES][3][3];
	real_point3d moved_to[NUMBER_OF_VR_ARM_BONES];
	real_vector3d upper, fore, reach, direction, bend, old_direction;
	real upper_length, fore_length, distance, along, across;
	real_point3d solved_target = *target;
	short moved[MAXIMUM_NODES_PER_ANIMATION];
	short index, bone;

	for (bone = 0; bone < NUMBER_OF_VR_ARM_BONES; bone++)
		old[bone] = matrices[bones[bone]];
	vr_point_minus(&old[_vr_arm_fore].position, &old[_vr_arm_upper].position, &upper);
	vr_point_minus(&old[_vr_arm_hand].position, &old[_vr_arm_fore].position, &fore);
	upper_length = vr_length(&upper);
	fore_length = vr_length(&fore);
	if (!isfinite(upper_length) || !isfinite(fore_length) || upper_length < 1e-4f || fore_length < 1e-4f)
		return;
	vr_point_minus(target, &shoulder, &reach);
	distance = vr_length(&reach);
	if (!isfinite(distance) || distance < 1e-4f)
		return;
	direction = reach;
	normalize3d(&direction);
	/* Small clavicle reach and mild proportional arm extension keep the
	 * shoulder attached. A held hand/gun retains its exact authored anchor. */
	if (distance > (upper_length + fore_length) * (history ? 0.996f : 0.999f))
	{
		real maximum = (upper_length + fore_length) * (history ? 0.996f : 0.999f);
		real travel = distance - maximum;
		travel = MIN(travel, 0.08f * vr_units_per_metre());
		shoulder.x += direction.i * travel;
		shoulder.y += direction.j * travel;
		shoulder.z += direction.k * travel;
		distance -= travel;
		if (history && distance > maximum) {
			real stretch = MIN(distance / maximum, 1.18f);
			upper_length *= stretch; fore_length *= stretch; maximum *= stretch;
		}
		distance = MIN(distance, maximum);
	}
    if (history) {
        real minimum = sqrtf(upper_length * upper_length + fore_length * fore_length -
            1.638304f * upper_length * fore_length); /* 145 degree flexion */
        if (distance < minimum) {
            real compress = MAX(0.65f, distance / minimum);
            upper_length *= compress; fore_length *= compress;
        }
    }
	if (distance < (real)fabs(upper_length - fore_length) + 1e-3f)
		distance = (real)fabs(upper_length - fore_length) + 1e-3f;
	solved_target.x = shoulder.x + direction.i * distance;
	solved_target.y = shoulder.y + direction.j * distance;
	solved_target.z = shoulder.z + direction.k * distance;
	/* the elbow: in the plane of the reach and the pole */
	{
		real dot = pole->i * direction.i + pole->j * direction.j + pole->k * direction.k;

		bend.i = pole->i - direction.i * dot;
		bend.j = pole->j - direction.j * dot;
		bend.k = pole->k - direction.k * dot;
		if (vr_length(&bend) < 1e-4f)
		{
			/* Looking straight along the pole must not snap back to the
			 * animation. First keep the old bend, then use a stable side. */
			dot = dot_product3d(&upper, &direction);
			bend = upper;
			bend.i -= dot * direction.i; bend.j -= dot * direction.j; bend.k -= dot * direction.k;
			if (vr_length(&bend) < 1e-4f)
			{
				real_vector3d basis = {0, 0, 1};
				if (fabsf(direction.k) > 0.9f) basis = (real_vector3d){0, 1, 0};
				cross_product3d(&direction, &basis, &bend);
			}
		}
        if (history) {
            double now = vr_pose_time(), dt = now - history->time;
            real_vector3d previous = history->bend, travel;
            real confidence = PIN(vr_length(&bend) / 0.35f, 0.0f, 1.0f);
            vr_point_minus(&shoulder, &history->shoulder, &travel);
            dot = dot_product3d(&previous, &direction);
            previous.i -= dot * direction.i; previous.j -= dot * direction.j; previous.k -= dot * direction.k;
            if (history->valid && history->graph == graph && dt >= 0 && dt < 0.25 &&
                vr_length(&travel) < 0.5f * vr_units_per_metre() && vr_unit_vector(&previous) && vr_unit_vector(&bend)) {
                real_vector3d tangent;
                real angle, change;
                cross_product3d(&previous, &bend, &tangent);
                angle = atan2f(dot_product3d(&tangent, &direction), dot_product3d(&previous, &bend));
                change = angle * (1.0f - expf(-18.0f * MIN(dt, 0.05))) * confidence;
                change = PIN(change, -8.0f * dt, 8.0f * dt);
                cross_product3d(&direction, &previous, &tangent);
                bend.i = previous.i * cosf(change) + tangent.i * sinf(change);
                bend.j = previous.j * cosf(change) + tangent.j * sinf(change);
                bend.k = previous.k * cosf(change) + tangent.k * sinf(change);
            } else history->roll_valid = FALSE;
            normalize3d(&bend);
            history->valid = TRUE; history->graph = graph; history->time = now;
            history->shoulder = shoulder; history->bend = bend;
        } else normalize3d(&bend);
    }
    along = (upper_length * upper_length + distance * distance - fore_length * fore_length) / (2.0f * distance);
	across = upper_length * upper_length - along * along;
	across = across > 0.0f ? (real)sqrt(across) : 0.0f;
	moved_to[_vr_arm_upper] = shoulder;
	moved_to[_vr_arm_fore].x = shoulder.x + direction.i * along + bend.i * across;
	moved_to[_vr_arm_fore].y = shoulder.y + direction.j * along + bend.j * across;
	moved_to[_vr_arm_fore].z = shoulder.z + direction.k * along + bend.k * across;
	moved_to[_vr_arm_hand] = solved_target;

	/* each bone turned to point at the next joint, and the hand carried by
	the forearm's turn */
	vr_point_minus(&moved_to[_vr_arm_fore], &moved_to[_vr_arm_upper], &reach);
	vr_rotation_between(&upper, &reach, rotation[_vr_arm_upper]);
	vr_rotate_vector(rotation[_vr_arm_upper], &fore, &old_direction);
	vr_point_minus(&moved_to[_vr_arm_hand], &moved_to[_vr_arm_fore], &reach);
	vr_rotation_between(&old_direction, &reach, rotation[_vr_arm_fore]);
	vr_multiply_rotations(rotation[_vr_arm_fore], rotation[_vr_arm_upper], rotation[_vr_arm_fore]);
	if (hand_stays)
	{
		/* the hand holding the gun stays as the controller put it, and with
		it the gun (the hand's child) */
		memset(rotation[_vr_arm_hand], 0, sizeof(rotation[_vr_arm_hand]));
		rotation[_vr_arm_hand][0][0] = rotation[_vr_arm_hand][1][1] = rotation[_vr_arm_hand][2][2] = 1.0f;
		moved_to[_vr_arm_hand] = old[_vr_arm_hand].position;
	}
	else
	{
		memcpy(rotation[_vr_arm_hand], rotation[_vr_arm_fore], sizeof(rotation[_vr_arm_hand]));
	}

	/* every node under a moved bone goes with the nearest one above it;
	the graph lists parents before children */
	for (index = 0; index < graph->nodes.count && index < MAXIMUM_NODES_PER_ANIMATION; index++)
	{
		struct vr_animation_graph_node const *node =
			TAG_BLOCK_GET_ELEMENT(&graph->nodes, index, struct vr_animation_graph_node);

		moved[index] = NONE;
		for (bone = 0; bone < NUMBER_OF_VR_ARM_BONES; bone++)
		{
			if (bones[bone] == index)
				moved[index] = bone;
		}
		if (moved[index] == NONE && node->parent_node_index >= 0 && node->parent_node_index < index)
			moved[index] = moved[node->parent_node_index];
		if (moved[index] != NONE)
		{
			bone = moved[index];
			vr_carry(&matrices[index], &old[bone].position, &moved_to[bone], rotation[bone]);
		}
	}
}

/* ---------- the free hand's fingers (vr.fingers) and its touch

The hand that is not on the gun takes the fingers' places on its controller
(vr_finger_pose: a point, a thumbs up, a fist) and meets the world as a hand
would: it stops at a wall rather than going in, and pressed to it its
fingers lie flat along it (unless they grip), as Half-Life: Alyx's hands
take the shape of what they touch.

Each finger's joints are found by name: "finger" and its number (0 the
thumb to 4 the little finger), or "thumb", "index", "middle", "ring",
"pinky". Joint directions are posed absolutely in the controller's palm
frame, carrying each subtree while preserving segment lengths. The legacy
vr.diag_finger_sign setting is no longer used. */

/* a rotation of `angle` radians about the unit `axis` (column vectors) */
static void vr_axis_rotation(
	real_vector3d const *axis,
	real angle,
	real rotation[3][3])
{
	real c = (real)cos(angle), s = (real)sin(angle), t = 1.0f - c;
	real x = axis->i, y = axis->j, z = axis->k;

	rotation[0][0] = c + x * x * t;
	rotation[0][1] = x * y * t - z * s;
	rotation[0][2] = x * z * t + y * s;
	rotation[1][0] = y * x * t + z * s;
	rotation[1][1] = c + y * y * t;
	rotation[1][2] = y * z * t - x * s;
	rotation[2][0] = z * x * t - y * s;
	rotation[2][1] = z * y * t + x * s;
	rotation[2][2] = c + z * z * t;
}

/* Share controller roll along the arm rather than putting its entire twist at
 * the wrist skin seam. Bone positions and the complete hand/gun subtree stay
 * fixed. Reference frames come from this weapon's animated rig, not guessed
 * local bone axes. Swing/twist decomposition keeps wrist bending independent. */
static void vr_distribute_arm_twist(real_matrix4x3 *matrices, short chain[3],
	real_matrix4x3 const *old_fore, real_matrix4x3 const *old_hand, struct vr_limb_history *history)
{
	real_matrix4x3 *fore = &matrices[chain[_vr_arm_fore]];
	real_matrix4x3 *hand = &matrices[chain[_vr_arm_hand]];
	real_matrix3x3 delta;
	real_vector3d expected[3], desired[3], axis;
	real_quaternion q;
	real angle, rotation[3][3];
	int a, b, row, column;
	vr_point_minus(&hand->position, &fore->position, &axis);
	if (!vr_unit_vector(&axis)) return;
	for (a = 0; a < 3; ++a)
	{
		expected[a] = (real_vector3d){0, 0, 0};
		desired[a] = *(real_vector3d const *)hand->n[a];
		for (b = 0; b < 3; ++b)
		{
			real amount = dot_product3d((real_vector3d const *)old_fore->n[b],
				(real_vector3d const *)old_hand->n[a]);
			for (row = 0; row < 3; ++row) expected[a].n[row] += fore->n[b][row] * amount;
		}
		if (!vr_unit_vector(&expected[a]) || !vr_unit_vector(&desired[a])) return;
	}
	for (row = 0; row < 3; ++row)
		for (column = 0; column < 3; ++column)
		{
			delta.n[row][column] = 0;
			for (a = 0; a < 3; ++a) delta.n[row][column] += desired[a].n[row] * expected[a].n[column];
		}
	matrix3x3_rotation_to_quaternion(&delta, &q);
	angle = 2.0f * atan2f(dot_product3d(&q.v, &axis), q.w);
	if (!isfinite(angle)) return;
	angle = atan2f(sinf(angle), cosf(angle));
	/* Keep the quaternion's +/-pi branch continuous before applying the
	 * anatomical twist limit; the hand itself is never damped here. */
	if (history && history->roll_valid)
		angle = history->roll + atan2f(sinf(angle - history->roll), cosf(angle - history->roll));
	angle = PIN(angle, -1.92f, 1.92f);
	if (history) { history->roll = angle; history->roll_valid = TRUE; }
	vr_axis_rotation(&axis, angle * 0.85f, rotation);
	vr_rotate_vector(rotation, &fore->forward, &fore->forward);
	vr_rotate_vector(rotation, &fore->left, &fore->left);
	vr_rotate_vector(rotation, &fore->up, &fore->up);
	vr_point_minus(&fore->position, &matrices[chain[_vr_arm_upper]].position, &axis);
	if (vr_unit_vector(&axis))
	{
		real_matrix4x3 *upper = &matrices[chain[_vr_arm_upper]];
		vr_axis_rotation(&axis, angle * 0.25f, rotation);
		vr_rotate_vector(rotation, &upper->forward, &upper->forward);
		vr_rotate_vector(rotation, &upper->left, &upper->left);
		vr_rotate_vector(rotation, &upper->up, &upper->up);
	}
}

/* the finger (0 thumb .. 4 little finger) a node's name says it is a
joint of, on that side; NONE when it is not a finger's */
static short vr_finger_of(
	char const *name,
	char const *side)
{
	static char const *const names[] = { "thumb", "index", "middle", "ring", "pinky" };
	char const *finger;
	short index;

	if (!strstr(name, side))
		return NONE;
	finger = strstr(name, "finger");
	if (finger && finger[6] >= '0' && finger[6] <= '4')
		return (short)(finger[6] - '0');
	for (index = 0; index < 5; index++)
	{
		if (strstr(name, names[index]))
			return index;
	}
	return NONE;
}

/* pressing into the world, felt: a buzz as strong as the press is deep -
faint at a touch, full 12 cm in - renewed every frame, so it follows the
hand in and out (the frame's buzz lasts a little longer than the frame) */
static void vr_pressure_buzz(
	int hand,
	real depth_metres)
{
	real strength;

	if (depth_metres <= 0.0f)
		return;
	strength = 0.08f + 0.72f * (depth_metres / 0.12f);
	vr_haptic(hand, strength > 0.8f ? 0.8f : strength, 0.03f);
}

/* what the free hand meets: the level's walls, and the things in it a hand
can lay on - vehicles, scenery, machines (doors, lifts), controls, other
characters, weapons, equipment and other solid object types */
#define VR_HAND_TOUCH_FLAGS (FLAG(_collision_test_structure_bit) | FLAG(_collision_test_objects_bit) | \
	_collision_test_objects_all_types_flags)

/* The held first-person gun has no world collider at its tracked pose.
Use a small capsule from its authored hand anchors; never move the weapon's
gameplay collider. This is a tactile approximation, not mesh-perfect contact. */
static struct {
	real_point3d a, b;
	real radius;
	double time;
	boolean valid;
} vr_touch_weapon;

static boolean vr_weapon_trace(real_point3d const *from, real_vector3d const *v,
	real extra_radius, struct collision_result *result)
{
	boolean hit = FALSE;
	real best = 1.0f, radius = vr_touch_weapon.radius + extra_radius;
	real_vector3d axis, p, d, q;
	real length, along, velocity, a, b, c, discriminant, t;
	boolean weapon_hit = FALSE;
	if (!vr_touch_weapon.valid || vr_touch_weapon.time != vr_pose_time()) return hit;
	vr_point_minus(&vr_touch_weapon.b, &vr_touch_weapon.a, &axis);
	length = vr_length(&axis);
	if (!isfinite(length) || length < 1e-5f) return hit;
	scale_vector3d(&axis, 1.0f / length, &axis);
	vr_point_minus(from, &vr_touch_weapon.a, &p);
	along = dot_product3d(&p, &axis);
	velocity = dot_product3d(v, &axis);
	for (int i = 0; i < 3; i++) {
		d.n[i] = v->n[i] - velocity * axis.n[i];
		q.n[i] = p.n[i] - along * axis.n[i];
	}
	a = dot_product3d(&d, &d); b = dot_product3d(&q, &d);
	c = dot_product3d(&q, &q) - radius * radius;
	discriminant = b * b - a * c;
	if (a > 1e-10f && discriminant >= 0.0f) {
		t = (-b - sqrtf(discriminant)) / a;
		if (c < 0.0f && along >= 0 && along <= length) t = 0.0f;
		if (t >= 0 && t <= best && along + t * velocity >= 0 && along + t * velocity <= length) {
			best = t; weapon_hit = TRUE;
		}
	}
	for (int end = 0; end < 2; end++) {
		vr_point_minus(from, end ? &vr_touch_weapon.b : &vr_touch_weapon.a, &q);
		a = dot_product3d(v, v); b = dot_product3d(&q, v);
		c = dot_product3d(&q, &q) - radius * radius;
		discriminant = b * b - a * c;
		if (a > 1e-10f && discriminant >= 0) {
			t = c <= 0 ? 0 : (-b - sqrtf(discriminant)) / a;
			if (t >= 0 && t <= best) { best = t; weapon_hit = TRUE; }
		}
	}
	if (weapon_hit) {
		memset(result, 0, sizeof(*result));
		result->t = best;
		for (int i = 0; i < 3; i++) result->point.n[i] = from->n[i] + best * v->n[i];
		vr_point_minus(&result->point, &vr_touch_weapon.a, &q);
		t = PIN(dot_product3d(&q, &axis), 0.0f, length);
		for (int component = 0; component < 3; component++) result->plane.n.n[component] = q.n[component] - t * axis.n[component];
		if (!vr_unit_vector(&result->plane.n)) result->plane.n = (real_vector3d){0, 0, 1};
		return TRUE;
	}
	return hit;
}

static boolean vr_touch_trace(real_point3d const *from, real_vector3d const *v,
    long ignore, struct collision_result *result)
{
    struct collision_result weapon;
    boolean hit = collision_test_vector(VR_HAND_TOUCH_FLAGS, from, v, ignore, result);
    if (vr_weapon_trace(from, v, 0, &weapon) && (!hit || weapon.t < result->t)) {
        *result = weapon;
        return TRUE;
    }
    return hit;
}

/* the free hand's target kept out of the walls and what it touches, as
seen from the head: TRUE when something stopped it (the hand pressed to it),
with how far the controller is past where the hand stopped, in metres. The
player's own body and attached world objects are ignored; the tracked gun
is added separately at its visible first-person pose */
static boolean vr_hand_out_of_walls(
	int controller, real_point3d *target,
	real_vector3d const *forward,
	real *depth_metres,
	real_vector3d *normal)
{
	real units = vr_units_per_metre();
	real_point3d head = vr_render.head_camera.position;
	real_vector3d vector;
	struct collision_result collision, nearest;
	real fraction = 1.0f;
	int probe;
	boolean hit = FALSE;
	long local_player_index = local_player_get_next(NONE);
	long player_index = local_player_index != NONE ? local_player_get_player_index(local_player_index) : NONE;
	long unit_index = player_index != NONE ? player_get(player_index)->unit_index : NONE;
	long ignore_index = unit_index != NONE ? object_get_ultimate_parent(unit_index) : NONE;
	static struct {
		long unit;
		double time;
		boolean valid, hit;
		real_point3d target;
		real_vector3d normal;
		real depth;
	} contact[2];
	if (contact[controller].valid && contact[controller].unit == unit_index &&
		contact[controller].time == vr_pose_time()) {
		*target = contact[controller].target;
		*normal = contact[controller].normal;
		*depth_metres = contact[controller].depth;
		return contact[controller].hit;
	}

	*depth_metres = 0.0f;
	normal->i = normal->j = normal->k = 0.0f;
	/* Sample a small palm volume, not just the wrist's zero-width ray.
	All rays originate at the head, so probes cannot start behind a wall. */
	for (probe = 0; probe < 7; probe++)
	{
		real_point3d palm = *target;
		int axis = (probe - 1) / 2;
		palm.x += forward->i * 0.075f * units;
		palm.y += forward->j * 0.075f * units;
		palm.z += forward->k * 0.075f * units;
		if (probe) palm.n[axis] += (probe & 1 ? 1.0f : -1.0f) * 0.025f * units;
		vr_point_minus(&palm, &head, &vector);
		if (collision_test_vector(VR_HAND_TOUCH_FLAGS, &head, &vector, ignore_index, &collision) &&
			isfinite(collision.t) && collision.t >= 0.0f && collision.t <= fraction)
		{
			fraction = collision.t;
			nearest = collision;
			hit = TRUE;
		}
	}
	if (hit) {
	vr_point_minus(target, &head, &vector);
	{
		real length = vr_length(&vector);
		real stop = fraction * length - 0.005f * units;

		if (length < 1e-4f)
			return FALSE;
		stop = stop < 0.0f ? 0.0f : stop;
		*depth_metres = units > 0.0f ? (length - stop) / units : 0.0f;
		target->x = head.x + vector.i / length * stop;
		target->y = head.y + vector.j / length * stop;
		target->z = head.z + vector.k / length * stop;
	}
	*normal = nearest.plane.n;
    }
    {
        static real_point3d previous[2];
        static double previous_time[2];
        static long previous_unit[2];
        real_point3d palm, requested;
        double now = vr_pose_time(), dt = now - previous_time[controller];
        for (int component = 0; component < 3; component++)
            palm.n[component] = target->n[component] + forward->n[component] * 0.075f * units;
        requested = palm;
        if (vr_touch_weapon.valid && vr_touch_weapon.time == now) {
            real_vector3d axis, offset, n;
            real length, along, distance, radius = vr_touch_weapon.radius + 0.025f * units;
            /* Swept contact prevents a fast palm crossing the barrel.
            Reset after map/tracking gaps; do not drag an old hand through a teleport. */
            if (dt > 0 && dt < 0.10 && previous_unit[controller] == unit_index) {
                vr_point_minus(&palm, &previous[controller], &vector);
                if (vr_length(&vector) < 0.5f * units &&
                    vr_weapon_trace(&previous[controller], &vector, 0.025f * units, &collision) && collision.t > 0.0001f) {
                    for (int component = 0; component < 3; component++)
                        palm.n[component] = collision.point.n[component] + collision.plane.n.n[component] * 0.001f * units;
                    *normal = collision.plane.n;
                    hit = TRUE;
                }
            }
            vr_point_minus(&vr_touch_weapon.b, &vr_touch_weapon.a, &axis);
            length = vr_length(&axis);
            if (vr_unit_vector(&axis)) {
                vr_point_minus(&palm, &vr_touch_weapon.a, &offset);
                along = PIN(dot_product3d(&offset, &axis), 0.0f, length);
                for (int component = 0; component < 3; component++) n.n[component] = offset.n[component] - along * axis.n[component];
                distance = vr_length(&n);
                if (isfinite(distance) && distance < radius) {
                    if (!vr_unit_vector(&n)) n = (real_vector3d){0, 0, 1};
                    for (int component = 0; component < 3; component++) palm.n[component] += n.n[component] * (radius - distance);
                    *normal = n;
                    hit = TRUE;
                }
            }
        }
        vr_point_minus(&palm, &requested, &vector);
        /* A gun must not push the palm through a wall when constraints
        conflict. World collision wins over the held-gun approximation. */
        if (vr_length(&vector) > 1e-5f &&
            collision_test_vector(VR_HAND_TOUCH_FLAGS, &requested, &vector, ignore_index, &collision)) {
            scale_vector3d(&vector, MAX(0.0f, collision.t - 0.001f), &vector);
            for (int axis = 0; axis < 3; axis++) palm.n[axis] = requested.n[axis] + vector.n[axis];
            *normal = collision.plane.n;
        }
        *depth_metres = MAX(*depth_metres, vr_length(&vector) / units);
        for (int coordinate = 0; coordinate < 3; coordinate++) target->n[coordinate] += vector.n[coordinate];
        previous[controller] = palm;
        previous_time[controller] = now;
        previous_unit[controller] = unit_index;
    }
	/* Both eyes use the same constrained hand, including a swept hit whose
	controller ended on the far side of the weapon this frame. */
	contact[controller].unit = unit_index;
	contact[controller].time = vr_pose_time();
	contact[controller].valid = TRUE;
	contact[controller].hit = hit;
	contact[controller].target = *target;
	contact[controller].normal = *normal;
	contact[controller].depth = *depth_metres;
	return hit;
}

/* A finger's order comes from ancestry, not name suffix spelling. */
static void vr_finger_nodes(struct animation_graph *graph, short hand, char const *side, short joints[5][3])
{
	short i, f, depth, at, steps, count = MIN(graph->nodes.count, MAXIMUM_NODES_PER_ANIMATION);
	for (f = 0; f < 5; f++)
		for (depth = 0; depth < 3; depth++)
			joints[f][depth] = NONE;
	for (i = 0; i < count; i++)
	{
		struct vr_animation_graph_node const *node = TAG_BLOCK_GET_ELEMENT(&graph->nodes, i, struct vr_animation_graph_node);
		f = vr_finger_of(node->name, side);
		if (f == NONE || !vr_node_under(graph, i, hand))
			continue;
		depth = 0;
		for (at = node->parent_node_index, steps = 0; at >= 0 && at < count && steps < count; steps++)
		{
			node = TAG_BLOCK_GET_ELEMENT(&graph->nodes, at, struct vr_animation_graph_node);
			if (vr_finger_of(node->name, side) == f)
				depth++;
			at = node->parent_node_index;
		}
		if (depth < 3)
			joints[f][depth] = i;
	}
}

/* Frames built from joint positions avoid depending on a model's bone axes. */
static void vr_orient_hand(struct animation_graph *graph, real_matrix4x3 *matrices, short hand,
	short joints[5][3], real_vector3d const *forward, real_vector3d const *up)
{
	real_vector3d base[3], wanted[3] = { *forward, *up };
	real rotation[3][3], dot;
	int row, column;
	if (joints[1][0] == NONE || joints[2][0] == NONE || joints[4][0] == NONE)
		return;
	vr_point_minus(&matrices[joints[2][0]].position, &matrices[hand].position, &base[0]);
	vr_point_minus(&matrices[joints[1][0]].position, &matrices[joints[4][0]].position, &base[1]);
	if (!vr_unit_vector(&base[0]))
		return;
	dot = dot_product3d(&base[0], &base[1]);
	base[1].i -= dot * base[0].i;
	base[1].j -= dot * base[0].j;
	base[1].k -= dot * base[0].k;
	if (!vr_unit_vector(&base[1]))
		return;
	cross_product3d(&base[0], &base[1], &base[2]);
	cross_product3d(&wanted[0], &wanted[1], &wanted[2]);
	for (row = 0; row < 3; row++)
		for (column = 0; column < 3; column++)
			rotation[row][column] = wanted[0].n[row] * base[0].n[column] +
				wanted[1].n[row] * base[1].n[column] + wanted[2].n[row] * base[2].n[column];
	vr_turn_subtree(graph, matrices, hand, rotation);
}

/* Absolute segment directions replace the animation's existing grip curl.
test26: each joint its own curl (the base, middle and last: a finger
meeting a wall bends joint by joint, vr_touch_fingers) */
static boolean vr_pose_finger_joints(struct animation_graph *graph, real_matrix4x3 *matrices,
	short hand, short joints[3], int finger, real const curls[3], real_vector3d const *palm,
	real_vector3d const *forward, real_vector3d const *up, real_point3d *tip)
{
	real curl = curls[0];
	static real const rest[3] = { 0.12f, 0.15f, 0.10f }, bend[3] = { 1.25f, 1.6f, 1.1f };
	real_vector3d base, axis, direction, wanted;
	real angle = 0.0f, dot, rotation[3][3], length = 0.0f;
	int depth;
	if (joints[0] == NONE || joints[1] == NONE || joints[2] == NONE)
		return FALSE;
	vr_point_minus(&matrices[joints[0]].position, &matrices[hand].position, &base);
	dot = dot_product3d(&base, palm);
	base.i -= dot * palm->i; base.j -= dot * palm->j; base.k -= dot * palm->k;
	if (finger == 0)
	{
		base.i = (up->i * 0.9f + forward->i * 0.3f) * (1.0f - curl) +
			(forward->i * 0.75f + palm->i * 0.55f + up->i * 0.1f) * curl;
		base.j = (up->j * 0.9f + forward->j * 0.3f) * (1.0f - curl) +
			(forward->j * 0.75f + palm->j * 0.55f + up->j * 0.1f) * curl;
		base.k = (up->k * 0.9f + forward->k * 0.3f) * (1.0f - curl) +
			(forward->k * 0.75f + palm->k * 0.55f + up->k * 0.1f) * curl;
	}
	if (!vr_unit_vector(&base))
		return FALSE;
	cross_product3d(&base, palm, &axis);
	if (!vr_unit_vector(&axis))
		return FALSE;
	for (depth = 0; depth < 3; depth++)
	{
		short joint = joints[depth];
		if (depth < 2)
		{
			vr_point_minus(&matrices[joints[depth + 1]].position, &matrices[joint].position, &direction);
			length = vr_length(&direction);
		}
		else
		{
			/* Use the terminal bone's corresponding axis only when the middle
			bone establishes which axis points down the finger. */
			real_vector3d previous;
			real_vector3d *axes[3] = { &matrices[joints[1]].forward, &matrices[joints[1]].left, &matrices[joints[1]].up };
			real_vector3d *last[3] = { &matrices[joint].forward, &matrices[joint].left, &matrices[joint].up };
			real best = 0.8f, sign = 1.0f;
			int a, found = -1;
			vr_point_minus(&matrices[joint].position, &matrices[joints[1]].position, &previous);
			if (!vr_unit_vector(&previous))
				return FALSE;
			for (a = 0; a < 3; a++)
			{
				real_vector3d unit = *axes[a];
				if (!vr_unit_vector(&unit)) continue;
				dot = dot_product3d(&unit, &previous);
				if (fabs(dot) > best) { best = (real)fabs(dot); found = a; sign = dot < 0.0f ? -1.0f : 1.0f; }
			}
			if (found < 0) { *tip = matrices[joint].position; return TRUE; }
			scale_vector3d(last[found], sign, &direction);
		}
		if (!vr_unit_vector(&direction))
			return FALSE;
		curl = curls[depth];
		angle += finger == 0 ? (depth == 0 ? 0.0f : (depth == 1 ? 0.35f : 0.3f) * curl) : rest[depth] + bend[depth] * curl;
		vr_axis_rotation(&axis, angle, rotation);
		vr_rotate_vector(rotation, &base, &wanted);
		vr_rotation_between(&direction, &wanted, rotation);
		vr_turn_subtree(graph, matrices, joint, rotation);
		if (depth == 2)
		{
			tip->x = matrices[joint].position.x + wanted.i * length * 0.7f;
			tip->y = matrices[joint].position.y + wanted.j * length * 0.7f;
			tip->z = matrices[joint].position.z + wanted.k * length * 0.7f;
		}
	}
	return TRUE;
}

/* test26: a finger's joints given how far (`t`, 0 none to 1 all the way)
contact moves it from the curl it wants toward straight (`direction` -1:
the base first, as fingers lie flat on what the palm presses) or curled
(+1: the tip first, as fingertips pressed to a wall buckle) */
static void vr_finger_contact_curls(real wanted, int direction, real t, real curls[3])
{
	/* (at what share of the way each joint, base, middle and last, has
	moved all the way: the smaller, the sooner) */
	static real const flatten[3] = { 0.5f, 0.75f, 1.0f }, buckle[3] = { 1.0f, 0.75f, 0.5f };
	real const *pace = direction < 0 ? flatten : buckle;
	real goal = direction < 0 ? 0.0f : 1.0f;
	int depth;

	for (depth = 0; depth < 3; depth++)
	{
		real share = direction ? PIN(t / pace[depth], 0.0f, 1.0f) : 0.0f;

		curls[depth] = wanted + (goal - wanted) * share;
	}
}

/* how far (0..1) the posed finger's first segment that meets the world gets
before it does, 1 for none: the pose is left in `matrices` */
static real vr_finger_clearance(struct animation_graph *graph, real_matrix4x3 *matrices, short hand,
	short joints[3], int finger, real const curls[3], real_vector3d const *palm, real_vector3d const *forward,
	real_vector3d const *up, long ignore, boolean *valid)
{
	real_point3d tip;
	real score = 1.0f;
	int segment;

	*valid = vr_pose_finger_joints(graph, matrices, hand, joints, finger, curls, palm, forward, up, &tip);
	if (!*valid)
		return 1.0f;
	for (segment = 0; segment < 3; segment++)
	{
		real_point3d const *from = &matrices[joints[segment]].position;
		real_point3d const *to = segment < 2 ? &matrices[joints[segment + 1]].position : &tip;
		real_vector3d vector;
		struct collision_result hit;

		vr_point_minus(to, from, &vector);
		if (vr_length(&vector) > 1e-5f && vr_touch_trace(from, &vector, ignore, &hit))
			score = MIN(score, hit.t);
	}
	return score;
}

/* test26: the free hand's fingers against the world. Each finger takes the
curl its controller gives it; where that meets the world, it bends away
from it continuously (searched, not picked from a few poses: the old
search's jumps between a few curls were the snapping and twisting), joint by
joint (vr_finger_contact_curls), keeping the way it bent while it stays in
contact (it never flips between curling and straightening), and eases back
as the contact ends (into contact the world wins at once: never through a
wall). A palm pressed flat relaxes the loose fingers gradually. */
static int vr_touch_fingers(struct animation_graph *graph, real_matrix4x3 *matrices,
	int controller, short hand, short joints[5][3], real const curls[4], real_vector3d const *palm,
	real_vector3d const *forward, real_vector3d const *up, boolean pressed)
{
	/* Render poses are evaluated serially. No allocation; only the contact state persists.
	Every attempt starts from this frame's original hand matrices. */
	static real_matrix4x3 saved[MAXIMUM_NODES_PER_ANIMATION];
	static real contact_t[2][5], pressed_weight[2];
	static int contact_direction[2][5];
	static double last_time[2];
	double now = vr_pose_time(), dt = now - last_time[controller];
	boolean reset = dt <= 0.0 || dt > 0.25;
	short count = MIN(graph->nodes.count, MAXIMUM_NODES_PER_ANIMATION);
	short local = local_player_get_next(NONE);
	long player = local != NONE ? local_player_get_player_index(local) : NONE;
	long unit = player != NONE ? player_get(player)->unit_index : NONE;
	long ignore = unit != NONE ? object_get_ultimate_parent(unit) : NONE;
	real ease = reset ? 1.0f : 1.0f - expf(-14.0f * (real)MIN(dt, 0.05));
	int f, contacts = 0;

	if (reset)
	{
		memset(contact_t[controller], 0, sizeof(contact_t[controller]));
		memset(contact_direction[controller], 0, sizeof(contact_direction[controller]));
		pressed_weight[controller] = pressed ? 1.0f : 0.0f;
	}
	else
	{
		/* (the palm's press eased in and out over a tenth of a second) */
		pressed_weight[controller] += ((pressed ? 1.0f : 0.0f) - pressed_weight[controller]) *
			(1.0f - expf(-20.0f * (real)MIN(dt, 0.05)));
	}
	memcpy(saved, matrices, count * sizeof(*matrices));
	for (f = 0; f < 5; f++)
	{
		real curl = curls[f < 3 ? f : 3], joint_curls[3], t = 0.0f, clear;
		real *state_t = &contact_t[controller][f];
		int *state_direction = &contact_direction[controller][f];
		int direction = *state_direction;
		boolean valid;

		if (joints[f][0] == NONE || joints[f][1] == NONE || joints[f][2] == NONE)
			continue;
		/* a pressed palm relaxes the loose fingers (no step at 0.6 curl) */
		{
			real loose = 1.0f - PIN((curl - 0.45f) / 0.3f, 0.0f, 1.0f);

			curl *= 1.0f - 0.65f * loose * pressed_weight[controller];
		}
		vr_finger_contact_curls(curl, 0, 0.0f, joint_curls);
		clear = vr_finger_clearance(graph, matrices, hand, joints[f], f, joint_curls, palm, forward, up, ignore, &valid);
		memcpy(matrices, saved, count * sizeof(*matrices));
		if (!valid)
			continue;
		if (clear < 1.0f)
		{
			/* into contact: which way, kept while it lasts unless that way
			cannot clear and the other can (a fresh contact: the way that
			clears, straightening if both do) */
			real clear_way[2];
			int way;

			for (way = 0; way < 2; way++)
			{
				vr_finger_contact_curls(curl, way ? 1 : -1, 1.0f, joint_curls);
				clear_way[way] = vr_finger_clearance(graph, matrices, hand, joints[f], f, joint_curls, palm, forward, up,
					ignore, &valid);
				memcpy(matrices, saved, count * sizeof(*matrices));
			}
			if (!direction || clear_way[direction > 0] < 1.0f)
				direction = clear_way[0] >= 1.0f ? -1 : clear_way[1] >= 1.0f ? 1 : clear_way[1] > clear_way[0] ? 1 : -1;
			/* the least contact moves it that clears (bisected) */
			if (clear_way[direction > 0] < 1.0f)
			{
				t = 1.0f;
			}
			else
			{
				real low = 0.0f, high = 1.0f;
				int step;

				for (step = 0; step < 6; step++)
				{
					real middle = 0.5f * (low + high);

					vr_finger_contact_curls(curl, direction, middle, joint_curls);
					if (vr_finger_clearance(graph, matrices, hand, joints[f], f, joint_curls, palm, forward, up, ignore,
						&valid) >= 1.0f)
					{
						high = middle;
					}
					else
					{
						low = middle;
					}
					memcpy(matrices, saved, count * sizeof(*matrices));
				}
				t = high;
			}
			contacts++;
		}
		/* out of contact (or less of it): eased back, the way kept until it
		is all the way back; into it: at once */
		if (t < *state_t && direction == *state_direction && !reset)
		{
			real eased = *state_t + (t - *state_t) * ease;

			vr_finger_contact_curls(curl, direction ? direction : *state_direction, eased, joint_curls);
			if (vr_finger_clearance(graph, matrices, hand, joints[f], f, joint_curls, palm, forward, up, ignore,
				&valid) >= 1.0f)
			{
				t = eased;
			}
			memcpy(matrices, saved, count * sizeof(*matrices));
		}
		if (t <= 0.001f && clear >= 1.0f)
		{
			t = 0.0f;
			direction = 0;
		}
		*state_t = t;
		*state_direction = direction;
		vr_finger_contact_curls(curl, direction, t, joint_curls);
		{
			real_point3d tip;

			vr_pose_finger_joints(graph, matrices, hand, joints[f], f, joint_curls, palm, forward, up, &tip);
		}
		/* (the next finger poses from this one's result: the hand's own
		matrices, carried) */
		memcpy(saved, matrices, count * sizeof(*matrices));
	}
	last_time[controller] = now;
	return contacts;
}

/* ---------- the full body (vr.body "full")

The biped supplies the torso and legs. Its head and arm subtrees collapse
to their root points; the first-person arms supply the tracked hands. Only
a copy of the drawing matrices changes, leaving hit boxes and markers
alone. Immersive cutscenes use the normal complete character model. */

static boolean vr_body_setting(
	void)
{
	static int full = -1, generation;

	if (full < 0 || generation != vr_settings_generation())
	{
		generation = vr_settings_generation();
		full = !strcmp(config_string("vr.body"), "full") || !strcmp(config_string("vr.body"), "legs");
	}
	return full != 0;
}

boolean vr_render_full_body(
	long object_index)
{
	short local_player_index = local_player_get_next(NONE);
	long player_index = local_player_index != NONE ? local_player_get_player_index(local_player_index) : NONE;

	return vr_render.stereo && !vr_render.cinematic_view && vr_body_setting() && player_index != NONE &&
		player_get(player_index)->unit_index == object_index &&
		!TEST_FLAG(object_get(object_index)->object.damage_flags, _object_dead_bit) &&
		object_get(object_index)->object.parent_object_index == NONE &&
		object_get(object_index)->object.type == _object_type_biped;
}

/* Shared by the body and the first-person arms, once per predicted XR frame. */
static struct {
    long unit;
    double time;
    boolean valid, shoulders_valid;
    real yaw;
    real_point3d shoulders[2];
} vr_body_pose;

/* The head's yaw that stays steady at any pitch: looking straight down the
forward vector has no horizontal part (and its remains flip), but the top of
the head points forward then; looking up, the back of it does (test21) */
static boolean vr_head_yaw_vector(real_vector3d const *forward, real_vector3d const *up, real_vector3d *out)
{
    out->i = forward->i - forward->k * up->i;
    out->j = forward->j - forward->k * up->j;
    out->k = 0.0f;
    return vr_unit_vector(out);
}

/* the torso's wanted yaw: the head's, drawn halfway toward the hands when
both are tracked ahead (turning the body turns both; looking around turns
only the head), within 45 degrees */
static real vr_body_wanted_yaw(real_vector3d const *head_yaw, boolean hands_valid,
    real_point3d const *head, real_point3d const hands[2])
{
    real yaw = atan2f(head_yaw->j, head_yaw->i);
    if (hands_valid) {
        real units = vr_units_per_metre();
        real_vector3d mid = {{(hands[0].x + hands[1].x) * 0.5f - head->x, (hands[0].y + hands[1].y) * 0.5f - head->y, 0.0f}};
        if (vr_length(&mid) > 0.12f * units) {
            real toward = atan2f(mid.j, mid.i);
            real difference = atan2f(sinf(toward - yaw), cosf(toward - yaw));
            if (fabsf(difference) < 1.745f)
                yaw += 0.5f * PIN(difference, -0.785f, 0.785f);
        }
    }
    return yaw;
}

static real_vector3d vr_body_heading(long unit)
{
    real_vector3d heading;
    double now = vr_pose_time(), dt = now - vr_body_pose.time;
    real wanted, difference;
    real_point3d hands[2];
    real_vector3d f, u;
    boolean hands_valid = vr_hand_world(0, vr_render.game_camera_position.n, hands[0].n, f.n, u.n) &&
        vr_hand_world(1, vr_render.game_camera_position.n, hands[1].n, f.n, u.n);
    if (!vr_head_yaw_vector(&vr_render.head_camera.forward, &vr_render.head_camera.up, &heading)) {
        if (!vr_heading_forward(heading.n)) heading = (real_vector3d){1, 0, 0};
    }
    wanted = vr_body_wanted_yaw(&heading, hands_valid, &vr_render.head_camera.position, hands);
    if (!vr_body_pose.valid || vr_body_pose.unit != unit || dt < 0 || dt > 0.25) {
        vr_body_pose.yaw = wanted;
        vr_body_pose.valid = TRUE;
    } else if (dt > 0) {
        difference = atan2f(sinf(wanted - vr_body_pose.yaw), cosf(wanted - vr_body_pose.yaw));
        /* The neck can look around; the torso follows beyond a 15-degree
        comfort cone (test21: 25 before, with the head alone deciding), with a
        bounded, frame-rate independent turn. */
        if (fabsf(difference) > 0.261799f) {
            real turn = difference - copysignf(0.261799f, difference);
            vr_body_pose.yaw += turn * (1.0f - expf(-10.0f * MIN(dt, 0.05)));
            vr_body_pose.yaw = atan2f(sinf(vr_body_pose.yaw), cosf(vr_body_pose.yaw));
        }
    }
    vr_body_pose.unit = unit;
    vr_body_pose.time = now;
    heading = (real_vector3d){cosf(vr_body_pose.yaw), sinf(vr_body_pose.yaw), 0};
    return heading;
}

/* the neck pivot below and behind the eyes (vr_render_body_matrices) */
static void vr_neck_pivot(real_point3d const *eye, real_vector3d const *forward, real_vector3d const *up,
    real units, real_point3d *neck)
{
    real_vector3d yaw;
    real pitch, c, s;
    if (!vr_head_yaw_vector(forward, up, &yaw)) yaw = (real_vector3d){{1.0f, 0.0f, 0.0f}};
    pitch = asinf(PIN(forward->k, -1.0f, 1.0f));
    pitch = PIN(pitch, -1.0472f, 0.5236f);
    c = cosf(pitch); s = sinf(pitch);
    /* f' = yaw*c + z*s, u' = -yaw*s + z*c; neck = eye - (0.14 f' + 0.20 u') */
    for (int axis = 0; axis < 2; axis++)
        neck->n[axis] = eye->n[axis] - (0.14f * c - 0.20f * s) * yaw.n[axis] * units;
    neck->z = eye->z - (0.14f * s + 0.20f * c) * units;
}

/* Reuse the analytic limb solver with a fixed hip, a clamped ankle target
and a forward knee hint. Never lengthen a bone to meet an unreachable foot. */
static void vr_body_leg(struct animation_graph *graph, real_matrix4x3 *m,
    short chain[3], real_point3d target, real_vector3d pole)
{
    real_vector3d a, b, reach;
    real length, low, high;
    real_matrix4x3 foot = m[chain[2]];
    vr_point_minus(&m[chain[1]].position, &m[chain[0]].position, &a);
    vr_point_minus(&m[chain[2]].position, &m[chain[1]].position, &b);
    low = fabsf(vr_length(&a) - vr_length(&b)) + 0.002f;
    high = (vr_length(&a) + vr_length(&b)) * 0.998f;
    vr_point_minus(&target, &m[chain[0]].position, &reach);
    length = vr_length(&reach);
    if (!isfinite(length) || length < 0.0001f || high <= low) return;
    scale_vector3d(&reach, PIN(length, low, high) / length, &reach);
    target.x = m[chain[0]].position.x + reach.i;
    target.y = m[chain[0]].position.y + reach.j;
    target.z = m[chain[0]].position.z + reach.k;
    vr_solve_arm(graph, m, chain, m[chain[0]].position, &target, &pole, FALSE, NULL);
    m[chain[2]].forward = foot.forward;
    m[chain[2]].left = foot.left;
    m[chain[2]].up = foot.up;
}

/* Keep idle feet planted while the torso follows the headset. Turn-in-place
 * steps move one foot at a time; walking/jumping retain the authored gait.
 * These are render targets only, never physics positions or network state. */
static void vr_body_plant_feet(long unit, struct object_datum const *object,
	real_matrix4x3 *matrices, short legs[2][3], boolean valid[2], real_point3d feet[2])
{
	static struct {
		boolean valid;
		long unit;
		double time, start_time;
		int step;
		real yaw[2];
		real_matrix4x3 planted[2], start, goal;
	} stance;
	double now = vr_pose_time();
	real units = vr_units_per_metre(), yaw = vr_body_pose.yaw;
	real speed = sqrtf(object->object.translational_velocity.i * object->object.translational_velocity.i +
		object->object.translational_velocity.j * object->object.translational_velocity.j);
	int side;
	boolean moving = speed > 0.006f * units || fabsf(object->object.translational_velocity.k) > 0.015f * units;
	if (!valid[0] || !valid[1]) { stance.valid = FALSE; return; }
	if (!stance.valid || stance.unit != unit || now < stance.time || now - stance.time > 0.25 || moving)
	{
		stance.valid = TRUE; stance.unit = unit; stance.step = -1;
		for (side = 0; side < 2; ++side)
		{
			stance.planted[side] = matrices[legs[side][2]];
			stance.yaw[side] = yaw;
		}
	}
	stance.time = now;
	if (moving) return;
	if (stance.step < 0)
	{
		real largest = 1.0f;
		int selected = -1;
		for (side = 0; side < 2; ++side)
		{
			real_vector3d gap;
			real turn = fabsf(atan2f(sinf(yaw - stance.yaw[side]), cosf(yaw - stance.yaw[side])));
			vr_point_minus(&feet[side], &stance.planted[side].position, &gap);
			real need = MAX(vr_length(&gap) / (0.18f * units), turn / 0.61f);
			if (need > largest) { largest = need; selected = side; }
		}
		if (selected >= 0)
		{
			stance.step = selected;
			stance.start_time = now;
			stance.start = stance.planted[selected];
			stance.goal = matrices[legs[selected][2]];
			stance.yaw[selected] = yaw;
		}
	}
	if (stance.step >= 0)
	{
		real t = PIN((real)((now - stance.start_time) / 0.22), 0.0f, 1.0f);
		real eased = t * t * (3.0f - 2.0f * t);
		real_matrix4x3 *foot = &stance.planted[stance.step];
		for (int axis = 0; axis < 3; ++axis)
		{
			foot->position.n[axis] = stance.start.position.n[axis] +
				(stance.goal.position.n[axis] - stance.start.position.n[axis]) * eased;
			foot->forward.n[axis] = stance.start.forward.n[axis] * (1.0f - eased) + stance.goal.forward.n[axis] * eased;
			foot->up.n[axis] = stance.start.up.n[axis] * (1.0f - eased) + stance.goal.up.n[axis] * eased;
		}
		foot->position.z += sinf(t * 3.14159265f) * 0.06f * units;
		if (vr_unit_vector(&foot->forward))
		{
			cross_product3d(&foot->up, &foot->forward, &foot->left);
			if (vr_unit_vector(&foot->left)) cross_product3d(&foot->forward, &foot->left, &foot->up);
		}
		if (t >= 1.0f) { *foot = stance.goal; stance.step = -1; }
	}
	for (side = 0; side < 2; ++side)
	{
		feet[side] = stance.planted[side].position;
		matrices[legs[side][2]].forward = stance.planted[side].forward;
		matrices[legs[side][2]].left = stance.planted[side].left;
		matrices[legs[side][2]].up = stance.planted[side].up;
	}
}

static struct { real_point3d position, elbow; double time; real animated; long unit; boolean valid; } vr_avatar_hands[2];

/* The observer palette keeps every body bone, before the local visibility
 * filter. The biped's own hierarchy/proportions are used, not FP bone indices. */
static void vr_publish_body(long unit, struct animation_graph *graph,
    real_matrix4x3 const *local, short count, real_vector3d heading)
{
    real_matrix4x3 m[MAXIMUM_NODES_PER_ANIMATION];
    real units = vr_units_per_metre();
    real_vector3d right = {heading.j, -heading.i, 0};
    short head = vr_find_node(graph, "", "head");
    if (!network_vr_pose_wanted()) return;
    memcpy(m, local, count * sizeof(*m));
    if (head >= 0 && head < count) {
        real_vector3d f = vr_render.head_camera.forward, u = vr_render.head_camera.up, l;
        real_vector3d original_left = {-heading.j, heading.i, 0};
        real rotation[3][3];
        cross_product3d(&u, &f, &l);
        if (vr_unit_vector(&f) && vr_unit_vector(&u) && vr_unit_vector(&l)) {
            for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++)
                rotation[row][col] = f.n[row] * heading.n[col] + l.n[row] * original_left.n[col] + u.n[row] * (col == 2);
            vr_turn_subtree(graph, m, head, rotation);
        }
    }
    for (int side = 0; side < 2; side++) {
        short chain[3];
        real_point3d hand; real_vector3d f, u, pole, old_axis, new_axis;
        real_matrix4x3 old_fore, old_hand;
        static char const *names[3] = {"upperarm", "forearm", "hand"};
        boolean valid = TRUE;
        for (int bone = 0; bone < 3; bone++) {
            chain[bone] = vr_find_node(graph, side ? " r " : " l ", names[bone]);
            if (chain[bone] < 0 || chain[bone] >= count) valid = FALSE;
        }
        if (!valid || !vr_node_under(graph, chain[1], chain[0]) || !vr_node_under(graph, chain[2], chain[1]) ||
            !vr_hand_pose(side, vr_render.game_camera_position.n, hand.n, f.n, u.n) || !vr_unit_vector(&f)) continue;
        hand.x -= f.i * 0.075f * units; hand.y -= f.j * 0.075f * units; hand.z -= f.k * 0.075f * units;
        /* Reuse last frame's displayed contact/grip target, when fresh and
         * nearby, so the remote support hand doesn't slide off the held gun. */
        vr_point_minus(&vr_avatar_hands[side].position, &hand, &old_axis);
        if (vr_avatar_hands[side].valid && vr_avatar_hands[side].unit == unit && vr_pose_time() - vr_avatar_hands[side].time >= 0 &&
            vr_pose_time() - vr_avatar_hands[side].time < 0.05 &&
            vr_length(&old_axis) < (vr_avatar_hands[side].animated > 0 ? 1.5f : 0.5f) * units)
            hand = vr_avatar_hands[side].position;
        pole = (real_vector3d){right.i * (side ? 0.6f : -0.6f) - heading.i * 0.3f,
            right.j * (side ? 0.6f : -0.6f) - heading.j * 0.3f, -1};
        if (vr_avatar_hands[side].valid && vr_avatar_hands[side].unit == unit &&
            vr_avatar_hands[side].animated > 0 && vr_pose_time() >= vr_avatar_hands[side].time &&
            vr_pose_time() - vr_avatar_hands[side].time < 0.05) {
            real_vector3d animated_pole;
            vr_point_minus(&vr_avatar_hands[side].elbow, &m[chain[0]].position, &animated_pole);
            if (vr_unit_vector(&animated_pole)) {
                real w = vr_avatar_hands[side].animated;
                for (int axis = 0; axis < 3; axis++) pole.n[axis] = pole.n[axis] * (1 - w) + animated_pole.n[axis] * w;
            }
        }
        old_fore = m[chain[1]]; old_hand = m[chain[2]];
        vr_solve_arm(graph, m, chain, vr_body_pose.shoulders_valid ? vr_body_pose.shoulders[side] : m[chain[0]].position,
            &hand, &pole, FALSE, &vr_arm_history[2 + side]);
        vr_point_minus(&m[chain[2]].position, &m[chain[1]].position, &new_axis);
        if (vr_unit_vector(&new_axis)) {
            real rotation[3][3], dot;
            real_vector3d from_up = m[chain[2]].up, from_side, to_side;
            dot = dot_product3d(&from_up, &new_axis);
            for (int axis = 0; axis < 3; axis++) from_up.n[axis] -= dot * new_axis.n[axis];
            dot = dot_product3d(&u, &f);
            for (int other = 0; other < 3; other++) u.n[other] -= dot * f.n[other];
            if (vr_unit_vector(&from_up) && vr_unit_vector(&u)) {
                cross_product3d(&new_axis, &from_up, &from_side);
                cross_product3d(&f, &u, &to_side);
                for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++)
                    rotation[row][col] = f.n[row] * new_axis.n[col] + u.n[row] * from_up.n[col] + to_side.n[row] * from_side.n[col];
                vr_turn_subtree(graph, m, chain[2], rotation);
            }
            vr_distribute_arm_twist(m, chain, &old_fore, &old_hand, &vr_arm_history[2 + side]);
        }
    }
    network_vr_pose_capture(unit, m, count);
}

boolean vr_render_hands_only(void)
{
    return vr_render.stereo && !vr_render.cinematic_view && !strcmp(config_string("vr.body"), "hands");
}

/* Hands Only (with any hand mode): the arms' bones (not the hands, fingers or
gun) shrink to nothing. Test19 shrank each bone onto its own origin, so the
glove's wrist vertices (shared with the forearm) were dragged toward the elbow
and the hand looked cut off. Test20c gathers the arm onto a point 3.5 cm back
from the wrist along the forearm: the cuff closes just behind the hand. */
/* the way from the knuckles back through the wrist (finger bases to wrist),
whatever the forearm bone is doing: a floating hand is moved without an arm
solve, so its forearm bone can be anywhere (test21) */
static boolean vr_hand_back_axis(real_matrix4x3 const *m, struct animation_graph *graph, short hand,
    char const *side, real_vector3d *back)
{
    short joints[5][3];
    real_point3d knuckles = {{0.0f, 0.0f, 0.0f}};
    int f, found = 0;

    vr_finger_nodes(graph, hand, side, joints);
    /* the four fingers' bases (the thumb's sits off the axis) */
    for (f = 1; f < 5; f++) {
        if (joints[f][0] == NONE) continue;
        for (int axis = 0; axis < 3; axis++) knuckles.n[axis] += m[joints[f][0]].position.n[axis];
        found++;
    }
    if (!found) return FALSE;
    for (int axis = 0; axis < 3; axis++) knuckles.n[axis] /= (real)found;
    vr_point_minus(&m[hand].position, &knuckles, back);
    return vr_unit_vector(back);
}

/* Hands Only, and floating hands (test21: no arms, as in test20c) */
static void vr_hide_forearms(real_matrix4x3 *m, struct animation_graph *graph, short left[3], short right[3])
{
    short gun = vr_find_node(graph, "frame", "gun");
    real units = vr_units_per_metre();
    if (!vr_render_hands_only() && vr_hand_tracking_mode() != 1) return;
    for (int side = 0; side < 2; side++) {
        short *chain = side ? right : left;
        real_point3d centre = m[chain[2]].position;
        real_vector3d back;
        boolean along = vr_hand_back_axis(m, graph, chain[2], side ? "r " : "l ", &back);
        if (!along) {
            vr_point_minus(&m[chain[1]].position, &m[chain[2]].position, &back);
            along = vr_unit_vector(&back);
        }
        if (along)
            for (int axis = 0; axis < 3; axis++) centre.n[axis] += back.n[axis] * 0.035f * units;
        for (short n = 0; n < graph->nodes.count; n++) {
            if (!vr_node_under(graph, n, chain[0]) || vr_node_under(graph, n, chain[2]) || vr_node_under(graph, n, gun))
                continue;
            m[n].scale = 0;
            m[n].position = centre;
        }
    }
}

/* Floating hands and arms: the shoulder is where the body would put it while
the hand is within reach; beyond (or too near) it slides along the line to the
hand, so the arm hangs from the hand and never pulls it off the controller. */
static void vr_float_shoulder(real_matrix4x3 const *m, short chain[3], real_point3d const *target, real_point3d *shoulder)
{
    real_vector3d upper, fore, reach;
    real a, b, distance, longest, shortest;
    vr_point_minus(&m[chain[1]].position, &m[chain[0]].position, &upper);
    vr_point_minus(&m[chain[2]].position, &m[chain[1]].position, &fore);
    a = vr_length(&upper); b = vr_length(&fore);
    vr_point_minus(target, shoulder, &reach);
    distance = vr_length(&reach);
    if (!isfinite(a) || !isfinite(b) || !isfinite(distance) || distance < 1e-4f) return;
    longest = (a + b) * 0.97f;
    shortest = (real)fabs(a - b) * 1.05f + 0.01f * vr_units_per_metre();
    if (longest <= shortest) return;
    if (distance <= longest && distance >= shortest) return;
    distance = PIN(distance, shortest, longest) / distance;
    shoulder->x = target->x - reach.i * distance;
    shoulder->y = target->y - reach.j * distance;
    shoulder->z = target->z - reach.k * distance;
}

/* the held gun anchored to its controller (vr_gun_anchor.h): the gun
hand's wrist goes where the empty hand's wrist would be (7.5 cm behind the
grip along the hand), moved by the player's gun position (vr.gun_*:
forward, up, outward, metres) and by any pullback out of a wall */
static void vr_anchor_gun(real_matrix4x3 *matrices, struct animation_graph *graph,
	short wrist, unsigned native_arms, real const gun_position[3])
{
	static struct vr_gun_anchor anchor = { .current = -1 };
	real units = vr_units_per_metre();
	int hand = vr_weapon_hand(), axis;
	real outward = hand == 1 ? -1.0f : 1.0f;
	real_point3d grip, target;
	real_vector3d f, u, forward, left, up;
	float axes[3][3], measured[3], move[3];
	short n;

	if (vr_hand_empty() || !vr_render.weapon_camera_valid || !(units > 0.0f) ||
		!vr_hand_pose(hand, vr_render.game_camera_position.n, grip.n, f.n, u.n) || !vr_unit_vector(&f))
	{
		vr_gun_anchor_reset(&anchor);
		return;
	}
	forward = vr_render.weapon_camera.forward;
	up = vr_render.weapon_camera.up;
	cross_product3d(&up, &forward, &left);
	if (!vr_unit_vector(&forward) || !vr_unit_vector(&left))
		return;
	cross_product3d(&forward, &left, &up);
	for (axis = 0; axis < 3; axis++)
	{
		target.n[axis] = grip.n[axis] - f.n[axis] * 0.075f * units + vr_render.weapon_pullback.n[axis] +
			(forward.n[axis] * gun_position[0] + up.n[axis] * gun_position[1] +
			left.n[axis] * outward * gun_position[2]) * units;
		measured[axis] = target.n[axis] - matrices[wrist].position.n[axis];
		axes[0][axis] = forward.n[axis];
		axes[1][axis] = left.n[axis];
		axes[2][axis] = up.n[axis];
	}
	if (!vr_gun_anchor_update(&anchor, graph, hand, axes, measured, (native_arms & 2) != 0,
		vr_pose_time(), 0.6f * units, move))
		return;
	for (n = 0; n < graph->nodes.count && n < MAXIMUM_NODES_PER_ANIMATION; n++)
	{
		matrices[n].position.x += move[0];
		matrices[n].position.y += move[1];
		matrices[n].position.z += move[2];
	}
}

real_matrix4x3 *vr_render_body_matrices(long object_index)
{
	static real_matrix4x3 matrices[MAXIMUM_NODES_PER_ANIMATION];
	static long logged_graph = NONE, cached_unit = NONE, cached_graph = NONE;
	static double cached_time = -1;
	struct object_datum *object = object_get(object_index);
	struct object_definition *definition = object_definition_get(object->definition_index);
	real_matrix4x3 *source = object_get_node_matrices(object_index);
	struct animation_graph *graph;
	short count, graph_count, roots[3], i, r;
	real_point3d collapse[3];
	real_vector3d forward = object->object.forward;
	if (!source || definition->object.animation_graph.index == NONE || vr_render.cinematic_view)
		return source;
	if (cached_unit == object_index && cached_graph == definition->object.animation_graph.index &&
		cached_time == vr_pose_time()) return matrices;
	vr_body_pose.shoulders_valid = FALSE;
	count = (short)(object->object.node_matrices.size / sizeof(*matrices));
	if (count <= 0 || count > MAXIMUM_NODES_PER_ANIMATION)
		return source;
	graph = animation_graph_definition_get(definition->object.animation_graph.index);
	graph_count = MIN(count, graph->nodes.count);
	/* Solvers traverse graph descendants, so a mismatched palette must stay stock. */
	if (graph->nodes.count != count) {
		if (logged_graph != definition->object.animation_graph.index) {
			logged_graph = definition->object.animation_graph.index;
			platform_log("vr: body IK unavailable: model %d nodes, graph %ld; using stock body pose", count, graph->nodes.count);
		}
		return source;
	}
	cached_unit = object_index; cached_time = vr_pose_time();
	cached_graph = definition->object.animation_graph.index;
	memcpy(matrices, source, count * sizeof(*matrices));
	/* Share a head-driven torso heading with the IK shoulders; the neck can
	turn within its comfort cone without rotating the entire avatar. */
	{
		real_vector3d heading;
		forward.k = 0.0f;
		heading = vr_body_heading(object_index);
		if (vr_unit_vector(&forward))
		{
			heading.k = 0.0f;
			if (vr_unit_vector(&heading))
			{
				real c = dot_product3d(&forward, &heading);
				real s = forward.i * heading.j - forward.j * heading.i;
				real rotation[3][3] = {{c, -s, 0}, {s, c, 0}, {0, 0, 1}};
				for (i = 0; i < count; i++)
				{
					real_vector3d offset;
					vr_point_minus(&source[i].position, &object->object.position, &offset);
					vr_rotate_vector(rotation, &offset, &offset);
					matrices[i].position.x = object->object.position.x + offset.i;
					matrices[i].position.y = object->object.position.y + offset.j;
					vr_rotate_vector(rotation, &source[i].forward, &matrices[i].forward);
					vr_rotate_vector(rotation, &source[i].left, &matrices[i].left);
					vr_rotate_vector(rotation, &source[i].up, &matrices[i].up);
				}
				forward = heading;
			}
		}
	}
	if (logged_graph != definition->object.animation_graph.index)
	{
		logged_graph = definition->object.animation_graph.index;
		for (i = 0; i < graph_count; i++)
		{
			struct vr_animation_graph_node const *node = TAG_BLOCK_GET_ELEMENT(&graph->nodes, i, struct vr_animation_graph_node);
			platform_log("vr: body node %d '%s' parent %d", i, node->name, node->parent_node_index);
		}
		if (count != graph->nodes.count)
			platform_log("vr: body node count mismatch: model %d graph %d; bounded to %d", count, graph->nodes.count, graph_count);
	}
    /* Real hierarchy confirmed by test10's Quest log. Discover names and
    ancestry at runtime so custom characters fall back without guessed indices. */
    {
        short spine = vr_find_node(graph, "", "spine");
        short neck = vr_find_node(graph, "", "neck");
        short legs[2][3], side, bone;
        real_point3d feet[2];
        boolean legs_valid[2] = {TRUE, TRUE};
        for (side = 0; side < 2; side++) {
            static char const *names[3] = {"thigh", "calf", "foot"};
            for (bone = 0; bone < 3; bone++) {
                legs[side][bone] = vr_find_node(graph, side ? " r " : " l ", names[bone]);
                if (legs[side][bone] < 0 || legs[side][bone] >= graph_count) legs_valid[side] = FALSE;
            }
            if (legs_valid[side]) {
                legs_valid[side] = vr_node_under(graph, legs[side][1], legs[side][0]) &&
                    vr_node_under(graph, legs[side][2], legs[side][1]);
                feet[side] = matrices[legs[side][2]].position;
            }
        }
        vr_body_plant_feet(object_index, object, matrices, legs, legs_valid, feet);
        if (spine >= 0 && spine < graph_count && neck >= 0 && neck < graph_count &&
            vr_node_under(graph, neck, spine)) {
            real units = vr_units_per_metre();
            real_point3d target;
            real_vector3d delta, from, to;
            real rotation[3][3], horizontal;
            /* test21: the neck is where the head turns about, 14 cm behind
            and 20 cm below the eyes in the head's own frame (pitch limited to
            -60..+30 degrees). Looking down swings the eyes forward and down
            about it, so the torso stays behind and below them instead of
            following them into the camera (the old fixed offset took only
            the torso's yaw, and room-scale moves the character with the
            eyes) */
            vr_neck_pivot(&vr_render.head_camera.position, &vr_render.head_camera.forward,
                &vr_render.head_camera.up, units, &target);
            vr_point_minus(&target, &matrices[neck].position, &delta);
            horizontal = sqrtf(delta.i * delta.i + delta.j * delta.j);
            if (horizontal > 0.45f * units) {
                delta.i *= 0.45f * units / horizontal;
                delta.j *= 0.45f * units / horizontal;
            }
            /* Give the tracked head priority when crouching deeply. The old
             * 40 cm downward cap left the chest above the eyes when kneeling. */
            delta.k = PIN(delta.k, -1.20f * units, 0.25f * units);
            /* Pelvis follows lean partially; spine solves the rest. Crouch
            lowers the pelvis while the grounded leg targets remain in place. */
            for (i = 0; i < count; i++) {
                matrices[i].position.x += delta.i * 0.65f;
                matrices[i].position.y += delta.j * 0.65f;
                matrices[i].position.z += delta.k;
            }
            vr_point_minus(&matrices[neck].position, &matrices[spine].position, &from);
            to = from;
            to.i += delta.i * 0.35f; to.j += delta.j * 0.35f;
            if (vr_unit_vector(&from) && vr_unit_vector(&to)) {
                vr_rotation_between(&from, &to, rotation);
                vr_turn_subtree(graph, matrices, spine, rotation);
            }
            for (side = 0; side < 2; side++) if (legs_valid[side]) {
                real_point3d start = feet[side];
                real_vector3d down = {0, 0, -0.45f * units};
                struct collision_result ground;
                real_vector3d pole = forward;
                start.z += 0.20f * units;
                /* Only nearby floor contact: do not pull airborne legs down
                to a distant surface or alter game collision/physics. */
                if (collision_test_vector(FLAG(_collision_test_structure_bit) |
                    FLAG(_collision_test_objects_bit) | FLAG(_collision_test_objects_scenery_bit) |
                    FLAG(_collision_test_objects_machines_bit), &start, &down, object_index, &ground) &&
                    ground.plane.n.k > 0.5f && fabsf(object->object.translational_velocity.k) < 0.02f * units) {
                    feet[side].z = MAX(feet[side].z, ground.point.z + 0.04f * units);
                }
                pole.i += forward.j * (side ? 0.15f : -0.15f);
                pole.j -= forward.i * (side ? 0.15f : -0.15f);
                vr_body_leg(graph, matrices, legs[side], feet[side], pole);
            }
        }
    }
    {
        short l = vr_find_node(graph, " l ", "upperarm"), r = vr_find_node(graph, " r ", "upperarm");
        if (l >= 0 && r >= 0 && l < graph_count && r < graph_count) {
            vr_body_pose.shoulders[0] = matrices[l].position;
            vr_body_pose.shoulders[1] = matrices[r].position;
            /* Weapon aim animations can shrug/twist the clavicles. Anchor
             * shoulders to the solved neck/collar line, using the model's
             * measured width, so aiming cannot pull a shoulder into the eye. */
            short neck = vr_find_node(graph, "", "neck");
            if (neck >= 0 && neck < graph_count) {
                real_vector3d span, right = {forward.j, -forward.i, 0};
                real units = vr_units_per_metre(), width;
                vr_point_minus(&matrices[r].position, &matrices[l].position, &span);
                width = PIN(vr_length(&span) * 0.5f, 0.14f * units, 0.24f * units);
                for (short side = 0; side < 2; side++) {
                    real sign = side ? 1.0f : -1.0f;
                    vr_body_pose.shoulders[side] = matrices[neck].position;
                    vr_body_pose.shoulders[side].x += right.i * width * sign - forward.i * 0.02f * units;
                    vr_body_pose.shoulders[side].y += right.j * width * sign - forward.j * 0.02f * units;
                    vr_body_pose.shoulders[side].z -= 0.03f * units;
                }
            }
            vr_body_pose.shoulders_valid = TRUE;
        }
    }
    vr_publish_body(object_index, graph, matrices, count, forward);
    roots[0] = vr_find_node(graph, "", "head");
	roots[1] = vr_find_node(graph, " l ", "clavicle");
	roots[2] = vr_find_node(graph, " r ", "clavicle");
	for (r = 0; r < 3; r++)
		if (roots[r] >= 0 && roots[r] < graph_count)
			collapse[r] = matrices[roots[r]].position;
	for (i = 0; i < graph_count; i++)
		for (r = 0; r < 3; r++)
			if (roots[r] >= 0 && roots[r] < graph_count && vr_node_under(graph, i, roots[r]))
			{
				matrices[i].scale = 0.0f;
				matrices[i].position = collapse[r];
				break;
			}
    if (!strcmp(config_string("vr.body"), "legs")) {
        short spine = vr_find_node(graph, "", "spine");
        for (i = 0; i < graph_count; i++)
            if (vr_node_under(graph, i, spine)) matrices[i].scale = 0;
    }
	return matrices;
}

static boolean vr_action_matrix_valid(real_matrix4x3 const *m)
{
    if (!isfinite(m->scale)) return FALSE;
    for (int axis = 0; axis < 3; axis++)
        if (!isfinite(m->position.n[axis]) || !isfinite(m->forward.n[axis]) ||
            !isfinite(m->left.n[axis]) || !isfinite(m->up.n[axis])) return FALSE;
    return TRUE;
}

/* Blend rotations without shearing the skin. Both palettes already have the
 * same left-hand reflection; factor it out before quaternion interpolation. */
static void vr_blend_action_matrix(real_matrix4x3 *tracked, real_matrix4x3 const *native, real w)
{
    real_matrix4x3 a = *tracked, b = *native;
    real_vector3d cross;
    real_quaternion qa, qb, q;
    boolean reflected;
    if (!isfinite(w) || w <= 0 || !vr_action_matrix_valid(native)) return;
    if (!vr_action_matrix_valid(tracked)) { *tracked = *native; return; }
    if (w >= 1) { *tracked = *native; return; }
    cross_product3d(&a.forward, &a.left, &cross);
    reflected = dot_product3d(&cross, &a.up) < 0;
    if (reflected) { scale_vector3d(&a.left, -1, &a.left); scale_vector3d(&b.left, -1, &b.left); }
    matrix4x3_rotation_to_quaternion(&a, &qa);
    matrix4x3_rotation_to_quaternion(&b, &qb);
    quaternions_interpolate_and_normalize(&qa, &qb, w, &q);
    matrix4x3_rotation_from_quaternion(tracked, &q);
    if (reflected) scale_vector3d(&tracked->left, -1, &tracked->left);
    for (int axis = 0; axis < 3; axis++)
        tracked->position.n[axis] = a.position.n[axis] + (b.position.n[axis] - a.position.n[axis]) * w;
    tracked->scale = a.scale + (b.scale - a.scale) * w;
}

/* Keep the gun and its attachments byte-for-byte unchanged by arm blending. */
static void vr_blend_action_arm(struct animation_graph *graph, real_matrix4x3 *matrices,
    real_matrix4x3 const *authored, short root, short gun, real weight)
{
    if (!graph || graph->nodes.count <= 0 || graph->nodes.count > MAXIMUM_NODES_PER_ANIMATION ||
        root < 0 || root >= graph->nodes.count) return;
    for (short n = 0; weight > 0 && n < graph->nodes.count; n++)
        if (vr_node_under(graph, n, root) && !vr_node_under(graph, n, gun))
            vr_blend_action_matrix(&matrices[n], &authored[n], weight);
}

void vr_render_first_person_ik(
	real_matrix4x3 *matrices,
	struct animation_graph *graph, long unit, long weapon, unsigned native_arms)
{
	static char const *const arms_setting_names[] = { "ik", "hidden", "animated" };
	static int arms = -1, generation, gun_anchor;
	static real gun_position[3];
	static struct animation_graph *logged;
	short left[NUMBER_OF_VR_ARM_BONES], right[NUMBER_OF_VR_ARM_BONES];
	int side, bone, draw_arms;
    real_matrix4x3 authored[MAXIMUM_NODES_PER_ANIMATION];
    static struct vr_action_blend action;
    static struct animation_graph *action_graph;
    static long action_unit = NONE, action_weapon = NONE;
    static int action_hand = -1, action_generation = -1;
    boolean action_reset;

	/* (again when the pause menu changes it) */
	if (arms < 0 || generation != vr_settings_generation())
	{
		char const *setting;

		generation = vr_settings_generation();
		setting = config_string("vr.arms");
		for (arms = 0; arms < 3 && strcmp(setting, arms_setting_names[arms]); arms++)
			;
		if (arms == 3)
			arms = 0;
		gun_anchor = config_boolean("vr.gun_anchor");
		gun_position[0] = PIN((real)config_real("vr.gun_forward"), -0.2f, 0.2f);
		gun_position[1] = PIN((real)config_real("vr.gun_up"), -0.2f, 0.2f);
		gun_position[2] = PIN((real)config_real("vr.gun_out"), -0.2f, 0.2f);
		for (side = 0; side < 3; side++)
			if (!isfinite(gun_position[side]))
				gun_position[side] = 0.0f;
		platform_log("vr: gun %s, position forward/up/out %.3f/%.3f/%.3f m",
			gun_anchor ? "anchored to the controller" : "classic (camera-placed)",
			gun_position[0], gun_position[1], gun_position[2]);
	}
	if (!vr_render.stereo || vr_render.cinematic_view || !vr_hand_aiming() || !graph ||
		graph->nodes.count <= 0 || graph->nodes.count > MAXIMUM_NODES_PER_ANIMATION)
		return;
	for (side = 0; side < 2; side++)
	{
		static char const *const bone_names[] = { "upperarm", "forearm", "wrist" };
		short *chain = side ? right : left;

		/* Halo's graphs name the hands' bones "wriste" (or "hand") */
		for (bone = 0; bone < NUMBER_OF_VR_ARM_BONES; bone++)
			chain[bone] = vr_find_node(graph, side ? "r " : "l ", bone_names[bone]);
		if (chain[_vr_arm_hand] == NONE)
			chain[_vr_arm_hand] = vr_find_node(graph, side ? "r " : "l ", "hand");
	}
	if (logged != graph)
	{
		short index;

		logged = graph;
		for (index = 0; index < graph->nodes.count; index++)
		{
			struct vr_animation_graph_node const *node =
				TAG_BLOCK_GET_ELEMENT(&graph->nodes, index, struct vr_animation_graph_node);

			platform_log("vr: first-person node %d '%s' parent %d", index, node->name, node->parent_node_index);
		}
		platform_log("vr: arms: left %d %d %d, right %d %d %d", left[0], left[1], left[2], right[0], right[1], right[2]);
	}
	for (side = 0; side < 2; side++)
	{
		short *chain = side ? right : left;

		for (bone = 0; bone < NUMBER_OF_VR_ARM_BONES; bone++)
		{
			if (chain[bone] == NONE)
				return;
		}
	}

	/* in the left hand: the whole model mirrored across the weapon
	camera's upright plane (its left axis the plane's normal), the grip
	then landing in the hand (vr_weapon_view takes the offset the other
	way); its triangles' winding turns over (halo_vr_mirror_winding) */
	if (vr_render_first_person_mirrored() && vr_render.weapon_camera_valid)
	{
		real_vector3d normal;
		real_point3d centre = vr_render.weapon_camera.position;
		short index;

		cross_product3d(&vr_render.weapon_camera.up, &vr_render.weapon_camera.forward, &normal);
		normalize3d(&normal);
		for (index = 0; index < graph->nodes.count && index < MAXIMUM_NODES_PER_ANIMATION; index++)
		{
			real_matrix4x3 *m = &matrices[index];
			real_vector3d *axes[3] = { &m->forward, &m->left, &m->up };
			real_vector3d offset;
			real along;
			int axis;

			vr_point_minus(&m->position, &centre, &offset);
			along = 2.0f * (offset.i * normal.i + offset.j * normal.j + offset.k * normal.k);
			m->position.x -= normal.i * along;
			m->position.y -= normal.j * along;
			m->position.z -= normal.k * along;
			for (axis = 0; axis < 3; axis++)
			{
				along = 2.0f * (axes[axis]->i * normal.i + axes[axis]->j * normal.j + axes[axis]->k * normal.k);
				axes[axis]->i -= normal.i * along;
				axes[axis]->j -= normal.j * along;
				axes[axis]->k -= normal.k * along;
			}
		}
		/* test22: the ammo display ("frame display", the assault rifle's)
		mirrored back about its own centre: it stays where the mirrored gun
		has it but reads as the right hand's does, not backwards (its
		triangles keep their winding: rasterizer_xbox.c) */
		{
			short display = vr_find_node(graph, "frame", "display");

			if (display >= 0 && display < graph->nodes.count && display < MAXIMUM_NODES_PER_ANIMATION)
			{
				real_point3d display_centre = matrices[display].position;

				for (index = 0; index < graph->nodes.count && index < MAXIMUM_NODES_PER_ANIMATION; index++)
				{
					real_matrix4x3 *m = &matrices[index];
					real_vector3d *axes[3] = { &m->forward, &m->left, &m->up };
					real_vector3d offset;
					real along;
					int axis;

					if (index != display && !vr_node_under(graph, index, display))
						continue;
					vr_point_minus(&m->position, &display_centre, &offset);
					along = 2.0f * (offset.i * normal.i + offset.j * normal.j + offset.k * normal.k);
					m->position.x -= normal.i * along;
					m->position.y -= normal.j * along;
					m->position.z -= normal.k * along;
					for (axis = 0; axis < 3; axis++)
					{
						along = 2.0f * (axes[axis]->i * normal.i + axes[axis]->j * normal.j + axes[axis]->k * normal.k);
						axes[axis]->i -= normal.i * along;
						axes[axis]->j -= normal.j * along;
						axes[axis]->k -= normal.k * along;
					}
				}
			}
		}
	}

    /* the gun in the hand before anything reaches for it */
    if (gun_anchor)
        vr_anchor_gun(matrices, graph, right[_vr_arm_hand], native_arms, gun_position);
    draw_arms = vr_render_hands_only() ? 0 : arms;
    if (draw_arms == 2) {
        vr_avatar_hands[0].valid = vr_avatar_hands[1].valid = FALSE;
        action.valid = 0;
        vr_hide_forearms(matrices, graph, left, right); return;
    }
    action_reset = action_graph != graph || action_unit != unit || action_weapon != weapon ||
        action_hand != vr_weapon_hand() || action_generation != vr_settings_generation() ||
        !action.valid || vr_pose_time() - action.time > 0.25 || vr_pose_time() < action.time;
    if (action_reset) vr_avatar_hands[0].valid = vr_avatar_hands[1].valid = FALSE;
    if (vr_hand_empty()) native_arms = 0;
    vr_action_blend_update(&action, vr_pose_time(), native_arms, action_reset);
    action_graph = graph; action_unit = unit; action_weapon = weapon;
    action_hand = vr_weapon_hand(); action_generation = vr_settings_generation();
    memcpy(authored, matrices, graph->nodes.count * sizeof(*matrices));

	/* Capture the authored support grip in weapon-local coordinates once,
	not once per animation frame. Holding grip then stays fixed to the gun. */
	{
		static real_matrix4x3 support_local[MAXIMUM_NODES_PER_ANIMATION];
		static struct animation_graph *support_graph;
		static boolean support_locked;
		short gun = vr_find_node(graph, "frame", "gun");
		real_point3d controller;
		real_vector3d f, u, gap;
		if (action_reset || !vr_two_handed()) support_locked = FALSE;
		vr_support_near(FALSE);
		vr_touch_weapon.valid = FALSE;
		if (gun >= 0 && gun < graph->nodes.count && !vr_hand_empty()) {
			real_vector3d along;
			real units = vr_units_per_metre();
			vr_touch_weapon.a = matrices[right[_vr_arm_hand]].position;
			vr_touch_weapon.b = matrices[left[_vr_arm_hand]].position;
			vr_point_minus(&vr_touch_weapon.b, &vr_touch_weapon.a, &along);
			if (vr_unit_vector(&along)) {
				for (int axis = 0; axis < 3; axis++) {
					vr_touch_weapon.a.n[axis] += matrices[gun].up.n[axis] * 0.025f * units;
					vr_touch_weapon.b.n[axis] += (matrices[gun].up.n[axis] * 0.025f + along.n[axis] * 0.12f) * units;
				}
				vr_touch_weapon.radius = 0.035f * units;
				vr_touch_weapon.time = vr_pose_time();
				vr_touch_weapon.valid = TRUE;
			}
			if (vr_hand_world(1 - vr_weapon_hand(), vr_render.game_camera_position.n, controller.n, f.n, u.n)) {
				vr_point_minus(&controller, &matrices[left[_vr_arm_hand]].position, &gap);
				vr_support_near(vr_length(&gap) < 0.16f * vr_units_per_metre());
			}
			if (vr_two_handed()) {
                /* Never acquire a new grip from a transient reload/throw pose. */
				if ((!support_locked || support_graph != graph) && !native_arms && action.weight[0] == 0) {
					real_matrix4x3 inverse;
					matrix4x3_inverse(&matrices[gun], &inverse);
					for (short n = 0; n < graph->nodes.count; n++)
						if (vr_node_under(graph, n, left[_vr_arm_hand]))
							matrix4x3_multiply(&inverse, &matrices[n], &support_local[n]);
					support_graph = graph;
                    support_locked = TRUE;
				}
				for (short n = 0; support_locked && n < graph->nodes.count; n++)
					if (vr_node_under(graph, n, left[_vr_arm_hand]))
						matrix4x3_multiply(&matrices[gun], &support_local[n], &matrices[n]);
			}
		} else support_locked = FALSE;
	}

	/* First-person arms remain the hands in full-body mode. */
	if (draw_arms == 1)
	{
		short gun = vr_find_node(graph, "frame", "gun");

		/* hidden: the arms' bones (and what they carry) shrunk to nothing,
		at the shoulder's place */
		short index;

		for (index = 0; index < graph->nodes.count && index < MAXIMUM_NODES_PER_ANIMATION; index++)
		{
			struct vr_animation_graph_node const *node =
				TAG_BLOCK_GET_ELEMENT(&graph->nodes, index, struct vr_animation_graph_node);
			short root = vr_node_under(graph, index, left[_vr_arm_upper]) ? left[_vr_arm_upper] :
				vr_node_under(graph, index, right[_vr_arm_upper]) ? right[_vr_arm_upper] : NONE;
			(void)node;
			if (root != NONE && !vr_node_under(graph, index, gun))
			{
				matrices[index].scale = 0.0f;
				matrices[index].position = matrices[root].position;
			}
		}
		return;
	}

	{
		real units = vr_units_per_metre();
		real_point3d head = vr_render.head_camera.position, shoulder, target;
		real_vector3d forward, right_side, up = { 0.0f, 0.0f, 1.0f }, pole;

		/* test21: the torso's heading (vr_body_heading) in every body mode.
		Without a drawn body the shoulders used to face the turn heading
		(vr.heading: stick turns only), so turning the real body left them
		behind and the arms twisted across */
		if (vr_body_pose.valid && vr_body_pose.time == vr_pose_time() && vr_body_pose.unit == unit)
			forward = (real_vector3d){cosf(vr_body_pose.yaw), sinf(vr_body_pose.yaw), 0};
		else
			forward = vr_body_heading(unit);
		forward.k = 0.0f;
		normalize3d(&forward);
		right_side.i = forward.j;
		right_side.j = -forward.i;
		right_side.k = 0.0f;
		/* the model's right arm holds the gun; in the left hand the model is
		mirrored, so that arm is the left one and the other reaches the right
		controller */
		int weapon_hand = vr_weapon_hand();
		/* 0 body IK; 1 floating hands (no arms); 2 floating hands with arms
		hung from a floating shoulder (with Body: Hands Only, as 1) */
		int tracking = vr_hand_tracking_mode();
		if (tracking == 2 && vr_render_hands_only())
			tracking = 1;

		for (side = 0; side < 2; side++)
		{
			short *chain = side ? right : left;
			boolean gun_arm = side == 1;
			real outward = (gun_arm == (weapon_hand == 1)) ? 1.0f : -1.0f;
			int controller = gun_arm ? weapon_hand : 1 - weapon_hand;

            /* At full ownership use the untouched native chain, including fingers.
             * No contact solver/haptics should fight an animated hand. */
            if (action.weight[side] >= 1) continue;
			/* shoulders below and either side of the eyes, a little back */
			shoulder.x = head.x + (right_side.i * 0.17f * outward - forward.i * 0.06f) * units;
			shoulder.y = head.y + (right_side.j * 0.17f * outward - forward.j * 0.06f) * units;
			shoulder.z = head.z - 0.22f * units;
            /* only body IK hangs the arms from the body's shoulders */
            if (tracking == 0 && vr_body_setting()) {
                short local = local_player_get_next(NONE);
                long player = local != NONE ? local_player_get_player_index(local) : NONE;
                long unit = player != NONE ? player_get(player)->unit_index : NONE;
                if (unit != NONE && vr_render_full_body(unit)) {
                    vr_render_body_matrices(unit);
                    if (vr_body_pose.shoulders_valid) shoulder = vr_body_pose.shoulders[controller];
                }
            }
			/* elbows down, out and back */
			pole.i = right_side.i * 0.6f * outward - forward.i * 0.3f;
			pole.j = right_side.j * 0.6f * outward - forward.j * 0.3f;
			pole.k = -1.0f;
			target = matrices[chain[_vr_arm_hand]].position;
			{
				real_point3d hand;
				real_vector3d f, u, palm, normal;
				real depth = 0.0f, dot;
				boolean free_hand = FALSE, pressed = FALSE;
				short joints[5][3];
				float curls[4];
				real_matrix4x3 old_fore = matrices[chain[_vr_arm_fore]];
				real_matrix4x3 old_hand = matrices[chain[_vr_arm_hand]];
				if ((!gun_arm || vr_hand_empty()) &&
					vr_hand_pose(controller, vr_render.game_camera_position.n, hand.n, f.n, u.n) && vr_unit_vector(&f))
				{
					dot = dot_product3d(&f, &u);
					u.i -= dot * f.i; u.j -= dot * f.j; u.k -= dot * f.k;
					/* Proximity alone never captures the support hand. */
					free_hand = vr_unit_vector(&u) && (gun_arm || vr_hand_empty() || !vr_two_handed());
					if (free_hand)
					{
						target.x = hand.x - f.i * 0.075f * units;
						target.y = hand.y - f.j * 0.075f * units;
						target.z = hand.z - f.k * 0.075f * units;
						pressed = vr_hand_out_of_walls(controller, &target, &f, &depth, &normal);
						cross_product3d(&f, &u, &palm);
						if (controller == 1) scale_vector3d(&palm, -1.0f, &palm);
						if (pressed && vr_unit_vector(&normal) && dot_product3d(&palm, &normal) < -0.2f)
						{
							real amount = 0.85f * MIN(depth / 0.05f, 1.0f), rotation[3][3];
							real_vector3d wanted;
							wanted.i = palm.i * (1.0f - amount) - normal.i * amount;
							wanted.j = palm.j * (1.0f - amount) - normal.j * amount;
							wanted.k = palm.k * (1.0f - amount) - normal.k * amount;
							if (vr_unit_vector(&wanted))
							{
								vr_rotation_between(&palm, &wanted, rotation);
								vr_rotate_vector(rotation, &f, &f); vr_rotate_vector(rotation, &u, &u);
								palm = wanted;
							}
						}
					}
				}
				if (tracking == 1)
				{
					/* floating hands: the free hand goes exactly to its
					controller; a held gun keeps its hand; no arm solve */
					if (free_hand)
					{
						real_vector3d delta;
						vr_point_minus(&target, &matrices[chain[_vr_arm_hand]].position, &delta);
						for (short n = 0; n < graph->nodes.count; n++)
							if (vr_node_under(graph, n, chain[_vr_arm_hand]))
							{
								matrices[n].position.x += delta.i;
								matrices[n].position.y += delta.j;
								matrices[n].position.z += delta.k;
							}
					}
				}
				else
				{
					if (tracking == 2)
						vr_float_shoulder(matrices, chain, &target, &shoulder);
					vr_solve_arm(graph, matrices, chain, shoulder, &target, &pole, !free_hand, &vr_arm_history[controller]);
				}
				if (free_hand)
				{
					int contacts = 0;
					vr_finger_nodes(graph, chain[_vr_arm_hand], side ? "r " : "l ", joints);
					vr_orient_hand(graph, matrices, chain[_vr_arm_hand], joints, &f, &u);
					if (tracking != 1)
						vr_distribute_arm_twist(matrices, chain, &old_fore, &old_hand, &vr_arm_history[controller]);
					if (vr_finger_pose(controller, curls))
						contacts = vr_touch_fingers(graph, matrices, controller, chain[_vr_arm_hand], joints, curls, &palm, &f, &u, pressed);
					if (pressed && action.weight[side] == 0) vr_pressure_buzz(controller, depth);
					else if (contacts && action.weight[side] == 0) vr_haptic(controller, 0.05f + 0.05f * contacts, 0.03f);
				}
				else if (tracking != 1) vr_distribute_arm_twist(matrices, chain, &old_fore, &old_hand, &vr_arm_history[controller]);

			}
		}
	}
    for (side = 0; side < 2; side++) {
        short *chain = side ? right : left;
        int controller = side ? vr_weapon_hand() : 1 - vr_weapon_hand();
        real weight = action.weight[side];
        short gun = vr_find_node(graph, "frame", "gun");
        vr_blend_action_arm(graph, matrices, authored, chain[0], gun, weight);
        vr_avatar_hands[controller].position = matrices[chain[2]].position;
        vr_avatar_hands[controller].elbow = matrices[chain[1]].position;
        vr_avatar_hands[controller].animated = weight;
        vr_avatar_hands[controller].unit = unit;
        vr_avatar_hands[controller].time = vr_pose_time();
        vr_avatar_hands[controller].valid = TRUE;
    }
    vr_hide_forearms(matrices, graph, left, right);
}

#endif /* HALO_VR */
