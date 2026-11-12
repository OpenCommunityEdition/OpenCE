"""The host validates a client's deck word in its own vehicle's body frame."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure
from harness.riding import engine, without_includes  # noqa: E402

NETWORK = "port/linux/game/network_distributed.c"
RIDING = "port/linux/game/vehicle_riding.c"
CASES = ["host-deck-lagging", "host-deck-relative-speed", "host-deck-advance-refused", "host-deck-anchor-walk",
         "host-deck-node-forgery", "host-deck-temporaries", "host-deck-host-moved"]
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
    "deck-taken-in-world": ("network", [("target.z -= base.z - object->object.position.z;",
         "target.z -= base.z - object->object.position.z;\n\ttarget = state->position;")], "host-deck-lagging"),
    "bound-world-speed": ("network", [("subtract_vectors3d(velocity, frame_velocity, &relative);",
        "relative = *velocity;")], "host-deck-relative-speed"),
    "deck-velocity-unclamped": ("riding", [("if (speed > 0.0f)", "if (FALSE)")], "host-deck-advance-refused"),
    "commit-before-checks": ("network", [("	if (!((vehicle_riding_is_riding(&host) ?",
        "	distributed_apply_state(unit_index, state, &target, &world_velocity, 0.0f, HOST_BLEND_DISTANCE, 0.0f, 0.0f);\n\tif (!((vehicle_riding_is_riding(&host) ?")], "host-deck-temporaries"),
    "ceiling-in-node": ("network", [("(vehicle_riding_is_riding(&host) ? place.z - ground_place_height",
        "(vehicle_riding_is_riding(&host) ? deck->place.z - ground_place_height")], "host-deck-node-forgery"),
    "deck-anchor-unchecked": ("network", [("else if (ticks <= 0 || !distributed_on_foot_move_valid(bound, &anchor, &target, ticks))",
        "else if (FALSE)")], "host-deck-anchor-walk"),
    "host-moved-unchecked": ("network", [("if (!distributed_on_foot_move_valid(bound, &taken_host, &host_now, now - accepted.taken_host_time))",
        "if (FALSE)")], "host-deck-host-moved"),
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
