/* Test-build-only fixture. The visual runner installs and removes this file.
			Creates real plasma projectiles; never linked into the normal game. */
#include "cseries.h"
#include <xtl.h>
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "objects/damage.h"
#include "physics/collisions.h"
#include "items/projectiles.h"
#include "items/projectile_definitions.h"
#include "items/weapons.h"
#include "camera/observer.h"
#include "tag_files/tag_groups.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_lights.h"
#include "plasma_lighting.h"
#include "game/players.h"
#include "game/game.h"
#include "units/units.h"
#include "units/bipeds.h"
#include "units/vehicles.h"
#include "devices/devices.h"
#include "cache/cache_files.h"
#include "cutscene/cinematics.h"
#include "game/cheats.h"
#include "ai/ai.h"
#include "scenario/scenario.h"
#include <stdlib.h>
#include <string.h>

void platform_log(const char *format, ...);
int config_boolean(const char *name);
long light_port_glow_definition_get(void);
real render_interpolation_fraction(void);
static long capture_ticks, capture_projectile = NONE, capture_last_time;
static struct observer_result capture_camera;
static int capture_model, capture_probe, capture_burst = 1;
static long capture_bolts[32], capture_door = NONE;
static int capture_combat, capture_toggle;
static long capture_target = NONE;
static long capture_emitter = NONE;
static long capture_emitter_holder = NONE;
static long capture_vehicle = NONE;
static int capture_emitter_attached;
int plasma_capture_skip_cinematic(void)
{
	return cinematic_globals && cinematic_in_progress();
}

int plasma_capture_turret_fire(void)
{
	return capture_ticks >= 75 && capture_ticks % 30 < 23;
}

int plasma_capture_scorpion_fire(void)
{
	if (capture_ticks < 75 || capture_ticks > 240) return 0;
	if (capture_ticks < 120) return 2;
	if (capture_ticks < 170) return 1;
	return 3;
}

/* Render-only exterior view: the seated player's ordinary controls still aim
   and fire the turret. Keep the chassis and adjacent ground in the picture. */
void plasma_capture_camera(void)
{
	struct observer_result *camera;
	struct object_datum *vehicle;
	struct object_marker marker;
	real_point3d target;
	real_vector3d right;
	int scorpion = getenv("HALO_SCORPION_CAPTURE") != NULL;
	if (getenv("HALO_EMITTER_CAPTURE_COUNTER_DETAIL") && capture_emitter != NONE &&
		object_try_and_get(capture_emitter) &&
		object_get_marker_by_name(capture_emitter, "primary trigger", &marker, 1))
	{
		/* Look over the holder's shoulder at the world-model display. This
		   changes only the capture camera, never the material or light. */
		camera = (struct observer_result *)observer_get_camera(0);
		target = marker.matrix.position;
		target.x -= 0.18f * marker.matrix.forward.i;
		target.y -= 0.18f * marker.matrix.forward.j;
		target.z -= 0.18f * marker.matrix.forward.k;
		cross_product3d(&marker.matrix.forward, &marker.matrix.up, &right);
		camera->position.x = target.x - 0.55f * marker.matrix.forward.i + 0.35f * right.i + 0.35f * marker.matrix.up.i;
		camera->position.y = target.y - 0.55f * marker.matrix.forward.j + 0.35f * right.j + 0.35f * marker.matrix.up.j;
		camera->position.z = target.z - 0.55f * marker.matrix.forward.k + 0.35f * right.k + 0.35f * marker.matrix.up.k;
		vector_from_points3d(&camera->position, &target, &camera->forward);
		normalize3d(&camera->forward);
		observer_up_from_forward(&camera->forward, &camera->up);
		scenario_location_from_point(&camera->location, &camera->position);
		return;
	}
	if ((!getenv("HALO_WARTHOG_CAPTURE_EXTERIOR") && !scorpion) || capture_vehicle == NONE ||
		!(vehicle = object_try_and_get(capture_vehicle)))
		return;
	camera = (struct observer_result *)observer_get_camera(0);
	target = vehicle->object.position;
	target.z += 0.6f;
	cross_product3d(&capture_camera.forward, &capture_camera.up, &right);
	camera->position = vehicle->object.position;
	camera->position.x += (scorpion ? 6.0f : 3.0f) * right.i - (scorpion ? 2.5f : 1.5f) * capture_camera.forward.i;
	camera->position.y += (scorpion ? 6.0f : 3.0f) * right.j - (scorpion ? 2.5f : 1.5f) * capture_camera.forward.j;
	camera->position.z += scorpion ? 2.4f : 1.1f;
	vector_from_points3d(&camera->position, &target, &camera->forward);
	normalize3d(&camera->forward);
	observer_up_from_forward(&camera->forward, &camera->up);
	scenario_location_from_point(&camera->location, &camera->position);
}

/* Apply after ordinary camera interpolation, so the capture sees the same
   vehicle pose as the renderer instead of having its observer edit replaced. */
struct observer_result const *plasma_capture_view(struct observer_result const *source)
{
	static struct observer_result view;
	struct object_datum *vehicle;
	real_point3d target;
	real_vector3d right;
	real distance;
	const char *kind = getenv("HALO_AMBIENT_CAPTURE");
	if (!kind || capture_vehicle == NONE || !(vehicle = object_try_and_get(capture_vehicle))) return source;
	view = *source;
	target = vehicle->object.position;
	target.z += !strcmp(kind, "dropship") ? -0.5f : 0.4f;
	distance = !strcmp(kind,"dropship") ? 9.0f : !strcmp(kind,"scorpion") ? 5.0f : 3.0f;
	cross_product3d(&vehicle->object.forward, &vehicle->object.up, &right);
	view.position.x = target.x + distance * vehicle->object.forward.i + distance * .6f * right.i;
	view.position.y = target.y + distance * vehicle->object.forward.j + distance * .6f * right.j;
	view.position.z = target.z + (!strcmp(kind,"dropship") ? 0.0f : 1.0f);
	vector_from_points3d(&view.position, &target, &view.forward);
	normalize3d(&view.forward);
	observer_up_from_forward(&view.forward, &view.up);
	scenario_location_from_point(&view.location, &view.position);
	return &view;
}

static void capture_ground(real_point3d *position)
{
	struct collision_result ground;
	real_vector3d down = {0, 0, -8};
	real_point3d from = *position;
	from.z = capture_camera.position.z + 1.0f;
	if (collision_test_vector(FLAG(_collision_test_structure_bit) |
		FLAG(_collision_test_front_facing_surfaces_bit), &from, &down, NONE, &ground))
		position->z = ground.point.z + 0.03f;
}

/* Create ordinary objects; exercise their stock functions and deletion. */
static void ambient_capture_tick(const char *kind)
{
	static const struct { const char *kind, *name, *alternate; long group; } profiles[] = {
		{"ghost", "vehicles\\ghost\\ghost_mp", "vehicles\\ghost\\ghost", 'vehi'},
		{"banshee", "vehicles\\banshee\\banshee_mp", "vehicles\\banshee\\banshee", 'vehi'},
		{"dropship", "vehicles\\c_dropship\\c_dropship", NULL, 'vehi'},
		{"warthog", "vehicles\\warthog\\mp_warthog", "vehicles\\warthog\\warthog", 'vehi'},
		{"scorpion", "vehicles\\scorpion\\scorpion_mp", "vehicles\\scorpion\\scorpion", 'vehi'},
		{"overshield", "powerups\\over shield", NULL, 'eqip'},
		{"camo", "powerups\\active camouflage", NULL, 'eqip'},
		{"teleporter", "scenery\\teleporter_shield\\teleporter", NULL, 'scen'},
	};
	int i;
	for (i = 0; i < NUMBEROF(profiles); i++)
		if (!strcmp(kind, profiles[i].kind)) break;
	if (i == NUMBEROF(profiles)) return;
	if (capture_ticks == 65)
	{
		struct object_placement_data placement;
		long tag = tag_loaded(profiles[i].group, profiles[i].name);
		if (tag == NONE && profiles[i].alternate) tag = tag_loaded(profiles[i].group, profiles[i].alternate);
		if (tag == NONE) { platform_log("AMBIENT missing %s", kind); return; }
		object_placement_data_new(&placement, tag, NONE);
		placement.position = capture_camera.position;
		placement.position.x += capture_camera.forward.i * (!strcmp(kind, "dropship") ? 9 : 3);
		placement.position.y += capture_camera.forward.j * (!strcmp(kind, "dropship") ? 9 : 3);
		capture_ground(&placement.position);
		if (!strcmp(kind, "dropship")) placement.position.z += 3.0f;
		placement.forward = capture_camera.forward;
		placement.forward.i = -placement.forward.i;
		placement.forward.j = -placement.forward.j;
		placement.forward.k = 0;
		normalize3d(&placement.forward);
		observer_up_from_forward(&placement.forward, &placement.up);
		capture_emitter = object_new(&placement);
		platform_log("AMBIENT created %ld %s", capture_emitter, kind);
		if (capture_emitter != NONE && profiles[i].group == 'vehi')
		{
			capture_vehicle = capture_emitter;
			vehicle_hover(capture_vehicle, TRUE);
			/* Use a real driver so engine functions and headlights remain on. */
			if (strcmp(kind, "dropship"))
			{
				long driver_tag = tag_loaded('bipd', "characters\\cyborg\\cyborg");
				if (driver_tag != NONE)
				{
					object_placement_data_new(&placement, driver_tag, NONE);
					placement.position = object_get(capture_vehicle)->object.position;
					capture_emitter_holder = object_new(&placement);
					unit_scripting_enter_vehicle(capture_emitter_holder, capture_vehicle,
						!strcmp(kind,"ghost") ? "G-driver" : !strcmp(kind,"banshee") ? "B-driver" :
						!strcmp(kind,"scorpion") ? "scorpion-driver" : "W-driver");
					platform_log("AMBIENT driver %ld parent %ld", capture_emitter_holder, object_get(capture_emitter_holder)->object.parent_object_index);
				}
			}
		}
	}
	if (capture_emitter != NONE && object_try_and_get(capture_emitter))
	{
		if (capture_ticks % 30 == 0)
		{
			struct object_datum *object = object_get(capture_emitter);
			platform_log("AMBIENT functions %ld %.4f %.4f %.4f %.4f", capture_ticks,
				object->object.outgoing_function_values[0], object->object.outgoing_function_values[1],
				object->object.outgoing_function_values[2], object->object.outgoing_function_values[3]);
		}
		if (capture_ticks == 200 && profiles[i].group == 'vehi') vehicle_hover(capture_emitter, FALSE);
		if (capture_ticks == 225) { object_delete(capture_emitter); capture_emitter = capture_vehicle = NONE; platform_log("AMBIENT deleted"); }
	}
}

static void ammo_capture_tick(const char *kind)
{
	struct object_placement_data placement;
	long tag, holder;
	int i;
	int sword = !strncmp(kind, "sword-", 6);
	int needler = !strncmp(kind, "needler-", 8);
	const char *pose = sword ? kind + 6 : needler ? kind + 8 : kind + 5;
	if (capture_ticks == 65)
	{
		holder = player_control_get_unit_index(0);
		if (strcmp(pose, "fp"))
		{
			tag = tag_loaded('bipd', !strcmp(pose, "elite") ? "characters\\elite\\elite" :
				!strcmp(pose, "marine") ? "characters\\marine_armored\\marine_armored" :
				!strcmp(pose, "grunt") ? "characters\\grunt\\grunt" : "characters\\cyborg\\cyborg");
			if (tag == NONE) { platform_log("EMITTER missing actor"); return; }
			object_placement_data_new(&placement, tag, NONE);
			placement.position = capture_camera.position;
			placement.position.x += capture_camera.forward.i * 1.5f;
			placement.position.y += capture_camera.forward.j * 1.5f;
			placement.position.z -= 0.6f;
			if (getenv("HALO_EMITTER_CAPTURE_GROUNDED"))
				capture_ground(&placement.position);
			placement.forward = capture_camera.forward;
			placement.forward.k = 0;
			if (getenv("HALO_EMITTER_CAPTURE_FACE_CAMERA"))
			{
				placement.forward.i = -placement.forward.i;
				placement.forward.j = -placement.forward.j;
			}
			normalize3d(&placement.forward);
			observer_up_from_forward(&placement.forward, &placement.up);
			holder = object_new(&placement);
		}
		tag = tag_loaded('weap', sword ? "weapons\\energy sword\\energy sword" : needler ? "weapons\\needler\\needler" : "weapons\\assault rifle\\assault rifle");
		if (holder == NONE || tag == NONE) return;
		capture_emitter_holder = holder;
		unit_drop_current_weapon(holder, TRUE);
		unit_drop_current_weapon(holder, TRUE);
		object_placement_data_new(&placement, tag, NONE);
		placement.position = object_get(holder)->object.position;
		capture_emitter = object_new(&placement);
		if (capture_emitter != NONE && unit_add_weapon_to_inventory(holder, capture_emitter, TRUE))
		{
			if (!strcmp(pose, "fp"))
				for (i = 0; i < MAXIMUM_WEAPONS_PER_UNIT; i++)
					if (unit_inventory_get_weapon(holder, (short)i) == capture_emitter)
						player_control_set_desired_weapon(holder, (short)i);
			platform_log("EMITTER created %ld tag ammo holder %ld mode %s", capture_emitter, holder, kind);
		}
	}
	if (capture_emitter_holder != NONE && object_try_and_get(capture_emitter_holder))
	{
		if (getenv("HALO_EMITTER_CAPTURE_GROUNDED") && strcmp(pose, "fp") && capture_ticks == 80)
		{
			real_vector3d facing;
			real_point3d target = object_get(capture_emitter_holder)->object.position;
			target.z += !strcmp(pose, "grunt") ? 0.25f : 0.4f;
			vector_from_points3d(&observer_get_camera(0)->position, &target, &facing);
			normalize3d(&facing);
			player_control_set_facing(0, &facing);
		}
		if (getenv("HALO_EMITTER_CAPTURE_CROUCH") && capture_ticks == 85)
		{
			/* An unowned fixture starts alert; only the combat stance has a crouch seat. */
			unit_get(capture_emitter_holder)->unit.animation.desired_state = _unit_animation_state_in_combat;
			unit_persistent_control(capture_emitter_holder, 90, FLAG(_unit_control_crouch_modifier_bit));
		}
		if (capture_ticks == 100 || capture_ticks == 140 || capture_ticks == 190)
		{
			struct biped_datum *biped = (struct biped_datum *)object_get(capture_emitter_holder);
			platform_log("EMITTER holder tick %ld crouch %.4f position %.4f %.4f %.4f", capture_ticks,
				biped->biped.crouch, biped->object.position.x, biped->object.position.y, biped->object.position.z);
		}
	}
	if (capture_ticks == 200 && capture_emitter_holder != NONE)
	{
		unit_drop_current_weapon(capture_emitter_holder, TRUE);
		platform_log("EMITTER ammo dropped");
	}
}
static void emitter_capture_tick(void)
{
	const char *kind = getenv("HALO_EMITTER_CAPTURE");
	struct object_placement_data placement;
	long tag;
	if (!strncmp(kind, "ammo-", 5) || !strncmp(kind, "needler-", 8) || !strncmp(kind, "sword-", 6))
	{
		ammo_capture_tick(kind);
		return;
	}
	if (capture_ticks == 65)
	{
		const char *name = !strcmp(kind, "stationary") ? "scenery\\c_field_generator\\c_field_generator" :
			!strcmp(kind, "fuel") ? "weapons\\fuel rod gun\\fuel rod" :
			!strcmp(kind, "hunter") ? "characters\\hunter\\hunter" :
			!strcmp(kind, "major") ? "characters\\jackal\\jackal major" : "characters\\jackal\\jackal";
		capture_emitter_attached = FALSE;
		tag = tag_loaded(!strcmp(kind, "stationary") ? 'scen' : !strcmp(kind, "fuel") ? 'weap' : 'bipd', name);
		if (tag == NONE) { platform_log("EMITTER missing tag %s", name); return; }
		object_placement_data_new(&placement, tag, NONE);
		placement.position = capture_camera.position;
		placement.position.x += capture_camera.forward.i * 2.5f;
		placement.position.y += capture_camera.forward.j * 2.5f;
		placement.position.z -= 0.65f;
		if (!strcmp(kind, "stationary") || !strcmp(kind, "hunter"))
		{
			struct collision_result ground;
			real_vector3d down = {0, 0, -5};
			real_point3d from = placement.position;
			from.z = capture_camera.position.z + 0.5f;
			if (collision_test_vector(FLAG(_collision_test_structure_bit) |
				FLAG(_collision_test_front_facing_surfaces_bit), &from, &down, NONE, &ground))
				placement.position.z = ground.point.z + 0.03f;
		}
		placement.forward = capture_camera.forward;
		placement.forward.i = -placement.forward.i;
		placement.forward.j = -placement.forward.j;
		placement.forward.k = 0;
		if (!strcmp(kind, "stationary"))
		{
			placement.forward.i = -capture_camera.forward.j;
			placement.forward.j = capture_camera.forward.i;
		}
		normalize3d(&placement.forward);
		observer_up_from_forward(&placement.forward, &placement.up);
		capture_emitter = object_new(&placement);
		platform_log("EMITTER created %ld tag %s", capture_emitter, name);
		if (!strcmp(kind, "fuel"))
		{
			tag = tag_loaded('bipd', "characters\\grunt\\grunt");
			if (tag != NONE)
			{
				struct object_placement_data holder;
				object_placement_data_new(&holder, tag, NONE);
				holder.position = placement.position;
				holder.forward = placement.forward;
				holder.up = placement.up;
				capture_emitter_holder = object_new(&holder);
				if (capture_emitter_holder != NONE && capture_emitter != NONE)
				{
					int held = unit_add_weapon_to_inventory(capture_emitter_holder, capture_emitter, TRUE);
					/* This fixture creates the gun loose before attaching it. Clear
					   that spawn-time detonation; the later stock drop stays live. */
					if (held) weapon_get(capture_emitter)->item.detonation_ticks = 0;
					platform_log("EMITTER held %d", held);
				}
			}
		}
	}
	if (capture_emitter != NONE && object_try_and_get(capture_emitter))
	{
		if (strcmp(kind, "fuel") && strcmp(kind, "hunter") && (capture_ticks == 100 || capture_ticks == 112 ||
			capture_ticks == 140 || capture_ticks == 160 || capture_ticks == 195))
		{
			struct object_datum *object = object_get(capture_emitter);
			platform_log("EMITTER shield tick %ld flash %.5f vitality %.5f depleted %d", capture_ticks,
				object->object.outgoing_function_values[0], object->object.shield_vitality,
				TEST_FLAG(object->object.damage_flags, _object_shield_depleted_bit));
		}
		if (!strcmp(kind, "fuel") && !capture_emitter_attached &&
			object_get(capture_emitter)->object.parent_object_index == capture_emitter_holder)
		{
			/* Inventory insertion precedes the actual hand attachment. The
			   loose spawn can arm itself until that animation completes. */
			weapon_get(capture_emitter)->item.detonation_ticks = 0;
			capture_emitter_attached = TRUE;
			platform_log("EMITTER attached");
		}
		if (capture_ticks == 110 && strcmp(kind, "fuel") && strcmp(kind, "hunter"))
		{
			struct damage_network_state state;
			damage_get_network_state(capture_emitter, &state);
			state.current_shield_damage = 1.0f;
			state.recent_shield_damage = 0.5f;
			state.shield_vitality = 0.75f;
			damage_set_network_state(capture_emitter, &state);
			platform_log("EMITTER hit %ld", capture_emitter);
		}
		if (capture_ticks == 150 && strcmp(kind, "fuel") && strcmp(kind, "hunter"))
		{
			object_deplete_shield(capture_emitter);
			platform_log("EMITTER depleted %ld", capture_emitter);
		}
		if (capture_ticks == 180 && strcmp(kind, "fuel") && strcmp(kind, "hunter"))
		{
			struct damage_network_state state;
			damage_get_network_state(capture_emitter, &state);
			state.shield_depleted = FALSE;
			state.shield_vitality = 1.0f;
			damage_set_network_state(capture_emitter, &state);
			platform_log("EMITTER recovered %ld", capture_emitter);
		}
		if (capture_ticks == 150 && !strcmp(kind, "fuel") && capture_emitter_holder != NONE)
		{
			unit_drop_current_weapon(capture_emitter_holder, TRUE);
			platform_log("EMITTER dropped");
		}
		if (capture_ticks == 225)
		{
			object_delete(capture_emitter);
			capture_emitter = NONE;
			platform_log("EMITTER deleted");
		}
	}
}
static const char *capture_projectile_tags[] = {
	"weapons\\plasma rifle\\bolt", "weapons\\plasma pistol\\bolt",
	"weapons\\plasma rifle\\charged bolt", "weapons\\rocket launcher\\rocket",
	"weapons\\needler\\needle",
	"vehicles\\ghost\\ghost bolt",
	"vehicles\\banshee\\banshee bolt",
	"vehicles\\banshee\\banshee fuel rod",
	"vehicles\\c gun turret\\c gun turret",
	"weapons\\fuel rod gun\\fuel rod",
	"vehicles\\wraith\\wraith mortar",
	"weapons\\needler\\mp_needle",
	"vehicles\\banshee\\mp_banshee fuel rod",
	"vehicles\\c gun turret\\mp gun turret",
	"weapons\\plasma_cannon\\plasma_cannon",
	"weapons\\plasma grenade\\plasma grenade"
};
static int capture_kind(void)
{
	const char *kind = getenv("HALO_PLASMA_CAPTURE_KIND");
	if (kind && !strcmp(kind, "pistol")) return 1;
	if (kind && !strcmp(kind, "charged")) return 2;
	if (kind && !strcmp(kind, "rocket")) return 3;
	if (kind && !strcmp(kind, "needle")) return 4;
	if (kind && !strcmp(kind, "ghost")) return 5;
	if (kind && !strcmp(kind, "banshee")) return 6;
	if (kind && !strcmp(kind, "banshee-fuel")) return 7;
	if (kind && !strcmp(kind, "shade")) return 8;
	if (kind && !strcmp(kind, "fuel")) return 9;
	if (kind && !strcmp(kind, "wraith")) return 10;
	if (kind && !strcmp(kind, "mp-needle")) return 11;
	if (kind && !strcmp(kind, "mp-banshee-fuel")) return 12;
	if (kind && !strcmp(kind, "mp-shade")) return 13;
	if (kind && !strcmp(kind, "ce-fuel")) return 14;
	if (kind && !strcmp(kind, "grenade")) return 15;
	return 0;
}
int config_write_boolean(const char *name, int value);
static real capture_distance, capture_height, capture_receiver_distance, capture_span;
static real capture_option(const char *name, real fallback)
{
	const char *value = getenv(name);
	return value ? (real)atof(value) : fallback;
}
static unsigned long capture_model_draws, capture_specular_draws;
static int capture_specular_selected;
void plasma_capture_model_draw(unsigned int mask)
{
	if (mask)
		capture_model_draws++;
}
void plasma_capture_specular_begin(long index)
{
	capture_specular_selected = plasma_lighting_selected(index);
}
void plasma_capture_specular_draw(void)
{
	if (capture_specular_selected)
		capture_specular_draws++;
}

int plasma_capture_hold_projectile(long index)
{
	int n;
	for (n = 0; n < capture_burst; n++)
		if (capture_probe && index == capture_bolts[n])
			return 1;
	return 0;
}
/* Shield-only acceptance: controlled charge, layer loss, hit and recharge.
   These writes exist exclusively in this test executable. */
static void shield_capture_tick(void)
{
	const char *mode = getenv("HALO_SHIELD_CAPTURE");
	struct object_placement_data placement;
	struct unit_datum *unit;
	long tag;
	if (capture_ticks == 65)
	{
		if (!strcmp(mode, "fp")) capture_target = player_control_get_unit_index(0);
		else
		{
			tag = tag_loaded('bipd', !strcmp(mode, "elite") ? "characters\\elite\\elite" : "characters\\cyborg\\cyborg");
			if (tag == NONE) { platform_log("SHIELD missing actor"); return; }
			object_placement_data_new(&placement, tag, NONE);
			placement.position = capture_camera.position;
			placement.position.x += capture_camera.forward.i * 2.f;
			placement.position.y += capture_camera.forward.j * 2.f;
			capture_ground(&placement.position);
			placement.forward = capture_camera.forward;
			placement.forward.i = -placement.forward.i; placement.forward.j = -placement.forward.j; placement.forward.k = 0;
			normalize3d(&placement.forward);
			observer_up_from_forward(&placement.forward, &placement.up);
			capture_target = object_new(&placement);
		}
		platform_log("SHIELD created %ld mode %s", capture_target, mode);
	}
	if (capture_target == NONE || !(unit = unit_try_and_get(capture_target))) return;
	if (!strcmp(mode, "charge") || !strcmp(mode, "fp"))
	{
		SET_FLAG(unit->object.damage_flags, _object_shield_over_charging_bit, capture_ticks >= 90 && capture_ticks < 150);
		unit->object.shield_vitality = capture_ticks < 90 ? 1.f : capture_ticks < 150 ? 1.f + (capture_ticks - 90) / 30.f : capture_ticks < 180 ? 3.f : capture_ticks < 220 ? 1.8f : 1.f;
	}
	else
	{
		if (capture_ticks == 100 || capture_ticks == 130)
		{
			unit->object.recent_shield_damage = 1.f;
			unit->object.current_shield_damage = 1.f;
			unit->object.shield_damage_decay_timer = 0;
			unit->object.shield_vitality = .45f;
		}
		if (capture_ticks >= 160 && capture_ticks < 210)
		{
			SET_FLAG(unit->object.damage_flags, _object_shield_charging_bit, TRUE);
			unit->object.shield_stun_ticks = 0;
			unit->object.shield_vitality = .1f + .9f * (capture_ticks - 160) / 50.f;
		}
	}
	if (getenv("HALO_EMITTER_CAPTURE_CROUCH") && capture_ticks == 110)
	{
		unit->unit.animation.desired_state = _unit_animation_state_in_combat;
		unit_persistent_control(capture_target, 65, FLAG(_unit_control_crouch_modifier_bit));
	}
	if (capture_ticks % 10 == 0)
		platform_log("SHIELD state tick %ld vitality %.4f values %.4f %.4f %.4f %.4f", capture_ticks, unit->object.shield_vitality,
			unit->object.outgoing_function_values[0], unit->object.outgoing_function_values[1], unit->object.outgoing_function_values[2], unit->object.outgoing_function_values[3]);
}

void plasma_capture_tick(void)
{
	struct object_placement_data placement;
	real_vector3d right;
	long tag, i, bolt;
	long now = game_time_get();
	if (now < capture_last_time)
	{
		capture_ticks = 0;
		capture_projectile = capture_door = capture_target = NONE;
		capture_emitter = capture_emitter_holder = NONE;
		capture_vehicle = NONE;
		for (i = 0; i < 32; i++)
			capture_bolts[i] = NONE;
	}
	capture_last_time = now;
	if (!game_in_progress() || game_time_menu_scene())
		return;
	if ((getenv("HALO_PLASMA_CAPTURE_DOOR") || getenv("HALO_COVENANT_CAPTURE_CAMPAIGN")) && cinematic_in_progress())
		return;
	/* Keep campaign captures reproducible without AI attacks or reloads from
	   the fixture's nearby explosions. Projectile flight/effects remain live. */
	if (getenv("HALO_COVENANT_CAPTURE_CAMPAIGN"))
	{
		ai_globals_ai_active(FALSE);
		cheat.deathless_player = TRUE;
	}
	capture_ticks++;
	if (getenv("HALO_WEAPON_GLOW") && capture_ticks % 15 == 0)
	{
		long holder = player_control_get_unit_index(0);
		struct unit_datum *unit = unit_try_and_get(holder);
		if (unit && unit->unit.current_weapon_index >= 0 && unit->unit.current_weapon_index < MAXIMUM_WEAPONS_PER_UNIT)
		{
			struct weapon_datum *weapon = weapon_try_and_get(unit_inventory_get_weapon(holder, unit->unit.current_weapon_index));
			if (weapon) platform_log("EMITTER weapon tick %ld heat %.4f charged %.4f state %d effect %ld", capture_ticks,
				weapon->weapon.heat, weapon->weapon.overcharged, weapon->weapon.triggers[0].state, weapon->weapon.overheated_effect_index);
		}
	}

	if ((getenv("HALO_WARTHOG_CAPTURE_EXTERIOR") || getenv("HALO_SCORPION_CAPTURE")) && capture_vehicle != NONE &&
		object_try_and_get(capture_vehicle))
	{
		real_vector3d facing = object_get(capture_vehicle)->object.forward;
		if (capture_ticks >= 170)
		{
			facing.i = -facing.i;
			facing.j = -facing.j;
		}
		else if (capture_ticks >= 120)
		{
			real x = facing.i;
			facing.i = -facing.j;
			facing.j = x;
		}
		facing.k = -0.15f;
		normalize3d(&facing);
		player_control_set_facing(0, &facing);
		if (capture_ticks == 110 || capture_ticks == 160 || capture_ticks == 210)
		{
			struct object_marker marker;
			if (object_get_marker_by_name(capture_vehicle, "primary trigger", &marker, 1))
				platform_log("WARTHOG aim tick %ld forward %.4f %.4f %.4f", capture_ticks,
					marker.matrix.forward.i, marker.matrix.forward.j, marker.matrix.forward.k);
		}
	}
	if (capture_ticks == 60)
	{
		capture_camera = *observer_get_camera(0);
		capture_combat = getenv("HALO_PLASMA_CAPTURE_COMBAT") != NULL;
		capture_toggle = getenv("HALO_PLASMA_CAPTURE_TOGGLE") != NULL;
		capture_model = getenv("HALO_PLASMA_CAPTURE_MODEL") != NULL;
		capture_burst = (int)capture_option("HALO_PLASMA_CAPTURE_BURST", 1);
		if (capture_burst < 1 || capture_burst > 32)
			capture_burst = 1;
		for (i = 0; i < 32; i++)
			capture_bolts[i] = NONE;
		capture_probe = getenv("HALO_PLASMA_CAPTURE_PROBE") != NULL;
		capture_span = capture_option("HALO_PLASMA_CAPTURE_SPAN", 2.0f);
		capture_distance = capture_option("HALO_PLASMA_CAPTURE_DISTANCE", capture_model ? 2.2f : 3.0f);
		capture_height = capture_option("HALO_PLASMA_CAPTURE_HEIGHT", capture_model ? -0.2f : -0.6f);
		capture_receiver_distance = capture_option("HALO_PLASMA_CAPTURE_RECEIVER_DISTANCE", 2.8f);
		platform_log("PLASMA camera %.4f %.4f %.4f forward %.4f %.4f %.4f", capture_camera.position.x,
					 capture_camera.position.y, capture_camera.position.z, capture_camera.forward.i,
					 capture_camera.forward.j, capture_camera.forward.k);
	}
	if (getenv("HALO_AMBIENT_CAPTURE")) { ambient_capture_tick(getenv("HALO_AMBIENT_CAPTURE")); return; }
	if (getenv("HALO_SHIELD_CAPTURE")) { shield_capture_tick(); return; }
	if (getenv("HALO_EMITTER_CAPTURE"))
	{
		emitter_capture_tick();
		return;
	}
	if (capture_ticks == 65 && capture_combat)
	{
		long unit = player_control_get_unit_index(0);
		const char *role = getenv("HALO_NETWORK_TEST");
		if (getenv("HALO_WARTHOG_CAPTURE") || getenv("HALO_SCORPION_CAPTURE"))
		{
			long vehicle;
			int scorpion = getenv("HALO_SCORPION_CAPTURE") != NULL;
			if (role && !strncmp(role, "join", 4)) return;
			tag = tag_loaded('vehi', scorpion ? "vehicles\\scorpion\\scorpion_mp" : "vehicles\\warthog\\warthog");
			if (tag == NONE && scorpion) tag = tag_loaded('vehi', "vehicles\\scorpion\\scorpion");
			if (tag == NONE || unit == NONE) return;
			object_placement_data_new(&placement, tag, NONE);
			placement.position = capture_camera.position;
			placement.position.x += capture_camera.forward.i * 4.0f;
			placement.position.y += capture_camera.forward.j * 4.0f;
			placement.position.z -= 0.5f;
			placement.forward = capture_camera.forward;
			placement.forward.k = 0;
			normalize3d(&placement.forward);
			observer_up_from_forward(&placement.forward, &placement.up);
			vehicle = object_new(&placement);
			capture_vehicle = vehicle;
			if (vehicle != NONE)
			{
				unit_scripting_enter_vehicle(unit, vehicle, scorpion ? "scorpion-driver" : "W-gunner");
				platform_log("WARTHOG seat vehicle %ld parent %ld", vehicle, object_get(unit)->object.parent_object_index);
				if (object_get(unit)->object.parent_object_index == vehicle)
					platform_log("PLASMA combat equipped %s %ld", scorpion ? "Scorpion" : "Warthog", vehicle);
			}
			return;
		}
		tag = tag_loaded('weap', capture_kind() == 3 ? "weapons\\rocket launcher\\rocket launcher" : capture_kind() ? "weapons\\plasma pistol\\plasma pistol" : "weapons\\plasma rifle\\plasma rifle");
		if (getenv("HALO_MUZZLE_CAPTURE_WEAPON"))
			tag = tag_loaded('weap', getenv("HALO_MUZZLE_CAPTURE_WEAPON"));
		if (unit == NONE || tag == NONE)
			platform_log("PLASMA combat unavailable unit %ld weapon tag %ld", unit, tag);
		if (unit != NONE && tag != NONE && (!role || strncmp(role, "join", 4)))
		{
			long weapon;
			if (role && !strncmp(role, "host", 4))
			{
				struct data_iterator players;
				struct player_datum *player;
				data_iterator_new(&players, player_data);
				while ((player = (struct player_datum *)data_iterator_next(&players)) != NULL)
					if (player->unit_index != NONE && player->unit_index != unit)
					{
						real_point3d position = object_get(unit)->object.position;
						position.x += capture_camera.forward.i * 4;
						position.y += capture_camera.forward.j * 4;
						object_set_position(player->unit_index, &position, NULL, NULL);
						platform_log("PLASMA combat target %ld", player->unit_index);
					}
			}
			unit_drop_current_weapon(unit, TRUE);
			unit_drop_current_weapon(unit, TRUE);
			object_placement_data_new(&placement, tag, NONE);
			placement.position = capture_camera.position;
			weapon = object_new(&placement);
			if (weapon != NONE && unit_add_weapon_to_inventory(unit, weapon, TRUE))
			{
				for (i = 0; i < MAXIMUM_WEAPONS_PER_UNIT; i++)
					if (unit_inventory_get_weapon(unit, (short)i) == weapon)
						player_control_set_desired_weapon(unit, (short)i);
				platform_log("PLASMA combat equipped %ld tag %s", weapon, tag_get_name(tag));
			}
			else
				platform_log("PLASMA combat inventory rejected weapon %ld tag %s", weapon, tag_get_name(tag));
		}
	}
	if (capture_ticks == 70 && getenv("HALO_PLASMA_CAPTURE_DOOR"))
	{
		struct tag_iterator iterator;
		tag_iterator_new(&iterator, 'mach');
		while ((tag = tag_iterator_next(&iterator)) != NONE)
			if (strstr(tag_get_name(tag), "door"))
			{
				object_placement_data_new(&placement, tag, NONE);
				placement.position = capture_camera.position;
				placement.position.x += capture_camera.forward.i * 2.5f;
				placement.position.y += capture_camera.forward.j * 2.5f;
				placement.position.z -= 0.6f;
				placement.forward = capture_camera.forward;
				observer_up_from_forward(&placement.forward, &placement.up);
				capture_door = object_new(&placement);
				if (capture_door != NONE)
				{
					struct scenario_device_datum info;
					info.power_group_index = info.position_group_index = NONE;
					info.flags = 0;
					device_add_scenario_information(capture_door, &info);
					device_operates_automatically_set(capture_door, FALSE);
					device_set_power(capture_door, 1);
					device_set_actual_position(capture_door, 0);
				}
				platform_log("PLASMA door %ld tag %s", capture_door, tag_get_name(tag));
				break;
			}
	}
	if (capture_door != NONE && object_try_and_get(capture_door) && capture_ticks % 30 == 0)
	{
		device_set_desired_position(capture_door, (capture_ticks / 30) % 2 ? 1.0f : 0.0f);
		platform_log("PLASMA door position %.3f", device_get_position(capture_door));
	}
	if (capture_toggle && capture_ticks >= 100 && capture_ticks <= 220 && capture_ticks % 10 == 0)
	{
		config_write_boolean("display.plasma_occlusion", (capture_ticks / 10) % 2);
		platform_log("PLASMA toggle %d", config_boolean("display.plasma_occlusion"));
	}
	if (capture_combat)
		return;
	if (capture_ticks == 65 && capture_model)
	{
		tag = tag_loaded('bipd', "characters\\cyborg_mp\\cyborg_mp");
		if (tag == NONE)
			tag = tag_loaded('bipd', "characters\\cyborg\\cyborg");
		if (tag != NONE)
		{
			object_placement_data_new(&placement, tag, NONE);
			placement.position.x =
				capture_camera.position.x + capture_receiver_distance * capture_camera.forward.i;
			placement.position.y =
				capture_camera.position.y + capture_receiver_distance * capture_camera.forward.j;
			placement.position.z = capture_camera.position.z - 0.6f;
			placement.forward = capture_camera.forward;
			placement.forward.i = -placement.forward.i;
			placement.forward.j = -placement.forward.j;
			observer_up_from_forward(&placement.forward, &placement.up);
			capture_target = object_new(&placement);
			platform_log("PLASMA receiver model %ld tag %s", capture_target, tag_get_name(tag));
		}
		else
			platform_log("PLASMA receiver missing");
	}
	for (bolt = 0; bolt < capture_burst; bolt++)
		if (capture_probe && capture_bolts[bolt] != NONE && object_try_and_get(capture_bolts[bolt]))
		{
			real_point3d position;
			real lateral = -capture_span + 2 * capture_span * (capture_ticks % 60 - 30) / 28.0f;
			cross_product3d(&capture_camera.forward, &capture_camera.up, &right);
			position.x = capture_camera.position.x + capture_distance * capture_camera.forward.i +
						 lateral * right.i + capture_height * capture_camera.up.i;
			position.y = capture_camera.position.y + capture_distance * capture_camera.forward.j +
						 lateral * right.j + capture_height * capture_camera.up.j;
			position.z = capture_camera.position.z + capture_distance * capture_camera.forward.k +
						 lateral * right.k + capture_height * capture_camera.up.k;
			if (capture_ticks % 60 >= 59 || capture_ticks % 60 < 30)
			{
				object_delete(capture_bolts[bolt]);
				capture_bolts[bolt] = NONE;
			}
			else
			{
				position.z += bolt * 0.025f;
				object_set_position(capture_bolts[bolt], &position, NULL, NULL);
			}
		}
	if (capture_ticks < 90 || capture_ticks > 210 || capture_ticks % 60 != 30)
		return;
	tag = tag_loaded(PROJECTILE_DEFINITION_TAG, capture_projectile_tags[capture_kind()]);
	if (tag == NONE)
	{
		platform_log("PLASMA missing projectile tag");
		return;
	}
	cross_product3d(&capture_camera.forward, &capture_camera.up, &right);
	object_placement_data_new(&placement, tag, NONE);
	placement.position.x = capture_camera.position.x + capture_distance * capture_camera.forward.i -
						   (capture_probe ? capture_span : 1.2f) * right.i +
						   capture_height * capture_camera.up.i;
	placement.position.y = capture_camera.position.y + capture_distance * capture_camera.forward.j -
						   (capture_probe ? capture_span : 1.2f) * right.j +
						   capture_height * capture_camera.up.j;
	placement.position.z = capture_camera.position.z + capture_distance * capture_camera.forward.k -
						   (capture_probe ? capture_span : 1.2f) * right.k +
						   capture_height * capture_camera.up.k;
	placement.forward = right;
	placement.forward.k -= capture_option("HALO_PLASMA_CAPTURE_DROP", 0.12f);
	normalize3d(&placement.forward);
	observer_up_from_forward(&placement.forward, &placement.up);
	for (bolt = 0; bolt < capture_burst; bolt++)
	{
		placement.position.z += 0.025f;
		if (getenv("HALO_PLASMA_CAPTURE_MIXED"))
			placement.definition_index = tag_loaded(PROJECTILE_DEFINITION_TAG, capture_projectile_tags[bolt % (capture_kind() == 3 ? 4 : 3)]);
		capture_bolts[bolt] = object_new(&placement);
		if (capture_bolts[bolt] != NONE && capture_target != NONE && (capture_kind() == 2 || capture_kind() == 4 || capture_kind() == 11) && !capture_probe)
		{
			projectile_set_target_object_index(capture_bolts[bolt], capture_target);
			platform_log("PLASMA homing %ld target %ld", capture_bolts[bolt], capture_target);
		}
	}
	capture_projectile = capture_bolts[0];
	platform_log("PLASMA shot tick %ld object %ld", capture_ticks, capture_projectile);
	if (capture_projectile != NONE)
	{
		struct object_datum *object = object_get(capture_projectile);
		struct object_definition *definition = object_definition_get(tag);
		platform_log("PLASMA tag %s type %d nodes %ld flags %lx", tag_get_name(tag), object->object.type, object->object.node_matrices.size, object->object.flags);
		if (capture_kind() == 3 || capture_kind() == 4 || capture_kind() == 10 || capture_kind() == 11)
		{
			long glow = light_port_glow_definition_get();
			if (glow != NONE)
			{
				long *light = tag_get('ligh', glow);
				platform_log("PLASMA glow template %s flags %lx", tag_get_name(glow), light[0]);
			}
		}
		for (i = 0; i < definition->object.attachments.count; i++)
		{
			struct object_attachment_definition *attachment =
				TAG_BLOCK_GET_ELEMENT(&definition->object.attachments, i, struct object_attachment_definition);
			if (attachment->type.group_tag == 'ligh' && attachment->type.index != NONE)
			{
				long *light = tag_get('ligh', attachment->type.index);
				platform_log("PLASMA enabled %d light flags %lx radius %.4f",
					config_boolean("display.plasma_lighting"), light[0], ((real *)light)[1]);
			}
		}
		for (i = 0; i < MAXIMUM_NUMBER_OF_ATTACHMENTS_PER_OBJECT; i++)
			platform_log("PLASMA attachment %ld type %d index %ld", i, object->object.attachment_types[i],
						 object->object.attachment_indices[i]);
	}
}

void plasma_capture_frame(void)
{
	long index, selected = 0;
	real_matrix4x3 *matrix;
	if (!game_in_progress() || game_time_menu_scene() || capture_ticks < 85 || capture_ticks > 240)
		return;
	platform_log("PLASMA draws model %lu specular %lu", capture_model_draws, capture_specular_draws);
	capture_model_draws = capture_specular_draws = 0;
	for (index = 0; index < rasterizer_lights.light_count; index++)
		selected += plasma_lighting_selected(index);
	if (capture_projectile != NONE && object_try_and_get(capture_projectile))
	{
		matrix = object_get_node_matrix(capture_projectile, 0);
		platform_log("PLASMA frame %lu tick %ld fraction %.5f position %.5f %.5f %.5f selected %ld",
					 GetTickCount(), capture_ticks, render_interpolation_fraction(), matrix->position.x,
					 matrix->position.y, matrix->position.z, selected);
	}
	else
		platform_log("PLASMA frame %lu tick %ld no projectile selected %ld", GetTickCount(), capture_ticks,
					 selected);
}

int plasma_capture_should_capture(void)
{
	static long shield_captured_tick = -1;
	if (getenv("HALO_SHIELD_CAPTURE") || getenv("HALO_LIGHTING_CAPTURE_SPARSE"))
	{
		if (getenv("HALO_PLASMA_CAPTURE_NO_IMAGES") || capture_ticks < 85 || capture_ticks > 240 ||
			capture_ticks % (getenv("HALO_LIGHTING_CAPTURE_SPARSE") ? (int)capture_option("HALO_LIGHTING_CAPTURE_IMAGE_STEP", 3) : 5) || shield_captured_tick == capture_ticks) return FALSE;
		shield_captured_tick = capture_ticks;
		return TRUE;
	}
	return !getenv("HALO_PLASMA_CAPTURE_NO_IMAGES") && capture_ticks >= 85 && capture_ticks <= 240 &&
		((capture_ticks % 60 >= 28 && capture_ticks % 60 <= 58) ||
		 (getenv("HALO_EMITTER_CAPTURE_CROUCH") && capture_ticks >= 180 && capture_ticks < 200));
}
