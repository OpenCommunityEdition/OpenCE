"""The host validates a client's deck word in its own vehicle's body frame."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure
from test_vehicle_riding import engine, without_includes

NETWORK = "port/linux/game/network_distributed.c"
RIDING = "port/linux/game/vehicle_riding.c"
CASES = ["host-deck-lagging", "host-deck-relative-speed", "host-deck-advance-refused",
         "host-deck-carried-anchor", "host-deck-vehicle-gone", "host-deck-after-vehicle-correction",
         "host-pass-partition", "host-deck-node-equivalent", "host-deck-node-forgery",
         "host-deck-accelerating", "apply-velocity-within-tolerance", "host-deck-temporaries",
         "host-path-rules", "host-velocity-limits", "host-pass-128", "host-world-new-life",
         "host-deck-anchor-walk", "host-now-once", "host-deck-host-moved", "host-deck-part-alternating",
         "host-deck-window-switch", "host-deck-jump-descending",
          "host-deck-jump-rising", "host-deck-hover-descending", "host-deck-hover-stopped", "host-deck-window-ceiling",
          "host-along-rising", "host-along-every-axis", "host-world-client-rides", "host-air-bound",
           "host-leash", "host-leash-toggle", "host-air-throw", "host-riding-off", "host-transition-128", "host-transition-no-word",
           "host-along-air-part", "host-along-air-part-place", "host-along-air-part-limits", "host-world-leash-velocity", "host-along-jump-takeoff",
           "host-anchor-omitted-parts", "host-anchor-named-resets", "host-forged", "host-parked-rub", "host-push-not-made",
           "host-window-braking-climb", "client-return-no-allowance",
         "host-physics-board-rise-agreed", "host-step-off-together-agreed", "host-physics-board-agreed",
         "host-physics-board-client-air-arms", "host-client-lands-in-host-air", "host-client-lands-ends-running-leash", "host-client-lands-other-vehicle-arms",
         "host-client-lands-host-riding-arms", "host-transition-real-drop", "host-step-off-other-vehicle-arms",
         "host-board-world-word-stale-arms", "host-board-ack-late-arms", "host-board-ack-then-jump-arms",
         "host-board-ack-other-vehicle-arms", "host-flip-after-leash-write", "host-agreement-touches-transition-only",
         "host-leash-world-write-stamped",
         "host-along-every-axis-air-part", "host-leash-keeps-support", "host-along-world-word-no-lift", "host-leash-world-word-no-lift",
         "host-leash-world-word-lowers", "host-deck-word-other-vehicle-riding-follows",
         "host-deck-word-other-vehicle-air-no-lift",
         "host-landing-deck-take-exact",
         "host-landing-after-leash-exact",
         "host-landing-after-along-exact",
         "host-landing-second-deck-take-blends",
         "host-deck-take-steady-blends",
         "host-deck-after-no-part-blends",
         "host-deck-after-other-vehicle-air-blends",
         "host-landing-word-refused",
         "host-landing-beyond-accept-refused",
         "host-landing-record-from-taken-only",
         "host-landing-new-life-blends",
         "host-landing-new-life-after-leash-blends",
         "host-landing-switch-off-unchanged",
         "host-landing-128",
         "host-landing-after-riding-leash-exact", "host-landing-leash-other-word-consumes", "host-landing-riding-leash-consumed-once"]
NETWORK_FUNCTIONS = ("distributed_apply_state", "distributed_riding_transition", "distributed_on_foot_bound", "distributed_on_foot_fall_speed",
                     "distributed_on_foot_ceiling", "distributed_on_foot_move_valid",
                      "distributed_supported_elevator", "distributed_riding_elevator", "distributed_air_part_vehicle", "distributed_take_riding", "distributed_take_prediction",
                     "distributed_prediction_rides", "distributed_apply_predictions")
RIDING_FUNCTIONS = ("riding_snapshot", "vehicle_riding_point_to_local", "vehicle_riding_point_to_world", "vehicle_riding_is_riding",
                     "vehicle_riding_rebase", "vehicle_riding_host_path", "vehicle_riding_host_target", "vehicle_riding_host_leash", "vehicle_riding_host_velocity", "vehicle_riding_host_unlifted",
                    "vehicle_riding_run_jump_speed", "vehicle_riding_deck_vehicle_named",
                     "vehicle_riding_deck_values_valid", "vehicle_riding_deck_valid", "vehicle_riding_air_frame",
                    "vehicle_riding_transition_agreed", "vehicle_riding_word_on_air_deck")

# source, replacements, and the case that must fail a CHECK (never compilation)
NEGATIVE_CONTROLS = {
    "along-rise-flattened": ("riding", [("\t\tif (place.z < host_place.z)\n\t\t\tplace.z = host_place.z;",
        "\t\tplace.z = host_place.z;")], "host-along-jump-takeoff"),
    "client-air-selects-world": ("riding", [("if (vehicle_riding_is_riding(host) && !transition)",
        "if (vehicle_riding_is_riding(host) && !deck_present && !transition)")], "host-along-air-part"),
    "world-leash-clamps-velocity": ("network", [("leash ? 0.f : HOST_BLEND_DISTANCE, leash ? 0.f : bound->speed,\n\t\tleash ? 0.f : distributed_on_foot_fall_speed(bound, position.z)",
        "leash ? 0.f : HOST_BLEND_DISTANCE, bound->speed,\n\t\tdistributed_on_foot_fall_speed(bound, position.z)")], "host-world-leash-velocity"),
    "along-world-height": ("riding", [("if (place.z < host_place.z)\n\t\t\tplace.z = host_place.z;", ";"),
        ("vehicle_riding_point_to_world(&host_node->position, &host_node->forward, &host_node->up, &place, base);",
         "vehicle_riding_point_to_world(&host_node->position, &host_node->forward, &host_node->up, &place, base);\n\t\tbase->z = host_base->z;")], "host-along-every-axis-air-part"),
    "along-air-world-place": ("riding", [("if (client_node && deck->vehicle_index == host_node->object_index && client_node->node_index == host_node->node_index)",
        "if (FALSE)")], "host-along-air-part-place"),
    "along-part-any-vehicle": ("riding", [("if (client_node && deck->vehicle_index == host_node->object_index && client_node->node_index == host_node->node_index)",
        "if (client_node)")], "host-along-air-part-limits"),
    "along-part-any-node": ("riding", [(" && client_node->node_index == host_node->node_index)", ")")], "host-along-air-part-limits"),
    "along-under-deck": ("riding", [("if (place.z < host_place.z)\n\t\t\tplace.z = host_place.z;", ";")], "host-along-rising"),
    "air-bound-unwidened": ("network", [("bound->speed += bound->frame_speed;", ";")], "host-air-bound"),
    "air-bound-drops-throw": ("network", [("bound->speed += bound->frame_speed;",
        "bound->speed = MIN(base_speed, MAXIMUM_PREDICTED_SPEED) + bound->frame_speed;")], "host-air-throw"),
    "leash-client-rearms": ("network", [("client_changed && !accepted->transition_client_spent", "client_changed")], "host-leash-toggle"),
    "leash-client-rate-only": ("network", [("client_changed && !accepted->transition_client_spent",
        "client_changed && (!accepted->transition_client_spent || game_time_get() >= accepted->transition_until)")], "host-leash-toggle"),
    "leash-always": ("network", [("now < accepted.transition_until &&", "TRUE &&")], "host-leash"),
    "leash-widens-anchor": ("network", [("else if (ticks <= 0 || !distributed_on_foot_move_valid(bound, &anchor, &target, ticks))",
        "else if (!leash && (ticks <= 0 || !distributed_on_foot_move_valid(bound, &anchor, &target, ticks)))")], "host-leash"),
    "leash-sets-velocity": ("network", [("\tworld_velocity = object->object.translational_velocity;",
        "\tdistributed_vector_unpack(&state->velocity, DISTRIBUTED_VELOCITY_SCALE, &world_velocity);")], "host-leash"),
    "deck-taken-in-world": ("network", [("target.z -= base.z - object->object.position.z;",
         "target.z -= base.z - object->object.position.z;\n\ttarget = state->position;")], "host-deck-lagging"),
    "bound-world-speed": ("network", [("subtract_vectors3d(velocity, frame_velocity, &relative);",
        "relative = *velocity;")], "host-deck-relative-speed"),
    "anchor-moved-in-world": ("network", [("&taken_host, &host_now, now - accepted.taken_host_time)",
        "&accepted.taken_host_position, &object->object.position, now - accepted.taken_host_time)")],
        "host-deck-carried-anchor"),
    "commit-without-rebase": ("network", [("\t\tbiped_riding_rebase(unit_index);\n\t}", "\t\t;\n\t}")], "host-deck-lagging"),
    "deck-velocity-unclamped": ("riding", [("if (speed > 0.0f)", "if (FALSE)")], "host-deck-advance-refused"),
    "commit-before-checks": ("network", [("	if (!((vehicle_riding_is_riding(&host) ?",
        "	distributed_apply_state(unit_index, state, &target, &world_velocity, 0.0f, HOST_BLEND_DISTANCE, 0.0f, 0.0f);\n\tif (!((vehicle_riding_is_riding(&host) ?")], "host-deck-temporaries"),
    "tolerance-in-world": ("network", [("distance_squared3d(&place, &host_place)",
        "distance_squared3d(&state->position, &object->object.position)")], "host-deck-after-vehicle-correction"),
    "rider-pass-by-valid": ("network", [("distributed_predictions[player_index].state.deck_present;",
        "distributed_predictions[player_index].valid;"),
        ("if (distributed_rider_pass[player_index] != riders)",
         "if (distributed_prediction_rides(player_index, distributed_living_unit(distributed_player(player_index))) != riders)")],
        "host-pass-partition"),
    "place-in-named-node": ("network", [("vehicle_riding_point_to_local(&body.position, &body.forward, &body.up, &target, &place);",
        "place = deck->place;")], "host-deck-node-equivalent"),
    "ceiling-in-node": ("network", [("(vehicle_riding_is_riding(&host) ? place.z - ground_place_height",
        "(vehicle_riding_is_riding(&host) ? deck->place.z - ground_place_height")], "host-deck-node-forgery"),
    "speeds-in-current-frame": ("network", [("subtract_vectors3d(&noted->velocity, &noted->frame_velocity, &taken_relative);",
        "subtract_vectors3d(&noted->velocity, frame_velocity, &taken_relative);")], "host-deck-accelerating"),
    "deck-anchor-unchecked": ("network", [("else if (ticks <= 0 || !distributed_on_foot_move_valid(bound, &anchor, &target, ticks))",
        "else if (FALSE)")], "host-deck-anchor-walk"),
    "host-moved-unchecked": ("network", [("if (!distributed_on_foot_move_valid(bound, &taken_host, &host_now, now - accepted.taken_host_time))",
        "if (FALSE)")], "host-deck-host-moved"),
     "alternating-drops-anchor": ("network", [("\ttarget = state->position;",
         "\tif (!deck_part) accepted.valid = FALSE;\n\ttarget = state->position;"),
        ("accepted.vehicle_index = host.object_index;", "accepted.vehicle_index = deck_part ? host.object_index : NONE;")],
        "host-deck-part-alternating"),
    "window-drops-anchor": ("network", [("(host.ticks > 0 && distributed_accepted[player_index].valid &&", "(FALSE && distributed_accepted[player_index].valid &&")],
        "host-deck-window-switch"),
    "window-deck-ceiling": ("network", [("(vehicle_riding_is_riding(&host) ? place.z", "(TRUE ? place.z")], "host-deck-jump-descending"),
    "window-frame-ignored": ("network", [("distributed_host_speeds[player_index].ground_frame_k * (real)MAX(bound->airborne_ticks, 0)",
        "0.0f")], "host-deck-jump-rising"),
    "frame-not-kept": ("network", [("distributed_host_speeds[player_index].ground_frame_k * (real)MAX(bound->airborne_ticks, 0)",
        "host.velocity.k * (real)MAX(bound->airborne_ticks, 0)")], "host-deck-hover-stopped"),
    "window-ceiling-removed": ("network", [("target.z - distributed_host_speeds[player_index].ground_height -\n\t\tdistributed_host_speeds[player_index].ground_frame_k * (real)MAX(bound->airborne_ticks, 0)",
        "0.0f")], "host-deck-window-ceiling"),
    "velocity-needs-write": ("network", [("\tif (velocity)\n\t\tobject->object.translational_velocity = *velocity;",
        "\tif (velocity && FALSE)\n\t\tobject->object.translational_velocity = *velocity;")],
        "apply-velocity-within-tolerance"),
    "anchor-reset-on-missing-part": ("network", [("\tleash = now < accepted.transition_until &&",
        "\tif (!state->deck_present || state->deck.ticks <= 0)\n\t\taccepted.valid = FALSE;\n\tleash = now < accepted.transition_until &&")],
        "host-anchor-omitted-parts"),
    "anchor-reset-each-carry": ("network", [("&taken_host, &host_now, now - accepted.taken_host_time)",
        "&accepted.taken_host_position, &object->object.position, now - accepted.taken_host_time)")],
        "host-anchor-named-resets"),
    "forged-ticks-taken": ("network", [("\tdistributed_accepted[player_index] = accepted;\n\tif (reset)",
        "\tbiped_riding_take_deck(unit_index, deck);\n\tdistributed_accepted[player_index] = accepted;\n\tif (reset)")], "host-forged"),
    "touch-allowance": ("network", [("\t\tHOST_BLEND_DISTANCE;\n\treal fall_reach", "\t\tHOST_BLEND_DISTANCE + 0.5f;\n\treal fall_reach")],
        "host-parked-rub"),
    "touch-allowance-pushed": ("network", [("\t\tHOST_BLEND_DISTANCE;\n\treal fall_reach", "\t\tHOST_BLEND_DISTANCE + 1.5f;\n\treal fall_reach")],
        "host-push-not-made"),
    "client-return-widens-bound": ("network", [(
        "\t\telse if (ticks <= 0 || !distributed_on_foot_move_valid(bound, &anchor, &target, ticks))\n\t\t{\n\t\t\treturn FALSE;\n\t\t}\n",
        "\t\telse if (ticks <= 0 || !distributed_on_foot_move_valid(bound, &anchor, &target, ticks))\n"
        "\t\t{\n\t\t\taccepted.valid = FALSE;\n\t\t\tground_place_height = host_place.z;\n\t\t\treset = TRUE;\n\t\t}\n")],
        "client-return-no-allowance"),
    "agreed-board-arms": ("network", [("if (host_changed && vehicle_riding_transition_agreed(", "if (FALSE && vehicle_riding_transition_agreed(")],
        "host-physics-board-rise-agreed"),
    "agreed-board-arms-world-word": ("network", [("if (host_changed && vehicle_riding_transition_agreed(", "if (FALSE && vehicle_riding_transition_agreed(")],
        "host-physics-board-agreed"),
    "agreed-step-off-arms": ("network", [("if (host_changed && vehicle_riding_transition_agreed(", "if (FALSE && vehicle_riding_transition_agreed(")],
        "host-step-off-together-agreed"),
    "agreed-board-swallows-air": ("riding", [("client_ticks == RIDING_TICKS && client_vehicle == host_vehicle &&\n\t\t(host_before <= 0",
        "client_ticks > 0 && client_vehicle == host_vehicle &&\n\t\t(host_before <= 0")], "host-physics-board-client-air-arms"),
    "agreed-step-off-any-vehicle": ("riding", [("client_ticks > 0 && client_ticks < RIDING_TICKS && client_vehicle == host_vehicle)",
        "client_ticks > 0 && client_ticks < RIDING_TICKS)")], "host-step-off-other-vehicle-arms"),
    "landing-in-air-arms": ("network", [("else if (!host_changed && client_changed &&\n\t\t\t\tvehicle_riding_word_on_air_deck(",
        "else if (FALSE && !host_changed && client_changed &&\n\t\t\t\tvehicle_riding_word_on_air_deck(")],
        "host-client-lands-in-host-air"),
    "landing-keeps-running-leash": ("network", [("\t\t\t\taccepted->transition_until = 0;\n\t\t\t}\n\t\t\telse if (host_changed || (client_changed",
        "\t\t\t}\n\t\t\telse if (host_changed || (client_changed")], "host-client-lands-ends-running-leash"),
    "landing-any-vehicle": ("riding", [(" && client_ticks == RIDING_TICKS && client_vehicle == host_vehicle;", " && client_ticks == RIDING_TICKS;")],
        "host-client-lands-other-vehicle-arms"),
    "landing-over-riding-host": ("riding", [("return host_ticks > 0 && host_ticks < RIDING_TICKS && client_ticks == RIDING_TICKS", "return host_ticks > 0 && client_ticks == RIDING_TICKS")],
        "host-client-lands-host-riding-arms"),
    "board-ack-arms": ("network", [("if (acknowledged)", "if (acknowledged && FALSE)")], "host-physics-board-agreed"),
    "board-ack-unbounded": ("network", [("\t\t\t\tnow <= accepted->transition_board_until &&\n", "")], "host-board-ack-late-arms"),
    "board-ack-spends": ("network", [("\t\t\t\taccepted->transition_board_vehicle = NONE;\n\t\t\t}\n\t\t\telse if (!host_changed && client_changed &&",
        "\t\t\t\taccepted->transition_board_vehicle = NONE;\n\t\t\t\taccepted->transition_client_spent = TRUE;\n\t\t\t}\n\t\t\telse if (!host_changed && client_changed &&")],
        "host-board-ack-then-jump-arms"),
    "board-ack-any-vehicle": ("network", [("client_ticks == RIDING_TICKS && client_vehicle == accepted->transition_board_vehicle &&\n",
        "client_ticks == RIDING_TICKS &&\n")], "host-board-ack-other-vehicle-arms"),
    "world-word-agreement-unbounded": ("riding", [("!client_part && word_taken)", "!client_part)")], "host-board-world-word-stale-arms"),
    "flip-after-leash-rearms": ("network", [("now == accepted->transition_leash_time + 1)", "FALSE)")], "host-flip-after-leash-write"),
    "flip-exemption-unbounded": ("network", [("now == accepted->transition_leash_time + 1)", "now >= accepted->transition_leash_time + 1)")],
        "host-flip-after-leash-write"),
    "leash-write-unstamped": ("network", [("\t\tif (leash)\n\t\t\taccepted.transition_leash_time = now;\n", "")], "host-flip-after-leash-write"),
    "leash-world-write-unstamped": ("network", [("\t\tif (leash)\n\t\t\tdistributed_accepted[player_index].transition_leash_time = now;\n", "")],
        "host-leash-world-write-stamped"),
    "leash-overrides-deck-word": ("network", [("now < accepted.transition_until && !deck_part &&", "now < accepted.transition_until &&")],
        "host-leash-keeps-support"),
    "world-word-lifts-along": ("network", [("if (vehicle_riding_is_riding(&host) && (!state->deck_present ||", "if (leash && vehicle_riding_is_riding(&host) && (!state->deck_present ||")],
        "host-along-world-word-no-lift"),
    "world-word-lifts-leash": ("network", [("if (vehicle_riding_is_riding(&host) && (!state->deck_present ||", "if (!leash && vehicle_riding_is_riding(&host) && (!state->deck_present ||")],
        "host-leash-world-word-no-lift"),
    "unlifted-clamps-down": ("riding", [("if (place.z > host_place.z)\n\t{\n\t\tplace.z = host_place.z;", "if (place.z != host_place.z)\n\t{\n\t\tplace.z = host_place.z;")],
        "host-leash-world-word-lowers"),
    "unlifted-air-part": ("network", [("(!state->deck_present ||\n\t\t(deck->vehicle_index != host.object_index && deck->ticks != RIDING_TICKS)))\n\t\tvehicle_riding_host_unlifted", "TRUE)\n\t\tvehicle_riding_host_unlifted")],
        "host-along-jump-takeoff"),
    "unlifted-other-vehicle-any": ("network", [(" && deck->ticks != RIDING_TICKS)))\n\t\tvehicle_riding_host_unlifted", ")))\n\t\tvehicle_riding_host_unlifted")],
        "host-deck-word-other-vehicle-riding-follows"),
    "unlifted-other-vehicle-air-lifts": ("network", [("(!state->deck_present ||\n\t\t(deck->vehicle_index != host.object_index && deck->ticks != RIDING_TICKS)))\n\t\tvehicle_riding_host_unlifted", "!state->deck_present)\n\t\tvehicle_riding_host_unlifted")],
        "host-deck-word-other-vehicle-air-no-lift"),
    "landing-take-blended": ("network", [("leash || landing ? 0.f : HOST_BLEND_DISTANCE", "leash || landing ? (leash ? 0.f : HOST_BLEND_DISTANCE) : HOST_BLEND_DISTANCE")],
        "host-landing-deck-take-exact"),
    "landing-take-every-deck": ("network", [("landing = deck_part && !leash && accepted.taken_air_unit_index == unit_index &&\n\t\taccepted.taken_air_vehicle == host.object_index;",
        "landing = deck_part && !leash;")], "host-deck-take-steady-blends"),
    "landing-take-any-vehicle": ("network", [("&&\n\t\taccepted.taken_air_vehicle == host.object_index;", ";")],
        "host-deck-after-other-vehicle-air-blends"),
    "landing-take-no-part-counts": ("network", [("return state->deck_present && state->deck.ticks > 0 &&", "return (!state->deck_present || state->deck.ticks > 0) &&")],
        "host-deck-after-no-part-blends"),
    "landing-take-record-received": ("network", [("\tlanding = deck_part && !leash &&",
        "\tdistributed_accepted[player_index].taken_air_unit_index = unit_index;\n\tdistributed_accepted[player_index].taken_air_vehicle = distributed_air_part_vehicle(state);\n\tlanding = deck_part && !leash &&")],
        "host-landing-record-from-taken-only"),
    "landing-take-record-kept": ("network", [("\taccepted.taken_air_unit_index = unit_index;\n\tif (!keep_air)\n\t\taccepted.taken_air_vehicle = distributed_air_part_vehicle(state);\n\tif (!accepted.valid",
        "\tif (!deck_part)\n\t{\n\t\taccepted.taken_air_unit_index = unit_index;\n\t\tif (!keep_air)\n\t\t\taccepted.taken_air_vehicle = distributed_air_part_vehicle(state);\n\t}\n\tif (!accepted.valid")],
        "host-landing-second-deck-take-blends"),
    "landing-take-unit-kept": ("network", [("accepted.taken_air_unit_index == unit_index &&\n\t\t", "")], "host-landing-new-life-blends"),
    "landing-leash-keeps-other-unit": ("network", [("keep_air = leash && accepted.taken_air_unit_index == unit_index && accepted.taken_air_vehicle != NONE &&",
        "keep_air = leash && accepted.taken_air_vehicle != NONE &&")], "host-landing-new-life-after-leash-blends"),
    "landing-take-unchecked": ("network", [("else if (ticks <= 0 || !distributed_on_foot_move_valid(bound, &anchor, &target, ticks))",
        "else if (!landing && (ticks <= 0 || !distributed_on_foot_move_valid(bound, &anchor, &target, ticks)))")],
        "host-landing-word-refused"),
    "landing-leash-consumes": ("network", [("keep_air = leash && accepted", "keep_air = FALSE && accepted")],
        "host-landing-after-riding-leash-exact"),
    "landing-leash-keeps-any": ("network", [("keep_air = leash && accepted.taken_air_unit_index == unit_index && accepted.taken_air_vehicle != NONE &&\n\t\tstate->deck_present && state->deck.ticks == RIDING_TICKS && state->deck.vehicle_index == accepted.taken_air_vehicle;",
        "keep_air = leash;")], "host-landing-leash-other-word-consumes"),
    "host-now-recomputed": ("network", [("\t\t/* (both ends carried by the same body now, never its world travel) */\n",
        "\t\tvehicle_riding_point_to_world(&body.position, &body.forward, &body.up, &host_place, &host_now);\n")], "host-now-once"),
    "window-carry-in-deck-frame": ("network", [("\t\tif (!vehicle_riding_is_riding(&host))\n\t\t{\n\t\t\ttaken_host = accepted.taken_host_position;",
        "\t\tif (FALSE)\n\t\t{\n\t\t\ttaken_host = accepted.taken_host_position;")], "host-window-braking-climb"),
}


def generated(control=None):
    sources = {"network": read(NETWORK), "riding": read(RIDING)}
    if control:
        owner, replacements, _ = NEGATIVE_CONTROLS[control]
        for before, after in replacements:
            sources[owner] = mutated(sources[owner], before, after)
    network = sources["network"]
    header = read("port/linux/game/network_distributed.h")
    types = engine() + "\n" + without_includes(read("port/linux/game/vehicle_riding.h")) + "\n"
    for name in ("RIDING_AIR_STILL", "riding_air_grip"):
        types += f"#define {name} {constant(sources['riding'], name)}\n"
    types += structure(header, "distributed_vector") + "\n"
    types += enum_with(read("source/game/players.h"), "_player_powerup_active_camouflage") + "\n"
    types += enum_with(network, "_distributed_pose_facing,") + "\n"
    types += structure(network, "distributed_unit_state") + "\n"
    types += structure(network, "distributed_on_foot_bound") + "\n"
    for name in ("HOST_ACCEPT_TOLERANCE", "MAXIMUM_PREDICTED_SPEED", "PREDICTED_ON_FOOT_SPEED_SCALE",
                 "PREDICTED_ON_FOOT_SPEED_MARGIN", "PREDICTED_RISE_TOLERANCE"):
        types += re.search(r"^#define " + name + r" .*$", network, re.M).group(0) + "\n"
    for name in ("DISTRIBUTED_UNIT_SCALE", "DISTRIBUTED_VELOCITY_SCALE", "HOST_BLEND_DISTANCE"):
        types += re.search(r"^#define " + name + r" .*$", header, re.M).group(0) + "\n"
    for name in ("PREDICTION_ANCHOR_TICKS", "PREDICTION_JITTER_TICKS", "PREDICTION_CEILING_LEAD_TICKS",
                 "PREDICTION_SPEED_TICKS"):
        types += f"#define {name} {constant(network + read('source/cseries/cseries.h'), name)}\n"
    for name in ("predictions", "accepted", "host_speeds"):
        types += re.search(r"static struct(?: distributed_host_speed| distributed_accepted_state)?\n\{[^}]*\} distributed_" + name +
                           r"\[MAXIMUM_TRACKED_PLAYERS\];", network).group(0) + "\n"
    # (the latency drift figures distributed_take_prediction notes: the source's own definitions)
    types += re.search(r"^#define DIVERGENCE_APART_DISTANCE .*$", network, re.M).group(0) + "\n"
    end = network.index("} distributed_divergence;")
    types += network[network.rindex("static struct", 0, end):end] + "} distributed_divergence;\n"
    code = "".join(function(sources["riding"], name) + "\n" for name in RIDING_FUNCTIONS)
    code += ("static long to_world_calls;\n#define vehicle_riding_point_to_world(...) "
             "(to_world_calls++, vehicle_riding_point_to_world(__VA_ARGS__))\n")
    for name in NETWORK_FUNCTIONS:
        if name == "distributed_apply_predictions":
            # (the passes' count of the words taken, by player and pass; a direct call of the take is not one)
            code += ("#define distributed_take_prediction(p, u, b) "
                     "(apply_calls[p]++, apply_phase[p] = phase, distributed_take_prediction(p, u, b))\n")
        code += function(network, name) + "\n"
    code += "#undef distributed_take_prediction\n"
    return (("types.inc", types), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("host_take", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    case = NEGATIVE_CONTROLS[control][-1]
    status, output = run(build("host_take", generated(control)), case)
    assert status == CHECK_FAILED and "check failed:" in output, f"{control}: {status}: {output}"


def accepted_state(network):
    return re.search(r"static struct distributed_accepted_state\n\{.*?\n\} distributed_accepted\[", network, re.S).group(0)


def test_accepted_state_has_no_node_index():
    assert "node_index" not in accepted_state(read(NETWORK))


def test_accepted_node_kept_fails_the_pin():
    network = mutated(read(NETWORK), "\tlong vehicle_index;\n\treal_point3d place;\n\treal_point3d taken_host_place;",
                      "\tlong vehicle_index;\n\tshort node_index;\n\treal_point3d place;\n\treal_point3d taken_host_place;")
    assert "node_index" in accepted_state(network)
