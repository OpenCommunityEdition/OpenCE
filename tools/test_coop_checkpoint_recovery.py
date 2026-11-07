"""A same-handle corpse through real replica creation and player binding.

The default-profile expected failure records an unresolved defect; the optional
profile is a positive control, not a complete checkpoint-compatibility claim.
"""
import re

import pytest

from harness import build, constant, enum_with, function, mutated, read, run, structure


class KnownDeadReplica(Exception):
    """Only the demonstrated default-profile dead/unbound result."""


class KnownLateKill(Exception):
    """An accepted old-life kill can affect a recreated same-handle unit."""


def define_value(source, name):
    """Keep a simple source constant's expression, including its dependencies."""
    match = re.search(r"^#define\s+" + re.escape(name) + r"\s+([^\n]+)", source, re.M)
    if not match:
        match = re.search(r"^\s*" + re.escape(name) + r"\s*=\s*([^,\n]+),", source, re.M)
    if not match:
        raise LookupError(f"constant declaration not found: {name}")
    return f"#define {name} ({match.group(1)})"


def generated(fault=None):
    objects = read("port/linux/game/network_objects.c")
    distributed = read("port/linux/game/network_distributed.c")
    data = read("source/memory/data.c")
    players = read("source/game/players.c")
    units = read("source/units/units.c")
    damage = read("port/linux/game/network_damage.c")
    distributed_h = read("port/linux/game/network_distributed.h")
    units_h = read("source/units/units.h")
    objects_h = read("source/objects/objects.h")
    damage_h = read("source/objects/damage.h")
    config = [f"#define TICKS_PER_SECOND {constant(read('source/cseries/cseries.h'), 'TICKS_PER_SECOND')}",
              f"#define TAG_STRING_LENGTH {constant(read('source/tag_files/tag_files.h'), 'TAG_STRING_LENGTH')}",
              f"#define MAXIMUM_WEAPONS_PER_UNIT {constant(units_h, 'MAXIMUM_WEAPONS_PER_UNIT')}",
              f"#define MAXIMUM_REGIONS_PER_OBJECT {constant(read('source/objects/object_definitions.h'), 'MAXIMUM_REGIONS_PER_OBJECT')}"]
    for source, names in (
        (objects, ("MAXIMUM_CLIENT_RETRIED_INVENTORIES", "INVENTORY_INTERVAL_TICKS", "INVENTORY_REFRESH_TICKS",
                   "EMPTY_INVENTORY_TICKS", "OWN_INVENTORY_MARGIN_TICKS", "MAXIMUM_INVENTORY_GRENADES",
                   "DEFAULT_OWN_ROUND_TRIP_TICKS", "MAXIMUM_OWN_ROUND_TRIP_TICKS", "AGE_SCALE",
                   "OBJECT_WORLD_BOUND", "HOST_OBJECTS_RESEND_TICKS", "CLIENT_RETRY_TICKS",
                   "CLIENT_RETRY_MAXIMUM_TICKS", "CTF_FLAG_TEAMS", "ODDBALL_TEAMS", "NETWORKED_OBJECT_TYPES")),
        (distributed, ("DEATH_BLOW_WAIT_TICKS", "SEAT_DISAGREEMENT_TICKS", "SEAT_DISAGREEMENT_SLACK_TICKS")),
        (distributed_h, ("REMOTE_CORRECTION_TOLERANCE", "REMOTE_BLEND_DISTANCE", "NO_PLAYER")),
        (damage, ("REPORT_WORLD_BOUND", "REPORT_IMPACT_TOLERANCE")),
    ):
        config.extend(define_value(source, name) for name in names)
    types = "\n".join([
        enum_with(read("source/objects/object_types.h"), "_object_mask_all ="),
        enum_with(objects_h, "_object_dead_bit,"),
        enum_with(objects_h, "_object_connected_to_map_bit,"),
        enum_with(objects_h, "NUMBER_OF_OBJECT_CHANGE_COLORS,"),
        enum_with(units_h, "_unit_active_camouflaged_bit,"),
        enum_with(units_h, "_unit_grenade_throw_idle ="),
        enum_with(units_h, "NUMBER_OF_UNIT_GRENADE_TYPES\n"),
        enum_with(read("source/items/weapons.h"), "_magazine_reloading,"),
        enum_with(read("source/game/players.h"), "NUMBER_OF_PLAYER_POWERUPS,"),
        enum_with(damage_h, "NUMBER_OF_DAMAGE_DATA_FLAGS,"),
        enum_with(damage_h, "_object_being_damaged_body_depleted_bit ="),
        enum_with(read("source/game/game_globals.h"), "NUMBER_OF_MATERIAL_TYPES,"),
        structure(read("source/memory/data.h"), "data_array"),
        structure(read("source/memory/data.h"), "datum_header"),
        structure(read("source/memory/data.h"), "data_iterator"),
        enum_with(objects, "_object_change_create,"),
        enum_with(objects, "_distributed_object_carried_bit ="),
        enum_with(distributed, "_distributed_unit_alive_bit ="),
        enum_with(distributed, "_distributed_unit_camouflaged_bit ="),
        structure(distributed_h, "distributed_vector"),
        structure(objects, "distributed_object_change"),
        structure(objects, "distributed_inventory"),
        structure(objects.replace("static struct", "struct"), "distributed_own_inventory"),
        structure(objects.replace("static struct", "struct"), "distributed_retried_inventory"),
        structure(distributed, "distributed_unit_state"),
        enum_with(damage, "_damage_event_player_effect,"),
        enum_with(damage, "_damage_event_friendly_fire_bit ="),
        structure(damage, "distributed_damage"),
        structure(damage, "distributed_damage_event"),
    ])
    data_code = "\n".join(function(data, name) for name in (
        "data_initialize", "data_verify", "data_usable", "datum_initialize",
        "datum_try_and_get", "datum_get", "datum_new_at_index", "datum_delete",
        "data_iterator_new", "data_iterator_next"))
    code = "\n".join(function(distributed, name) for name in (
        "distributed_object_index_valid", "distributed_real_valid", "distributed_point_valid",
        "distributed_axes_make_valid", "distributed_player", "distributed_player_from_byte",
        "distributed_player_is_local", "distributed_living_unit"))
    code += "\n" + "\n".join(function(objects, name) for name in (
        "distributed_vector_valid", "distributed_transform_valid", "network_objects_client_has"))
    code += "\n" + "\n".join(function(players, name) for name in (
        "network_player_detach_unit", "network_player_attach_unit"))
    code += "\n" + "\n".join(function(units, name) for name in (
        "unit_kill", "unit_kill_silent", "unit_kill_no_statistics",
        "unit_network_forget_weapon", "unit_network_add_weapon", "unit_network_drop_weapon"))
    code += "\n" + "\n".join(function(objects, name) for name in (
        "distributed_client_release_item", "distributed_client_delete",
        "distributed_client_request_objects", "distributed_client_create_failed",
        "network_objects_expect_live_unit", "distributed_client_own_object",
        "distributed_client_change_valid", "distributed_client_apply_change",
        "distributed_client_reuses_object", "distributed_client_create",
        "network_objects_handle_changes", "network_objects_handle_synchronized"))
    code += "\n" + function(distributed, "distributed_handle_unit_state")
    code += "\n" + "\n".join(function(objects, name) for name in (
        "distributed_client_note_own_inventory", "distributed_client_own_settled",
        "distributed_client_weapon_here", "distributed_client_inventory_whole",
        "distributed_client_retry_inventory", "distributed_client_forget_retried_inventory",
        "distributed_client_apply_inventory", "distributed_client_apply_retried_inventories"))
    code += "\n" + "\n".join(function(damage, name) for name in (
        "distributed_damage_to_data", "distributed_damage_indices_valid",
        "distributed_damage_numbers_valid", "network_damage_handle_events"))
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", "\n".join(config)), ("types.inc", types), ("data.inc", data_code),
            ("engine.inc", function(read("source/objects/objects.c"), "object_delete_recursive")),
            ("under_test.inc", code))


def test_optional_profile_reconstructs_and_rebinds():
    status, output = run(build("coop_checkpoint_recovery", generated()), "profile")
    assert status == 0, output


@pytest.mark.xfail(strict=True, raises=KnownDeadReplica,
                   reason="OpenCE Crossplay reuses a same-handle dead replica; Milestone10 remains open")
def test_default_profile_recovers_authoritative_live_unit():
    status, output = run(build("coop_checkpoint_recovery", generated()), "default")
    if status == 41 and output == "known defect: validated live create leaves same-handle corpse unbound":
        raise KnownDeadReplica(output)
    assert status == 0, output


@pytest.mark.xfail(strict=True, raises=KnownLateKill,
                   reason="Same-handle reconstruction has no prior-life kill fence; direct-handler proof only")
def test_reconstructed_unit_survives_accepted_old_life_kill():
    status, output = run(build("coop_checkpoint_recovery", generated()), "late-kill")
    if status == 42 and output == "known limitation: accepted old-life kill affects reconstructed same-handle unit":
        raise KnownLateKill(output)
    assert status == 0, output


@pytest.mark.parametrize("case", [
    "live", "host-dead", "host-kills-live", "bound-corpse", "different-handle", "unknown", "unknown-player", "stale-delete",
    "invalid-tag", "invalid-team", "invalid-color", "invalid-position", "invalid-axes",
    "invalid-identifier", "outside-pool", "allocation-failure", "repeat", "retry",
    "inventory", "hierarchy", "late-inventory",
    "kill-current", "kill-unknown", "kill-invalid-number", "kill-invalid-tag", "kill-invalid-node",
])
def test_replica_controls(case):
    status, output = run(build("coop_checkpoint_recovery", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("before,after,case", [
    ("!distributed_client_change_valid(change, &forward, &up)",
     "(forward=change->forward, up=change->up, FALSE)", "invalid-tag"),
    ("objects_client_has[absolute_index] == change->object_index)", "TRUE)", "stale-delete"),
    ("network_player_attach_unit(player_index, state->unit_index);", "(void)player_index;", "profile"),
    ("unit_network_drop_weapon(object_index, weapon_slot);", "(void)weapon_slot;", "hierarchy"),
    ("unit_exit_seat_end(child_index);", "(void)child_index;", "hierarchy"),
    ("distributed_client_apply_inventory(&inventory);", "(void)inventory;", "late-inventory"),
    ("!distributed_damage_numbers_valid(&event->damage)", "FALSE", "kill-invalid-number"),
    ("damage_replay_kill(event->object_index, &damage, event->node_index, event->region_index,\n"
     "\t\t\t\t\tevent->material_index);", "(void)victim_player_index;", "kill-current"),
])
def test_replica_mutations(before, after, case):
    status, output = run(build("coop_checkpoint_recovery", generated((before, after))), case)
    assert status == 1, output
