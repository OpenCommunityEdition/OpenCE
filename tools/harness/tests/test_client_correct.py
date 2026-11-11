"""A client takes a remote rider in this machine's deck frame."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import CHECK_FAILED, biped_riding_sources, build, constant, enum_with, function, mutated, read, run, structure
from test_vehicle_riding import engine, without_includes

NETWORK = "port/linux/game/network_distributed.c"
RIDING = "port/linux/game/vehicle_riding.c"
BIPEDS = "port/linux/game/vehicle_riding_bipeds.c"
CASES = ["remote-deck-lagging", "remote-deck-blend", "remote-deck-still", "remote-deck-velocity",
         "remote-air", "remote-invalid-clears", "remote-late-join"]
CASES += ["own-history-deck", "own-deck-lagging", "own-deck-lagging-far", "own-deck-correct-once",
          "own-host-boards", "own-host-drops", "own-boarding-ahead", "own-deck-twice",
          "claim-node-kept", "claim-crossing", "own-echo-after-crossing", "own-echo-after-reacquire"]
CASES += ["own-stale-host-current"]
CASES += ["own-boarding-retry"]
CASES += ["own-host-drop-reboard"]
CASES += ["own-air-difference", "own-air-twice", "own-air-other-window",
          "own-air-from-riding-echo", "own-air-two-jumps"]
CASES += ["own-air-peak-held", "own-air-whole-down", "own-air-whole-up"]
CASES += ["remote-receive-before-carry"]
CASES += ["remote-written-retakes"]
CASES += ["remote-receive-above-tolerance"]
CASES += ["remote-opposing-motion", "remote-opposing-converges"]
CASES += ["remote-opposing-exact-target"]
CASES += ["remote-air-one-frame-step", "remote-air-genuine-residual", "remote-air-frame-is-hosts",
          "remote-takeoff-frame-after-refresh"]
CASES += ["deck-place-carried-frame", "deck-place-driven-1140", "deck-place-unwritten-unchanged",
          "deck-place-air-live-frame", "remote-standing-same-snapshot"]
CASES += ["own-verdict-within-tolerance", "remote-verdict-within-tolerance", "invalid-part-drops-host",
           "late-join-verdict", "transition-predict", "host-return-transaction", "own-episode-echo"]
CASES += ["own-legacy-verdict-equal-world", "own-legacy-verdict-open-equal-world",
          "own-legacy-over-local-cabin-jump", "own-legacy-verdict-history-no-deck",
          "own-legacy-verdict-stale-episode", "own-legacy-verdict-resolved",
          "own-legacy-verdict-new-episode-clears", "own-legacy-verdict-landing-clears",
          "own-legacy-verdict-vehicle-change-clears", "own-cabin-verdict-unchanged",
          "remote-legacy-verdict-held", "own-host-air-claim-after-grace", "own-host-air-claim-stale"]
CASES += ["own-host-air-other-vehicle-corrects", "own-host-air-stale-episode-corrects", "own-host-air-taken-corrects"]
CASES += ["own-unechoed-deck-lagging", "own-unechoed-deck-error", "own-unechoed-other-vehicle", "own-unechoed-other-node"]
CASES += ["own-legacy-verdict-off-deck-host-held", "own-legacy-verdict-off-deck-local-resets"]
# a fault in the real transaction, and the case that must fail a CHECK
NEGATIVE_CONTROLS = {
    "host-verdict-before-over-deck": ("riding", [("\tif (air->host_verdict && airborne)\n\t\treturn FALSE;\n", ""), (
        "\treturn air->enclosure == _vehicle_riding_enclosure_unknown;",
        "\tif (air->host_verdict && airborne)\n\t\treturn FALSE;\n\treturn air->enclosure == _vehicle_riding_enclosure_unknown;")],
        "own-legacy-verdict-off-deck-host-held"),
    "local-verdict-off-deck-not-reset": ("riding", [("if (!step->over_deck || !airborne)", "if (!airborne)")],
        "own-legacy-verdict-off-deck-local-resets"),
    "unechoed-rider-world-only": ("network", [(
        "\tif (distributed_correct_own_unechoed_rider(unit_index, state))\n\t\treturn;\n", "")],
        "own-unechoed-deck-lagging"),
    "unechoed-no-rebase": ("network", [(
        "LOCAL_CORRECTION_TOLERANCE, 0.0f, 0.0f, 0.0f))\n\t\treturn TRUE;\n\tbiped_riding_rebase(unit_index);\n\treturn TRUE;",
        "LOCAL_CORRECTION_TOLERANCE, 0.0f, 0.0f, 0.0f))\n\t\treturn TRUE;\n\treturn TRUE;")], "own-unechoed-deck-error"),
    "unechoed-rider-any-node": ("network", [(
        "current_deck.vehicle_index != state->deck.vehicle_index || current_deck.node_index != state->deck.node_index)",
        "current_deck.vehicle_index != state->deck.vehicle_index)")], "own-unechoed-other-node"),
    "unechoed-rider-any-vehicle": ("network", [(
        "current_deck.vehicle_index != state->deck.vehicle_index || ", "")], "own-unechoed-other-vehicle"),
    "host-air-other-vehicle-no-correction": ("network", [("\t\t\tif (host_air && current_present && current_deck.ticks == RIDING_TICKS &&",
        "\t\t\tif (host_air && current_present && current_deck.vehicle_index != host_vehicle)\n\t\t\t\treturn;\n"
        "\t\t\tif (host_air && current_present && current_deck.ticks == RIDING_TICKS &&")], "own-host-air-other-vehicle-corrects"),
    "host-air-stale-episode-no-correction": ("network", [("\t\t\tif (host_air && current_present && current_deck.ticks == RIDING_TICKS &&",
        "\t\t\tif (host_air && current_present && !current_episode)\n\t\t\t\treturn;\n"
        "\t\t\tif (host_air && current_present && current_deck.ticks == RIDING_TICKS &&")], "own-host-air-stale-episode-corrects"),
    "host-air-taken-no-correction": ("network", [("\t\t\tif (host_air && current_present && current_deck.ticks == RIDING_TICKS &&",
        "\t\t\tif (host_air && current_present && claim && claim->taken)\n\t\t\t\treturn;\n"
        "\t\t\tif (host_air && current_present && current_deck.ticks == RIDING_TICKS &&")], "own-host-air-taken-corrects"),
    "legacy-verdict-positional-gate": ("network", [(
        "\tbiped_riding_air_verdict_host(unit_index, distributed_deck_whole(state) ? &state->deck : NULL, current_episode);\n", ""), (
        "\t\t\t\tboolean air_taken = FALSE;", "\t\t\t\tboolean air_taken = FALSE;\n"
        "\t\t\t\tbiped_riding_air_verdict_host(unit_index, distributed_deck_whole(state) ? &state->deck : NULL, current_episode);")],
        "own-legacy-verdict-equal-world"),
    "legacy-verdict-resampled": ("riding", [("if (air->host_verdict && airborne)", "if (FALSE)")],
        "own-legacy-verdict-equal-world"),
    "local-cabin-precedence": ("riding", [("if (!(cabin->host_legacy && state->ticks < RIDING_TICKS) &&\n\t\t\tcabin->jump !=",
        "if (cabin->jump !=")], "own-legacy-over-local-cabin-jump"),
    "legacy-cabin-bit": ("riding", [("cabin && !cabin->host_legacy && cabin->vehicle_index", "cabin && cabin->vehicle_index")],
        "own-legacy-over-local-cabin-jump"),
    "legacy-episode-permissive": ("network", [("boolean current_episode = distributed_own_episode_current(player, unit_index, state);",
        "boolean current_episode = distributed_own_window_current(player, unit_index, state);")], "own-legacy-verdict-history-no-deck"),
    "host-air-claim-bypass-grace": ("network", [("air_claim = vehicle_riding_claim_grace(&pending, NONE, NONE, host_vehicle, host_node, grace);",
        "air_claim = TRUE;")], "own-host-air-claim-after-grace"),
    "host-air-claim-keeps-standing-history": ("network", [("(air_claim || (noted->deck.ticks > 0 && noted->deck.ticks < RIDING_TICKS)) &&",
        "(noted->deck.ticks > 0 && noted->deck.ticks < RIDING_TICKS) &&")], "own-host-air-claim-after-grace"),
    "host-peak-raised": ("riding", [("if (!air->host_peak)", "if (TRUE)")], "own-air-peak-held"),
    "whole-baseline-kept": ("network", [("noted->air_baseline_valid = FALSE;", ";")], "own-air-whole-down"),
    "whole-baseline-ignored": ("network", [("\t\t\t\t\t\town->air_baseline_valid &&\n", "")], "own-air-whole-up"),
    "air-whole-always": ("network", [(
        "if (own->deck_present && own->deck.ticks > 0 && own->deck.ticks < RIDING_TICKS &&",
        "if (FALSE && own->deck_present && own->deck.ticks > 0 && own->deck.ticks < RIDING_TICKS &&"),
        # The mutant removes the only assignment; keep it compilable so the runtime check judges the fault.
        ("real_vector3d air_difference;", "real_vector3d air_difference = {0};")],
        "own-air-difference"),
    "air-history-not-rebased": ("network", [(
        "add_vectors3d(&noted->deck.velocity, &air_difference, &noted->deck.velocity);", ";")], "own-air-twice"),
    "peak-not-taken": ("riding", [("air->peak = peak;", "air->peak = air->peak;")], "own-air-difference"),
    "rebase-other-window": ("network", [(
        "if (air_rebased && noted->deck_present && noted->deck.ticks > 0 && noted->deck.ticks < RIDING_TICKS &&\n"
        "\t\t\t\t\t\t\tnoted->window == own->window && noted->air_episode == own->air_episode &&\n"
        "\t\t\t\t\t\t\tnoted->deck.vehicle_index == host_vehicle)", "if (air_rebased)")], "own-air-other-window"),
    "difference-from-riding": ("network", [(
        "if (own->deck_present && own->deck.ticks > 0 && own->deck.ticks < RIDING_TICKS &&",
        "if (own->deck_present &&" )], "own-air-from-riding-echo"),
    "episode-ignored": ("network", [(
        "!stale_window && own->air_episode == distributed_own_air_episodes[local_player_index] &&",
        "!stale_window &&")], "own-air-two-jumps"),
    "rebase-riding-entries": ("network", [(
        "if (air_rebased && noted->deck_present && noted->deck.ticks > 0 && noted->deck.ticks < RIDING_TICKS &&\n"
        "\t\t\t\t\t\t\tnoted->window == own->window && noted->air_episode == own->air_episode &&\n"
        "\t\t\t\t\t\t\tnoted->deck.vehicle_index == host_vehicle)",
        "if (air_rebased && noted->deck_present && noted->window == own->window)")], "own-air-two-jumps"),
    "remote-world-veto": ("network", [(
        "tolerance = -1.0f;", "tolerance = REMOTE_CORRECTION_TOLERANCE;")], "remote-opposing-motion"),
    "remote-world-veto-convergence": ("network", [(
        "tolerance = -1.0f;", "tolerance = REMOTE_CORRECTION_TOLERANCE;")], "remote-opposing-converges"),
    "remote-zero-world-veto": ("network", [(
        "tolerance = -1.0f;", "tolerance = 0.0f;")], "remote-opposing-exact-target"),
    "remote-world-error": ("network", [(
        "if (distance_squared3d(&place, &deck->place) <=",
        "if (distance_squared3d(&before, &target) <=")], "remote-receive-above-tolerance"),
    "remote-always-retakes": ("network", [(
        "(!same_window || tolerance < 0.0f)",
        "TRUE")], "remote-receive-before-carry"),
    "remote-written-keeps-snapshot": ("network", [(
        "(!same_window || tolerance < 0.0f)",
        "!same_window")], "remote-written-retakes"),
    "skip-verdict-within-tolerance": ("network", [(
        "\tbiped_riding_cabin_host(unit_index, distributed_deck_whole(state) ? &state->deck : NULL,\n"
        "\t\tstate->deck_present && state->deck.ticks < RIDING_TICKS &&\n"
        "\t\t!TEST_FLAG(state->deck.flags, _vehicle_riding_deck_cabin_bit) ? current_episode :\n"
        "\t\tdistributed_own_window_current(player, unit_index, state));\n", ""), (
        "\t\t\t\t\t\treturn;\n\t\t\t\t\ttarget_deck.place.x += deck_error.i;",
        "\t\t\t\t\t\treturn;\n\t\t\t\t\tbiped_riding_cabin_host(unit_index, &state->deck, TRUE);\n"
        "\t\t\t\t\ttarget_deck.place.x += deck_error.i;")], "own-verdict-within-tolerance"),
    "echo-any-episode": ("network", [(
        "\t\t(state->deck.ticks >= RIDING_TICKS ||\n\t\town->air_episode == distributed_own_air_episodes[local_player_index]) &&\n", "")], "own-episode-echo"),
    "invalid-keeps-host": ("bipeds", [(
        "\tif (!deck)\n\t\tvehicle_riding_cabin_drop_host(cabin);\n", "\tif (!deck)\n\t\t;\n")], "invalid-part-drops-host"),
    "caller-keeps-obsolete-claim": ("network", [(
        "if (claim && claim->taken && (claim->vehicle_index != host_vehicle || claim->node_index != host_node))",
        "if (FALSE)")], "own-host-drop-reboard"),
    "claim-taken-before-boarding": ("network", [("pending.taken = FALSE;\n\t\t\t\t\t\tpending.disagreement = grace;",
                                                 "pending.taken = TRUE;\n\t\t\t\t\t\tpending.disagreement = 0;")],
                                    "own-boarding-retry"),
    "stale-claim-from-echo": ("network", [("if (stale_window)\n\t\t\t\t{", "if (FALSE)\n\t\t\t\t{")],
                               "own-stale-host-current"),
    "own-world-compare": ("network", [(
        "vector_from_points3d(&own->deck.place, &state->deck.place, &deck_error);",
        "vector_from_points3d(&own->position, &state->position, &deck_error);")], "own-deck-lagging-far"),
    "own-no-rebase": ("network", [("\t\t\t\t\tbiped_riding_rebase(unit_index);", "\t\t\t\t\t;")], "own-deck-correct-once"),
    "claim-take-at-once": ("riding", [("if (++claim->disagreement <= grace)", "if (FALSE)")], "own-host-boards"),
    "claim-second-grace": ("riding", [("if (claim->taken)\n\t\treturn FALSE;", "if (FALSE)\n\t\treturn FALSE;")], "claim-node-kept"),
    "node-kept-ignored": ("riding", [("if (!vehicle->node_kept && state->node_index", "if (state->node_index")], "claim-node-kept"),
    "host-drop-under-deck": ("network", [("if (target_deck.place.z < current_deck.place.z)", "if (FALSE)")], "own-host-drops"),
    "history-no-deck": ("network", [("own->deck_present = unit_index != NONE && biped_riding_deck(unit_index, &own->deck);",
                                          "own->deck_present = FALSE;")], "own-history-deck"),
    "compare-against-now": ("network", [(
        "boolean own_riding = own->deck_present && own->deck.ticks == RIDING_TICKS;",
        "boolean own_riding = current_present && current_deck.ticks == RIDING_TICKS;"), (
        "long own_vehicle = own->deck_present ? own->deck.vehicle_index : NONE;",
        "long own_vehicle = current_present ? current_deck.vehicle_index : NONE;"), (
        "short own_node = own->deck_present ? own->deck.node_index : NONE;",
        "short own_node = current_present ? current_deck.node_index : NONE;")], "own-boarding-ahead"),
    "echo-any-window": ("network", [(
        "boolean stale_window = own->deck_present && (own->window != distributed_own_windows[local_player_index] ||\n"
        "\t\t\t\t!current_present || current_deck.vehicle_index != own->deck.vehicle_index);",
        "boolean stale_window = FALSE;")], "own-echo-after-reacquire"),
    "echo-vehicle-only": ("network", [(
        "own->window != distributed_own_windows[local_player_index] ||\n\t\t\t\t!current_present",
        "!current_present")], "own-echo-after-reacquire"),
    "deck-history-not-rebased": ("network", [(
        "noted->deck.place.x += deck_error.i;\n\t\t\t\t\t\t\tnoted->deck.place.y += deck_error.j;\n"
        "\t\t\t\t\t\t\tnoted->deck.place.z += deck_error.k;", ";")], "own-deck-twice"),
    "remote-world-target": ("network", [(
        "distributed_apply_state(unit_index, state, &target, &velocity, tolerance,",
        "distributed_apply_state(unit_index, state, &state->position, &velocity, tolerance,")],
        "remote-deck-lagging"),
    "remote-place-from-part": ("riding", [(
        "/* (the blend may not have reached the part's place) */\n"
        "\tvehicle_riding_point_to_local(&vehicle->position, &vehicle->forward, &vehicle->up, base, &state->local_point);",
        "state->local_point = deck->place;")], "remote-deck-blend"),
    "remote-velocity-world": ("network", [(
        "distributed_apply_state(unit_index, state, &target, &velocity, tolerance,",
        "distributed_apply_state(unit_index, state, &target, NULL, tolerance,")],
        "remote-deck-velocity"),
    "remote-take-before-blend": ("network", [(
        "\t\t\tadd_vectors3d(&deck->velocity, &frame, &velocity);",
        "\t\t\tbiped_riding_take_deck(unit_index, deck);\n"
        "\t\t\tadd_vectors3d(&deck->velocity, &frame, &velocity);"), (
        "\t\t\t\tbiped_riding_take_deck(unit_index, deck);", "\t\t\t\t;")], "remote-deck-blend"),
    "remote-air-deck-frame": ("network", [(
        "taken after) */\n\t\tdistributed_vector_unpack(&state->velocity, DISTRIBUTED_VELOCITY_SCALE, &velocity);\n"
        "\t\tdistributed_apply_state(unit_index, state, &state->position, &velocity,",
        "taken after) */\n\t\tbiped_riding_deck_position(unit_index, deck, &target);\n"
        "\t\tdistributed_vector_unpack(&state->velocity, DISTRIBUTED_VELOCITY_SCALE, &velocity);\n"
        "\t\tdistributed_apply_state(unit_index, state, &target, &velocity,")], "remote-air"),
    "remote-air-velocity-uncommitted": ("network", [(
        "distributed_apply_state(unit_index, state, &state->position, &velocity,",
        "distributed_apply_state(unit_index, state, &state->position, NULL,")], "remote-air-one-frame-step"),
    "remote-air-riding-branch-world": ("network", [(
        "\t\t\tadd_vectors3d(&deck->velocity, &frame, &velocity);",
        "\t\t\tdistributed_vector_unpack(&state->velocity, DISTRIBUTED_VELOCITY_SCALE, &velocity);")],
        "remote-deck-velocity"),
    "takeoff-frame-from-object": ("bipeds", [(
        "\tif (state.ticks > 0 && state.object_index == deck->vehicle_index && state.node_index == deck->node_index)\n"
        "\t\t*velocity = state.velocity;\n\telse",
        "\tif (0)\n\t\t*velocity = state.velocity;\n\telse")], "remote-takeoff-frame-after-refresh"),
    "deck-frame-live": ("riding", [('if (vehicle_riding_is_riding(state) && state->object_index == vehicle->object_index &&\n\t\tstate->node_index == vehicle->node_index)', "if (FALSE)")], "deck-place-carried-frame"),
    "deck-frame-live-standing": ("riding", [('if (vehicle_riding_is_riding(state) && state->object_index == vehicle->object_index &&\n\t\tstate->node_index == vehicle->node_index)', "if (FALSE)")], "remote-standing-same-snapshot"),
    "deck-frame-unwired": ("bipeds", [("return vehicle_riding_deck_make(&riding, &frame, &base,", "return vehicle_riding_deck_make(&riding, &vehicle, &base,")], "deck-place-carried-frame"),
    "deck-frame-in-air": ("riding", [('if (vehicle_riding_is_riding(state) && state->object_index == vehicle->object_index &&\n\t\tstate->node_index == vehicle->node_index)', "if (state->ticks > 0 && state->object_index == vehicle->object_index &&\n\t\tstate->node_index == vehicle->node_index)")], "deck-place-air-live-frame"),
    "remote-invalid-kept": ("network", [("\tbiped_riding_drop(unit_index);\n\tdistributed_apply_state",
                                          "\tdistributed_apply_state")], "remote-invalid-clears"),
    "remote-air-verdict-dropped": ("bipeds", [(
        "\tvehicle_riding_air_take(vehicle_riding_player_air(biped_riding_player(biped_get(biped_index))), deck);\n",
        "")], "remote-air"),
    "remote-air-frame-dropped": ("riding", [("\t\tstate->velocity = deck->velocity;\n", "")], "remote-air"),
    "remote-fresh-ticks-dropped": ("riding", [(
        "\tstate->node_index = deck->node_index;\n\tstate->ticks = deck->ticks;\n",
        "\tstate->node_index = deck->node_index;\n")], "remote-late-join"),
    "remote-fresh-falling-dropped": ("riding", [(
        "\tstate->node_index = deck->node_index;\n\tstate->ticks = deck->ticks;\n"
        "\tstate->falling = TEST_FLAG(deck->flags, _vehicle_riding_deck_falling_bit);\n",
        "\tstate->node_index = deck->node_index;\n\tstate->ticks = deck->ticks;\n\tstate->falling = 0;\n")],
        "remote-late-join"),
    "remote-fresh-velocity-kept": ("riding", [(
        "\tif (!(state->ticks > 0 && state->object_index == deck->vehicle_index && state->node_index == deck->node_index))\n"
        "\t\tvehicle_riding_deck_point_velocity(vehicle, base,",
        "\tif (0)\n\t\tvehicle_riding_deck_point_velocity(vehicle, base,")], "remote-late-join"),
    "remote-fresh-frame-kept": ("bipeds", [(
        "\tif (state.ticks > 0 && state.object_index == deck->vehicle_index && state.node_index == deck->node_index)\n"
        "\t\t*velocity = state.velocity;",
        "\tif (1)\n\t\t*velocity = state.velocity;")], "remote-late-join"),
}

RIDING_FUNCTIONS = (
    "riding_period", "riding_snapshot", "riding_body", "riding_anchor_body", "riding_open_window",
    "riding_node_motion", "vehicle_riding_correction_identity", "riding_correction_matrix",
    "vehicle_riding_correction_since", "vehicle_riding_clear", "vehicle_riding_drop",
    "vehicle_riding_is_riding", "vehicle_riding_point_to_local", "vehicle_riding_point_to_world",
    "vehicle_riding_yaw", "vehicle_riding_deck_point_velocity", "riding_node_judged",
    "vehicle_riding_snapshot_step", "vehicle_riding_deck_vehicle_named",
    "vehicle_riding_deck_values_valid", "vehicle_riding_deck_valid", "vehicle_riding_take_deck",
    "riding_air_clear", "vehicle_riding_air_take", "vehicle_riding_air_frame", "vehicle_riding_air_correct",
    "vehicle_riding_air_begin", "vehicle_riding_air_enclosure", "vehicle_riding_air_take_verdict", "riding_clip", "vehicle_riding_record")


def generated(control=None):
    sources = {"network": read(NETWORK), "riding": read(RIDING), "bipeds": biped_riding_sources()}
    if control:
        owner, replacements, _ = NEGATIVE_CONTROLS[control]
        for before, after in replacements:
            sources[owner] = mutated(sources[owner], before, after)
    network, riding, bipeds = (sources[k] for k in ("network", "riding", "bipeds"))
    header = read("port/linux/game/network_distributed.h")
    types = engine() + "\n" + without_includes(read("port/linux/game/vehicle_riding.h")) + "\n"
    own_type = structure(network.replace("static struct distributed_own_position", "struct distributed_own_position"), "distributed_own_position")
    own_type = own_type[:own_type.rindex("}") + 1] + ";"
    types += own_type + "\n"
    types += re.search(r"^#define MIN\(.*$", read("source/cseries/cseries.h"), re.M)[0] + "\n"
    for name in ("ELEVATOR_PHASE_TOLERANCE",):
        types += f"#define {name} {constant(network, name)}\n"
    types += structure(header, "distributed_vector") + "\n"
    types += enum_with(read("source/game/players.h"), "_player_powerup_active_camouflage") + "\n"
    types += enum_with(network, "_distributed_pose_facing,") + "\n"
    types += structure(network, "distributed_unit_state") + "\n"
    for name in ("DISTRIBUTED_UNIT_SCALE", "DISTRIBUTED_VELOCITY_SCALE", "REMOTE_BLEND_DISTANCE",
                 "REMOTE_CORRECTION_TOLERANCE"):
        types += re.search(r"^#define " + name + r" .*$", header, re.M).group(0) + "\n"
    types += "#define MAXIMUM_LOCAL_PLAYERS 4\n#define OWN_POSITION_TICKS 64\n"
    types += "#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS 128\n"
    types += re.search(r"^#define MAX\(.*$", read("source/cseries/cseries.h"), re.M)[0] + "\n"
    types += enum_with(network, "_distributed_unit_predicted_bit") + "\n"
    for name in ("LOCAL_CORRECTION_TOLERANCE",):
        types += re.search(r"^#define " + name + r" .*$", network, re.M)[0] + "\n"
    for name in ("SEAT_DISAGREEMENT_TICKS", "SEAT_DISAGREEMENT_SLACK_TICKS"):
        types += f"#define {name} {constant(network, name)}\n"
    for name in ("RIDING_NODE_SHIFT", "RIDING_NODE_TILT", "RIDING_SNAP_STEP", "RIDING_AIR_STILL"):
        types += re.search(r"^#define " + name + r" .*$", riding, re.M).group(0) + "\n"
    code = "".join(function(riding, name) + "\n" for name in RIDING_FUNCTIONS)
    for name in ("vehicle_riding_rebase", "vehicle_riding_cabin_verdict", "vehicle_riding_deck_frame",
                 "vehicle_riding_deck_make"):
        code += function(riding, name) + "\n"
    for name in ("vehicle_riding_player_claim", "vehicle_riding_claim_grace", "vehicle_riding_cabin_clear",
                  "vehicle_riding_cabin_take_host", "vehicle_riding_cabin_drop_host",
                  "vehicle_riding_cabin_begin", "vehicle_riding_cabin_standing", "vehicle_riding_cabin_take_off"):
        code += function(riding, name) + "\n"
    remote = function(network, "distributed_deck_whole") + "\n" + function(network, "distributed_own_window_current") + "\n"
    remote += function(network, "distributed_own_episode_current") + "\n"
    remote += function(network, "distributed_apply_remote_unit") + "\n"
    exports = function(bipeds, "biped_riding_deck_position") + "\n"
    exports += function(bipeds, "biped_riding_take_deck") + "\n"
    exports += function(bipeds, "biped_riding_frame_velocity") + "\n"
    exports += function(bipeds, "biped_riding_rebase") + "\n"
    exports += function(bipeds, "biped_riding_drop") + "\n"
    exports += function(bipeds, "biped_riding_deck") + "\n"
    exports += function(bipeds, "biped_riding_air_correct") + "\n"
    exports += function(bipeds, "biped_riding_air_take") + "\n"
    exports += function(bipeds, "biped_riding_cabin_host") + "\n"
    exports += function(bipeds, "biped_riding_air_verdict_host") + "\n"
    kept = bipeds[bipeds.index("\t\t/* (a taken host claim"):]
    kept = kept[:kept.index("\t\triding_step_valid =")]
    exports += ("static struct vehicle_riding_vehicle own_vehicle_for_step(void) {\n"
                "struct biped_datum *biped = &unit; struct vehicle_riding_vehicle riding_vehicle;\n" + kept +
                "CHECK(have_vehicle, \"kept vehicle missing\"); return riding_vehicle; }\n")
    return (("types.inc", types), ("riding.inc", code), ("bipeds.inc", exports),
            ("under_test.inc", function(network, "distributed_apply_state") + "\n" + remote + "\n" +
             function(network, "distributed_note_own_positions") + "\n" +
             function(network, "distributed_correct_own_unechoed_rider") + "\n" +
             function(network, "distributed_correct_own_unit").replace(
                 "error.i = state->position.x - own->position.x;",
                 "world_comparisons++; error.i = state->position.x - own->position.x;")))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("client_correct", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    case = NEGATIVE_CONTROLS[control][-1]
    status, output = run(build("client_correct", generated(control)), case)
    assert status == CHECK_FAILED and "check failed:" in output, f"{control}: {status}: {output}"
