/* MCC bipeds can carry authored seats. Keep their interaction separate from
   vehicle flipping, prediction and the Xbox/CE interaction rules. */
#include "cseries.h"
#include "game/game.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "mcc_cache.h"
#include "mcc_seats.h"

struct unit_datum *mcc_seat_parent(long object_index)
{
    struct unit_datum *unit;
    struct unit_definition *definition;
    if (!mcc_cache_tags_loaded()) return NULL;
    unit = (struct unit_datum *)object_try_and_get_and_verify_type(object_index, _object_mask_biped);
    if (!unit) return NULL;
    definition = unit_definition_get(unit->definition_index);
    return definition->unit.seats.count > 0 && definition->unit.seats.count <= 16 ? unit : NULL;
}

boolean mcc_seat_nearby(long player_index, long target_index, short *seat_index)
{
    struct player_datum *player;
    struct unit_datum *rider, *parent;
    long ancestor;
    short depth;
    *seat_index = NONE;
    parent = mcc_seat_parent(target_index);
    player = parent ? player_try_and_get(player_index) : NULL;
    rider = player ? (struct unit_datum *)object_try_and_get_and_verify_type(player->unit_index, _object_mask_biped) : NULL;
    if (!rider || player->unit_index == target_index || rider->object.parent_object_index != NONE ||
        TEST_FLAG(rider->object.damage_flags, _object_dead_bit) ||
        TEST_FLAG(parent->object.damage_flags, _object_dead_bit) || unit_overcharged(player->unit_index)) return FALSE;
    ancestor = target_index;
    for (depth = 0; ancestor != NONE; ++depth) {
        struct object_datum *object;
        if (depth >= 16 || ancestor == player->unit_index) return FALSE;
        object = object_try_and_get(ancestor);
        if (!object) return FALSE;
        ancestor = object->object.parent_object_index;
    }
    return unit_find_nearby_seat(player->unit_index, target_index, seat_index) == _unit_nearby_seat_available;
}

boolean mcc_seat_action(long player_index, long target_index, short seat_index)
{
    short nearby;
    if (!mcc_seat_parent(target_index)) return FALSE;
    /* Clients propose the ordinary action button. Only the host boards a
       biped; no client-supplied position or seat becomes authoritative. */
    if (game_connection() != _game_connection_network_client &&
        mcc_seat_nearby(player_index, target_index, &nearby) && nearby == seat_index)
        unit_enter_seat(player_get(player_index)->unit_index, target_index, seat_index);
    return TRUE;
}

boolean mcc_seat_update(long unit_index)
{
    struct unit_datum *rider, *parent;
    if (!mcc_cache_tags_loaded()) return FALSE;
    rider = (struct unit_datum *)object_try_and_get_and_verify_type(unit_index, _object_mask_biped);
    parent = rider ? mcc_seat_parent(rider->object.parent_object_index) : NULL;
    if (!parent || rider->unit.parent_seat_index < 0 ||
        rider->unit.parent_seat_index >= unit_definition_get(parent->definition_index)->unit.seats.count) return FALSE;
    /* A real seat is not an infection form clinging to a victim. The
       ordinary seat animation must finish, including a scripted exit. */
    if (game_connection() != _game_connection_network_client) {
        if (TEST_FLAG(parent->object.damage_flags, _object_dead_bit))
            unit_exit_seat_end(unit_index);
        else if (TEST_FLAG(rider->unit.control_flags, _unit_control_action_bit))
            unit_try_and_exit_seat(unit_index);
    }
    return TRUE;
}

void mcc_seat_camera(struct player_control_unit_camera_info *info)
{
    struct unit_datum *rider, *parent;
    struct unit_definition *definition;
    struct unit_seat *seat;
    if (!mcc_cache_tags_loaded() || info->seat_index != NONE) return;
    rider = (struct unit_datum *)object_try_and_get_and_verify_type(info->unit_index, _object_mask_unit);
    parent = rider ? mcc_seat_parent(rider->object.parent_object_index) : NULL;
    if (!parent) return;
    definition = unit_definition_get(parent->definition_index);
    if (rider->unit.parent_seat_index < 0 || rider->unit.parent_seat_index >= definition->unit.seats.count) return;
    seat = TAG_BLOCK_GET_ELEMENT(&definition->unit.seats, rider->unit.parent_seat_index, struct unit_seat);
    info->unit_index = rider->object.parent_object_index;
    info->seat_index = rider->unit.parent_seat_index;
    info->camera = &seat->camera;
}
