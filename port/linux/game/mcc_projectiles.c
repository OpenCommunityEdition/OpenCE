/* MCC compatibility for camera-relative player rays. Animation bounds pose the body;
 * they must not turn these shots away from the player's requested view. */
#include "cseries.h"
#include "mcc_projectiles.h"
#include "mcc_cache.h"
#include "mcc_camera.h"
#include "game/aim_assist.h"
#include "game/players.h"
#include "game/game.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "physics/collisions.h"
#include "units/units.h"
#include "units/unit_definitions.h"

/* H1A trigger bit 14 opts back into unit_adjust_projectile_ray's original
 * projected world-muzzle origin (c20's weapon format reference). */
#define MCC_TRIGGER_ORIGINAL_PROJECTILE_RAY 0x4000UL

boolean mcc_projectile_ray(long weapon_index, long owner, short trigger_index, boolean firing,
    real_point3d *origin, real_vector3d *forward, real *velocity, long *target_index)
{
    struct weapon_datum *weapon;
    struct weapon_definition *definition;
    struct weapon_trigger_definition *trigger;
    struct unit_datum *unit;
    struct player_datum *player;
    long target;
    real_point3d camera, spawn, aim_point, assisted_point;
    real_vector3d aim, shot, right, up, ray, assisted_direction, motion;
    struct collision_result collision;
    real assist_level, distance;

    if (!mcc_cache_tags_loaded()) return FALSE;
    weapon = weapon_try_and_get(weapon_index);
    if (!weapon) return FALSE;
    definition = weapon_definition_get(weapon->definition_index);
    if (trigger_index < 0 || trigger_index >= definition->weapon.triggers.count)
        return FALSE;
    trigger = TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, trigger_index,
        struct weapon_trigger_definition);
    if (trigger->flags & (MCC_TRIGGER_ORIGINAL_PROJECTILE_RAY |
        FLAG(_weapon_trigger_uses_weapon_origin_bit) |
        FLAG(_weapon_trigger_projectiles_cannot_be_aimed_bit))) return FALSE;
    unit = owner != NONE ? unit_try_and_get(owner) : NULL;
    if (!unit || unit->unit.player_index == NONE ||
        unit->object.parent_object_index != NONE || unit->unit.gunner_object_index != NONE ||
        TEST_FLAG(unit->object.damage_flags, _object_dead_bit) ||
        !TEST_FLAG(unit_definition_get(unit->definition_index)->unit.flags, _unit_fires_from_camera_bit) ||
        mcc_camera_weapon_following(owner)) return FALSE;

    aim = unit->unit.desired_aiming_vector;
    if (normalize3d(&aim) == 0.0f) return FALSE;
    unit_get_camera_position(owner, &camera);
    cross_product3d(global_up3d, &aim, &right);
    if (normalize3d(&right) == 0.0f) right = *global_left3d;
    cross_product3d(&aim, &right, &up);
    normalize3d(&up);
    point_from_line3d(&camera, &aim, trigger->first_person_weapon_offset.x, &spawn);
    point_from_line3d(&spawn, &right, trigger->first_person_weapon_offset.y, &spawn);
    point_from_line3d(&spawn, &up, trigger->first_person_weapon_offset.z, &spawn);

    /* Keep the spawn on the camera's side of cover. Ignore the shooter just
     * as projectile flight does; do not let its body pull the ray to the eye.
     * Validate before creating attachments, which emit their first point in
     * object_new, rather than translating an already visible trail. */
    vector_from_points3d(&camera, &spawn, &ray);
    distance = magnitude3d(&ray);
    if (distance > 0.0f && collision_test_vector(_collision_test_for_projectiles_flags,
        &camera, &ray, owner, &collision))
        point_from_line3d(&camera, &ray, MAX(0.0f, collision.t - 0.001f / distance), &spawn);

    /* Apply authored aim assistance in camera space. Parallax from the
     * offset origin is separate from the autoaim cone, even at zero autoaim. */
    shot = aim;
    target = player_get_aim_assist_target(unit->unit.player_index, &camera, &aim,
        &assisted_point, &assist_level);
    if (target != NONE && assist_level > 0.0f)
    {
        vector_from_points3d(&camera, &assisted_point, &assisted_direction);
        if (normalize3d(&assisted_direction) > 0.0f)
        {
            fast_normals_interpolate(&aim, &assisted_direction, assist_level, &shot);
            normalize3d(&shot);
        }
    }
    scale_vector3d(&shot, 128.0f, &ray);
    point_from_line3d(&camera, &ray, 1.0f, &aim_point);
    if (collision_test_vector(_collision_test_for_projectiles_flags, &camera, &ray, owner, &collision))
        aim_point = collision.point;
    vector_from_points3d(&spawn, &aim_point, &ray);
    /* Cover or an authored forward offset can put the target behind the
     * spawn. Never reverse a projectile back towards its shooter. */
    if (dot_product3d(&ray, &shot) > 0.0f && normalize3d(&ray) > 0.0f)
        shot = ray;
    object_get_velocities(owner, &motion, NULL);
    *velocity = dot_product3d(&aim, &motion);
    *origin = spawn;
    *forward = shot;
    *target_index = target;
    if (firing)
    {
        player = player_get(unit->unit.player_index);
        player->aim_assist_unit_index = target;
        player->aim_assist_timestamp = game_time_get();
    }
    return TRUE;
}
