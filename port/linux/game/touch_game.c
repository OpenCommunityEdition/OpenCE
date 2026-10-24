/*
TOUCH_GAME.C

What the touch controls (port/linux/src/touch_input.c) need to know of the
game. The platform layer cannot see the game's types, so it asks here
(this file is compiled as the game's own sources are).
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "cutscene/cinematics.h"
#include "input/input.h"
#include "input/input_abstraction.h"
#include "interface/ui_widget.h"
#include "game/players.h"
#include "game/local_players.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "interface/weapon_hud_interface_definition.h"
#include "interface/hud_definitions.h"
#include "tag_files/tag_files.h"
#include <string.h>

/* asks whether a cinematic is playing that A would skip
(player_control.c, player_control_action_test_check_reset_input_blob); tests
cinematic_globals first because the controller is read before the game exists
during input_initialize and shell_initialize, and game_initialize makes it
after the game time globals that game_in_progress reads (game.c) */
int touch_game_cinematic_skippable(void)
{
	return cinematic_globals && game_in_progress() && cinematic_in_progress() &&
		cinematic_can_be_skipped();
}

/* asks whether a cinematic is playing, skippable or not: the player has no
controls, so the on-screen touch controls hide (touch_input.c) */
int touch_game_cinematic_playing(void)
{
	return cinematic_globals && game_in_progress() && cinematic_in_progress();
}

/* asks whether a game is being played: not while the game starts, nor
between maps, nor at the main menu (whose ui.map is a game too), where the
on-screen touch controls do not show (touch_input.c); cinematic_globals
first, as touch_game_cinematic_skippable has it */
int touch_game_playing(void)
{
	return cinematic_globals && game_in_progress() && !main_menu_is_active();
}

/* Android-only HUD cue. Reuse Halo's own resolved nearby interaction;
 * it already checks seat availability, player velocity and vehicle orientation.
 * Do not infer enterability from distance alone. */
int touch_game_vehicle_enter_available(void)
{
    long unit_index;
    struct unit_datum *unit;
    struct player_datum *player;
    struct object_datum *target;

    if (!touch_game_playing()) return 0;
    unit_index = player_control_get_unit_index(0);
    if (unit_index == NONE) return 0;
    unit = unit_get(unit_index);
    if (!unit || unit->object.parent_object_index != NONE ||
        unit->unit.player_index == NONE) return 0;
    player = player_try_and_get(unit->unit.player_index);
    if (!player || player->unit_index != unit_index) return 0;
    if (player->action_result != _player_action_result_enter_vehicle &&
        player->action_result != _player_action_result_evict_from_vehicle) return 0;
    if (player->action_object_index == NONE) return 0;

    /* VEHICLE_PICKUP_GUARD_V10: ENTER is permitted only for an actual
     * vehicle object; a dropped weapon or other X-action cannot show wheel. */
    target = object_try_and_get(player->action_object_index);
    return target && target->object.type == _object_type_vehicle;
}

/* VEHICLE_WEAPON_PICKUP_PRIORITY_V11: the on-foot Hold-X weapon/equipment
 * prompt takes priority over ANY vehicle-HUD state. An X prompt does not
 * mean that a vehicle is available or that the player is in a vehicle. */
int touch_game_ground_item_prompt(void)
{
    long unit_index;
    struct unit_datum *unit;
    struct player_datum *player;

    if (!touch_game_playing()) return 0;
    unit_index = player_control_get_unit_index(0);
    if (unit_index == NONE) return 0;
    unit = unit_get(unit_index);
    if (!unit || unit->unit.player_index == NONE) return 0;
    player = player_try_and_get(unit->unit.player_index);
    if (!player || player->unit_index != unit_index) return 0;

    switch (player->action_result)
    {
    case _player_action_result_pickup_powerup:
    case _player_action_result_pickup_weapon:
    case _player_action_result_swap_for_grenades:
    case _player_action_result_swap_for_powerup:
    case _player_action_result_swap_for_weapon:
    case _player_action_result_add_weapon_to_inventory:
        return 1;
    default:
        return 0;
    }
}

/* VEHICLE_OCCUPIED_V8: the player has entered a vehicle, even when
 * an unusual/modded seat tag cannot be classified as driver/gunner/passenger.
 * This is deliberately independent from touch_game_vehicle_hud_state(). */
/* VEHICLE_FLIP_RELOAD_PROMPT_V18: Halo's distinct "Press X to flip"
 * action. Only the visual X replacement is affected; gameplay remains native. */
int touch_game_vehicle_flip_available(void)
{
    long unit_index;
    struct unit_datum *unit;
    struct player_datum *player;
    struct object_datum *target;

    if (!touch_game_playing()) return 0;
    unit_index = player_control_get_unit_index(0);
    if (unit_index == NONE) return 0;
    unit = unit_get(unit_index);
    if (!unit || unit->object.parent_object_index != NONE ||
        unit->unit.player_index == NONE) return 0;
    player = player_try_and_get(unit->unit.player_index);
    if (!player || player->unit_index != unit_index ||
        player->action_result != _player_action_result_flip_vehicle ||
        player->action_object_index == NONE) return 0;
    target = object_try_and_get(player->action_object_index);
    return target && target->object.type == _object_type_vehicle;
}

int touch_game_vehicle_occupied(void)
{
    long unit_index;
    struct unit_datum *unit;
    struct object_datum *parent;
    struct unit_datum *vehicle;
    struct unit_definition *definition;
    struct player_datum *player;

    if (!touch_game_playing()) return 0;
    unit_index = player_control_get_unit_index(0);
    if (unit_index == NONE) return 0;
    unit = unit_get(unit_index);
    if (!unit || unit->unit.player_index == NONE ||
        unit->object.parent_object_index == NONE ||
        unit->unit.parent_seat_index == NONE ||
        unit->unit.parent_seat_index < 0) return 0;
    player = player_try_and_get(unit->unit.player_index);
    if (!player || player->unit_index != unit_index) return 0;

    /* VEHICLE_PICKUP_GUARD_V10: attached-to-something is not seated-in-car.
     * Use the actual seat AND check the parent object's type. */
    parent = object_try_and_get(unit->object.parent_object_index);
    if (!parent || parent->object.type != _object_type_vehicle) return 0;
    vehicle = unit_get(unit->object.parent_object_index);
    if (!vehicle) return 0;
    definition = unit_definition_get(vehicle->definition_index);
    return definition && unit->unit.parent_seat_index < definition->unit.seats.count;
}

/* VEHICLE_GHOST_SECONDARY_V12: hide the non-existent secondary trigger
 * for stock Ghosts. Use the seated vehicle's own tag name, not the current
 * pickup prompt or any nearby unrelated object. Other vehicles untouched. */
int touch_game_vehicle_is_ghost(void)
{
    long player_unit_index;
    struct unit_datum *player_unit;
    struct unit_datum *vehicle;
    const char *name;

    if (!touch_game_vehicle_occupied()) return 0;
    player_unit_index = player_control_get_unit_index(0);
    if (player_unit_index == NONE) return 0;
    player_unit = unit_get(player_unit_index);
    if (!player_unit || player_unit->object.parent_object_index == NONE) return 0;
    vehicle = unit_get(player_unit->object.parent_object_index);
    if (!vehicle) return 0;
    name = tag_get_name(vehicle->definition_index);
    return name && strstr(name, "ghost") != NULL;
}

/* Android modern HUD: report actual occupied vehicle seat and its capabilities.
 * 0 on foot; 1 driver; 2 gunner; 3 passenger; bit 2 as used by v4
 * for passengers; bit 3 when the DRIVER is also the vehicle GUNNER.
 * Driver gunner is derived from Halo's seat flags/current gunner unit. */
int touch_game_vehicle_hud_state(void)
{
    long unit_index;
    struct unit_datum *unit;
    struct unit_datum *vehicle;
    struct unit_definition *definition;
    struct unit_seat *seat;
    short seat_index;
    struct player_datum *player;

    if (!touch_game_playing()) return 0;
    unit_index = player_control_get_unit_index(0);
    if (unit_index == NONE) return 0;
    unit = unit_get(unit_index);
    if (!unit || unit->unit.player_index == NONE) return 0;
    player = player_try_and_get(unit->unit.player_index);
    if (!player || player->unit_index != unit_index) return 0;
    seat_index = unit->unit.parent_seat_index;
    if (unit->object.parent_object_index == NONE || seat_index == NONE ||
        !touch_game_vehicle_occupied()) return 0;

    vehicle = unit_get(unit->object.parent_object_index);
    if (!vehicle) return 0;
    definition = unit_definition_get(vehicle->definition_index);
    if (!definition || seat_index < 0 || seat_index >= definition->unit.seats.count)
        return 0;
    seat = TAG_BLOCK_GET_ELEMENT(&definition->unit.seats, seat_index, struct unit_seat);
    if (!seat) return 0;

    if (unit_seat_is_driver(unit->object.parent_object_index, seat_index))
    {
        /* On a Ghost, Banshee, Scorpion, etc. the DRIVER may also be
         * the vehicle GUNNER. Detect the engine's actual gunner, or the
         * seat's gunner flag. Warthog drivers without guns stay unarmed.
         * Bit 3 of our seat-state (-> bit 7 in slots[9]) means armed pilot. */
        int armed_driver = unit_seat_is_gunner(unit->object.parent_object_index, seat_index) ||
            vehicle->unit.gunner_object_index == unit_index;
        return 1 | (armed_driver ? 8 : 0);
    }
    if (unit_seat_is_gunner(unit->object.parent_object_index, seat_index)) return 2;
    return 3 | (TEST_FLAG(seat->flags, 5) ? 4 : 0);
}

/* (touch_input.c's TOUCH_BUTTONS and the overlay count 16 buttons) */
typedef char verify_touch_buttons[NUMBER_OF_GAMEPAD_BUTTONS == 16 ? 1 : -1];

/* the game control on each of player 1's 16 controller buttons, as the
gamepad's buttons are numbered (input.h), -1 for none: the touch controls
name their buttons by them, so a profile's own mapping shows */
void touch_game_button_controls(int *controls)
{
	short buttons[NUMBER_OF_GAMEPAD_BUTTONS];
	short index;

	input_abstraction_port_button_controls(0, buttons);
	for (index = 0; index < NUMBER_OF_GAMEPAD_BUTTONS; index++)
	{
		controls[index] = buttons[index];
	}
}

/* Compact type IDs, transmitted as three integers (two slots, active slot).
 * The native engine decides the icon identity from the equipped weapon tag;
 * no guesses about which gun is actually held on Android. */
static int touch_weapon_icon(const char *name)
{
    if (!name) return 0;
    if (strstr(name, "assault rifle")) return 1;
    if (strstr(name, "pistol") && !strstr(name, "plasma")) return 2;
    if (strstr(name, "plasma pistol")) return 3;
    if (strstr(name, "plasma rifle")) return 4;
    if (strstr(name, "needler")) return 5;
    if (strstr(name, "shotgun")) return 6;
    if (strstr(name, "sniper rifle")) return 7;
    if (strstr(name, "rocket launcher")) return 8;
    if (strstr(name, "flamethrower")) return 9;
    if (strstr(name, "fuel rod")) return 10;
    return 11; /* unknown/modded weapon: neutral rifle silhouette */
}

void touch_game_weapon_slots(int slots[16], int requested)
{
    long unit_index;
    long player_index;
    struct player_datum *player;
    struct unit_datum *unit;
    int i;
    for (i = 0; i < 16; i++) slots[i] = -1;
    /* Extra weapon occupancy mask: bit 0 = third slot, bit 1 = fourth.
     * Bit 8 tells the Android modern HUD that Halo is showing a weapon
     * pickup/swap prompt, so it can replace the Xbox X glyph with the
     * touch reload pictogram without changing the localized message. */
    slots[9] = 0;
    if (!touch_game_playing()) return;
    player_index = local_player_get_player_index(0);
    player = player_index == NONE ? NULL : player_get(player_index);
    if (player && (player->action_result == _player_action_result_pickup_weapon ||
        player->action_result == _player_action_result_swap_for_weapon ||
        player->action_result == _player_action_result_add_weapon_to_inventory))
    {
        slots[9] |= 1 << 8;
    }
    unit_index = player_control_get_unit_index(0);
    if (unit_index == NONE) return;
    unit = unit_get(unit_index);
    if (!unit) return;
    for (i = 0; i < 2; i++)
    {
        long weapon_index = unit_inventory_get_weapon(unit_index, (short)i);
        if (weapon_index != NONE)
        {
            struct weapon_datum *weapon = weapon_try_and_get(weapon_index);
            if (weapon)
            {
                struct weapon_definition *definition = weapon_definition_get(weapon->definition_index);
                long hud_index = definition->weapon.interface_definition.hud_interface.index;
                slots[i] = touch_weapon_icon(tag_get_name(weapon->definition_index));
                if (hud_index != NONE)
                {
                    /* Original messaging icon metadata, rather than an invented sprite ID.
                     * The bitmap itself belongs to hud_globals->messaging.messaging_icons.
                     * slots[3..4]: sequence, [5..6]: original RGBA packed color,
                     * [7..8]: original icon flags. */
                    const struct weapon_hud_interface_definition *hud =
                        (const struct weapon_hud_interface_definition *)tag_get('wphi', hud_index);
                    if (hud && hud->messaging_icon.sequence_index != NONE)
                    {
                        slots[3 + i] = hud->messaging_icon.sequence_index;
                        slots[5 + i] = (int)hud->messaging_icon.color;
                        slots[7 + i] = (int)hud->messaging_icon.flags;
                    }
                }
            }
        }
    }
    /* Respawn profiles and scripts can fill inventory slots 2 and 3 even
     * when ordinary pickup is limited to two weapons. */
    for (i = 2; i < 4; i++)
    {
        long extra_index = unit_inventory_get_weapon(unit_index, (short)i);
        if (extra_index != NONE)
        {
            struct weapon_datum *extra = weapon_try_and_get(extra_index);
            if (extra)
            {
                /* An inventory slot is occupied even if its HUD tag has no icon. */
                slots[9] |= 1 << (i - 2);
                struct weapon_definition *definition = weapon_definition_get(extra->definition_index);
                if (definition)
                {
                    long hud_index = definition->weapon.interface_definition.hud_interface.index;
                    if (hud_index != NONE)
                    {
                        const struct weapon_hud_interface_definition *hud =
                            (const struct weapon_hud_interface_definition *)tag_get('wphi', hud_index);
                        if (hud && hud->messaging_icon.sequence_index != NONE)
                        {
                            slots[10 + i - 2] = hud->messaging_icon.sequence_index;
                            slots[12 + i - 2] = (int)hud->messaging_icon.color;
                            slots[14 + i - 2] = (int)hud->messaging_icon.flags;
                        }
                    }
                }
            }
        }
    }
    if (unit->unit.current_weapon_index >= 0 && unit->unit.current_weapon_index < 4)
        slots[2] = unit->unit.current_weapon_index;
    if (requested >= 0 && requested < 4 &&
        (requested < 2 ? slots[requested] >= 0 :
            (slots[9] & (1 << (requested - 2))) != 0) &&
        unit->unit.current_weapon_index != requested)
        player_control_set_desired_weapon(unit_index, (short)requested);
}
