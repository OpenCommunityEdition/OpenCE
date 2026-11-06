/* Short, render-owned light pulses. Successful shots only record identities;
   they never allocate engine lights or alter the weapon's authored effects. */
#include "cseries.h"
#include "objects/objects.h"
#include "objects/object_lights.h"
#include "items/weapons.h"
#include "units/units.h"
#include "interface/first_person_weapons.h"
#include "camera/observer.h"
#include "game/players.h"
#include "tag_files/tag_files.h"
#include "muzzle_lighting.h"
#include "plasma_occlusion.h"
#include <string.h>

#include "lighting_config.h"
real render_interpolation_game_time_sec(long ticks);
long light_port_glow_definition_get(void);
boolean light_port_glow_set(long light, long owner, real_rgb_color const *color, real radius);
boolean light_port_glow_move(long light, real_point3d const *position);

static const struct muzzle_profile
{
	const char *tag;
	real_rgb_color color;
	real radius, duration;
	short triggers; /* bit mask; zero keeps the existing shared pulse */
	boolean vehicle_occlusion;
	const char *marker;
	real forward_offset;
} muzzle_profiles[] = {
	{"weapons\\assault rifle\\assault rifle", {1.0f, 0.65f, 0.25f}, 1.8f, 0.10f},
	{"weapons\\pistol\\pistol", {1.0f, 0.65f, 0.25f}, 1.8f, 0.12f},
	{"weapons\\shotgun\\shotgun", {1.0f, 0.55f, 0.18f}, 2.2f, 0.14f},
	{"weapons\\sniper rifle\\sniper rifle", {1.0f, 0.70f, 0.35f}, 2.0f, 0.12f},
	{"weapons\\plasma rifle\\plasma rifle", {0.15f, 0.75f, 1.0f}, 1.6f, 0.10f},
	{"weapons\\plasma pistol\\plasma pistol", {0.25f, 1.0f, 0.12f}, 1.6f, 0.12f},
	{"weapons\\needler\\mp_needler", {0.85f, 0.20f, 1.0f}, 1.3f, 0.09f},
	{"weapons\\needler\\needler", {0.85f, 0.20f, 1.0f}, 1.3f, 0.09f},
	{"weapons\\rocket launcher\\rocket launcher", {1.0f, 0.50f, 0.15f}, 2.0f, 0.14f},
	{"weapons\\flamethrower\\flamethrower", {1.0f, 0.40f, 0.10f}, 1.4f, 0.08f},
	/* The model-less turret weapon uses the vehicle's rendered muzzle marker.
	   A wider warm pulse lights the hull and ground without changing its flash. */
	{"vehicles\\warthog\\warthog gun", {1.0f, 0.75f, 0.35f}, 5.0f, 0.16f, 0, TRUE},
	/* Separate triggers retain both pulses when the tank fires both guns.
	   The cannon flash is ahead of its projectile marker; move the coaxial
	   source just beyond its barrel so the vehicle can occlude it correctly. */
	{"vehicles\\scorpion\\scorpion cannon", {1.0f, 0.78f, 0.42f}, 7.0f, 0.22f, 1, TRUE, "cannon flash", 0.0f},
	{"vehicles\\scorpion\\scorpion cannon", {1.0f, 0.75f, 0.35f}, 5.0f, 0.12f, 2, TRUE, "secondary trigger", 0.08f},
};
enum { MUZZLE_LIGHTS = 8 };
static struct muzzle_pulse
{
	int active, connected, profile, pending;
	long weapon, owner, light, rasterizer, shot_tick;
	short trigger;
	real start, radius, duration;
} pulses[MUZZLE_LIGHTS];
static const real_rgb_color muzzle_black = {0, 0, 0};

/* Zero slope at both ends, evaluated at render time rather than tick count. */
static real muzzle_envelope(real age, real duration)
{
	real t = PIN(age / duration, 0.0f, 1.0f);
	return (1.0f - t) * (1.0f - t) * (1.0f + 2.0f * t);
}

static void muzzle_release(struct muzzle_pulse *pulse)
{
	if (pulse->active && pulse->light != NONE &&
		light_port_glow_set(pulse->light, NONE, &muzzle_black, 0.0f))
		light_delete(pulse->light);
	memset(pulse, 0, sizeof(*pulse));
}

void muzzle_lighting_shot(long weapon_index, short trigger_index)
{
	struct weapon_datum *weapon;
	const char *name;
	struct muzzle_pulse *pulse;
	int i, profile, slot = NONE;
	if (!lighting_config_boolean("display.muzzle_lighting") || trigger_index < 0 || trigger_index > 1)
		return;
	weapon = weapon_try_and_get(weapon_index);
	if (!weapon || !unit_try_and_get(weapon->object.parent_object_index))
		return;
	name = tag_get_name(weapon->definition_index);
	if (!name)
		return;
	for (profile = 0; profile < NUMBEROF(muzzle_profiles); profile++)
		if (!strcmp(name, muzzle_profiles[profile].tag) &&
			(!muzzle_profiles[profile].triggers || TEST_FLAG(muzzle_profiles[profile].triggers, trigger_index)))
			break;
	if (profile == NUMBEROF(muzzle_profiles))
		return; /* unfamiliar custom weapons retain their authored effects */
	for (i = 0; i < MUZZLE_LIGHTS; i++)
	{
		if (pulses[i].active && pulses[i].weapon == weapon_index &&
			(!muzzle_profiles[profile].triggers || pulses[i].trigger == trigger_index))
		{
			slot = i;
			break;
		}
		if (!pulses[i].active && slot == NONE)
			slot = i;
	}
	if (slot == NONE)
		return; /* never evict a fading light to admit another */
	pulse = &pulses[slot];
	if (pulse->active && pulse->shot_tick == game_time_get() && pulse->trigger == trigger_index)
		return;
	if (!pulse->active)
	{
		pulse->light = pulse->rasterizer = NONE;
		pulse->connected = FALSE;
	}
	pulse->active = pulse->pending = TRUE;
	pulse->weapon = weapon_index;
	pulse->owner = weapon->object.parent_object_index;
	pulse->shot_tick = game_time_get();
	pulse->trigger = trigger_index;
	pulse->profile = profile;
	pulse->radius = muzzle_profiles[profile].radius;
	pulse->duration = muzzle_profiles[profile].duration;
	if (!strcmp(name, "weapons\\plasma pistol\\plasma pistol") && trigger_index == 1)
	{
		pulse->radius = 2.2f;
		pulse->duration = 0.16f;
	}
}

static int muzzle_position(struct muzzle_pulse *pulse, real_point3d *position)
{
	struct weapon_datum *weapon = weapon_try_and_get(pulse->weapon);
	struct unit_datum *unit = unit_try_and_get(pulse->owner);
	struct object_marker marker;
	const char *name = pulse->trigger ? "secondary trigger" : "primary trigger";
	short player;
	real_vector3d forward, up;
	const struct muzzle_profile *profile = &muzzle_profiles[pulse->profile];
	if (!weapon || !unit || weapon->object.parent_object_index != pulse->owner ||
		unit->unit.current_weapon_index < 0 || unit->unit.current_weapon_index >= MAXIMUM_WEAPONS_PER_UNIT ||
		unit_inventory_get_weapon(pulse->owner, unit->unit.current_weapon_index) != pulse->weapon)
		return FALSE;
	if (profile->marker)
		name = profile->marker;
	if (!first_person_weapon_adjust_light(pulse->weapon, name, position, &forward, &up))
	{
		long object = TEST_FLAG(weapon->object.flags, _object_invisible_bit) ? pulse->owner : pulse->weapon;
		if (object_get(object)->object.node_matrices.size < sizeof(real_matrix4x3) ||
			!object_get_marker_by_name(object, name, &marker, 1))
			return FALSE;
		*position = marker.matrix.position;
		forward = marker.matrix.forward;
	}
	if (profile->forward_offset != 0.0f)
	{
		position->x += forward.i * profile->forward_offset;
		position->y += forward.j * profile->forward_offset;
		position->z += forward.k * profile->forward_offset;
	}
	for (player = local_player_get_next(NONE); player != NONE; player = local_player_get_next(player))
	{
		const struct observer_result *camera = observer_get_camera(player);
		if (camera && distance3d(position, &camera->position) <= 25.0f)
			return TRUE;
	}
	return FALSE;
}

void muzzle_lighting_update(void)
{
	int i;
	real now = render_interpolation_game_time_sec(game_time_get());
	for (i = 0; i < MUZZLE_LIGHTS; i++)
	{
		struct muzzle_pulse *pulse = &pulses[i];
		real_point3d position;
		real_rgb_color color;
		real strength;
		long definition;
		if (!pulse->active)
			continue;
		if (!lighting_config_boolean("display.muzzle_lighting") || game_time_get() < pulse->shot_tick ||
			(pulse->pending && game_time_get() - pulse->shot_tick > 2) ||
			!muzzle_position(pulse, &position))
		{
			muzzle_release(pulse);
			continue;
		}
		if (pulse->pending)
		{
			pulse->start = now;
			pulse->pending = FALSE;
		}
		strength = muzzle_envelope(now - pulse->start, pulse->duration);
		if (now < pulse->start || strength <= 0.0f)
		{
			muzzle_release(pulse);
			continue;
		}
		color = muzzle_profiles[pulse->profile].color;
		color.red *= strength;
		color.green *= strength;
		color.blue *= strength;
		if (pulse->light == NONE)
		{
			definition = light_port_glow_definition_get();
			if (definition != NONE)
				pulse->light = light_new_unattached(definition, NONE, 0, &position, global_forward3d, 0.0f);
		}
		if (!light_port_glow_set(pulse->light, NONE, &color, pulse->radius) ||
			!light_port_glow_move(pulse->light, &position))
		{
			muzzle_release(pulse);
			continue;
		}
		pulse->connected = TRUE;
	}
}

void muzzle_lighting_frame_end(void)
{
	int i;
	for (i = 0; i < MUZZLE_LIGHTS; i++)
	{
		struct muzzle_pulse *pulse = &pulses[i];
		if (!pulse->active)
			continue;
		if (!lighting_config_boolean("display.muzzle_lighting"))
			muzzle_release(pulse);
		else if (pulse->connected && light_port_glow_set(pulse->light, NONE, &muzzle_black, 0.0f))
		{
			light_disconnect_from_map(pulse->light);
			pulse->connected = FALSE;
		}
		pulse->rasterizer = NONE;
	}
}

void muzzle_lighting_forget(void)
{
	memset(pulses, 0, sizeof(pulses));
}

void muzzle_lighting_window_begin(void)
{
	int i;
	for (i = 0; i < MUZZLE_LIGHTS; i++)
		pulses[i].rasterizer = NONE;
}

void muzzle_lighting_submitted(long light_index, long rasterizer_index)
{
	int i;
	for (i = 0; i < MUZZLE_LIGHTS; i++)
		if (pulses[i].active && pulses[i].light == light_index)
		{
			pulses[i].rasterizer = rasterizer_index;
			plasma_occlusion_submit(light_index, rasterizer_index);
		}
}

int muzzle_lighting_selected(long rasterizer_index)
{
	int i;
	if (!lighting_config_boolean("display.muzzle_lighting") || rasterizer_index == NONE)
		return FALSE;
	for (i = 0; i < MUZZLE_LIGHTS; i++)
		if (pulses[i].active && pulses[i].rasterizer == rasterizer_index)
			return TRUE;
	return FALSE;
}

float muzzle_lighting_radius(long light_index, float radius)
{
	int i;
	if (!lighting_config_boolean("display.muzzle_lighting"))
		return radius;
	for (i = 0; i < MUZZLE_LIGHTS; i++)
		if (pulses[i].active && pulses[i].light == light_index)
			return MAX(radius, pulses[i].radius);
	return radius;
}

int muzzle_lighting_vehicle_occlusion(long light)
{
	int i;
	if (light == NONE || !lighting_config_boolean("display.muzzle_lighting"))
		return FALSE;
	for (i = 0; i < MUZZLE_LIGHTS; i++)
		if (pulses[i].active && pulses[i].light == light &&
			muzzle_profiles[pulses[i].profile].vehicle_occlusion)
			return TRUE;
	return FALSE;
}
