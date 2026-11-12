"""The deck part of a unit's state on the wire (port/linux/game/network_distributed.c's distributed_unit_state_write
and distributed_unit_state_read, and port/linux/game/vehicle_riding.c's checks of it): docs port/linux/NETCODE.md.

    python -m pytest -q tools/harness/tests/test_network_deck.py
"""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402
from test_vehicle_riding import engine, without_includes  # noqa: E402

NETWORK = "port/linux/game/network_distributed.c"
RIDING = "port/linux/game/vehicle_riding.c"
CASES = ["deck-roundtrip", "deck-truncate", "deck-invalid-whole", "deck-zero-salt", "deck-not-finite",
         "deck-grammar", "deck-standing-verdict",
         "deck-cabin-bit"]
NETWORK_FUNCTIONS = ("distributed_put", "distributed_take", "distributed_vector_pack", "distributed_vector_unpack",
                     "distributed_deck_put", "distributed_deck_take", "distributed_unit_state_write",
                     "distributed_unit_state_read")
RIDING_FUNCTIONS = ("vehicle_riding_deck_vehicle_named", "vehicle_riding_deck_values_valid", "vehicle_riding_deck_valid",
                    "vehicle_riding_point_to_local", "vehicle_riding_is_riding", "vehicle_riding_cabin_verdict",
                    "vehicle_riding_deck_make")

# a fault in a source (network or riding), and the case that must catch it
NEGATIVE_CONTROLS = {
    "deck-size-short": ("network", "DISTRIBUTED_DECK_SIZE = 4 + 2 + 12 + 6 + 1 + 1 + 2,",
                        "DISTRIBUTED_DECK_SIZE = 4 + 2 + 12 + 6 + 1 + 1,", "deck-roundtrip"),
    "deck-maximum-not-grown": ("network", "(12 + 3 * 6) + DISTRIBUTED_DECK_SIZE + 2", "(12 + 3 * 6) + 2",
                               "deck-roundtrip"),
    "deck-grammar-ignored": ("network", "if (TEST_FLAG(parts, _distributed_unit_part_deck_bit) &&\n\t\t!(",
                             "if (FALSE &&\n\t\t!(", "deck-grammar"),
    "deck-writer-order": ("network",
                          "\tcursor = distributed_put(cursor, &deck->node_index, sizeof(deck->node_index));\n"
                          "\tcursor = distributed_put(cursor, &deck->place, sizeof(deck->place));\n",
                          "\tcursor = distributed_put(cursor, &deck->place, sizeof(deck->place));\n"
                          "\tcursor = distributed_put(cursor, &deck->node_index, sizeof(deck->node_index));\n",
                          "deck-roundtrip"),
    "deck-peak-unsent": ("network", "\treturn distributed_put(cursor, &peak.i, sizeof(peak.i));", "\treturn cursor;",
                          "deck-roundtrip"),
    "deck-air-bits-dropped": ("network", "\tcursor = distributed_put(cursor, &deck->flags, sizeof(deck->flags));",
                               "\t/* dropped air verdict bits */", "deck-roundtrip"),
    "deck-peak-unread": ("network", " ||\n\t\t!distributed_take(cursor, end, &peak.i, sizeof(peak.i)))", ")",
                         "deck-roundtrip"),
    "deck-invalid-kept": ("network", "\t*present = vehicle_riding_deck_vehicle_named(taken.vehicle_index) &&\n"
                          "\t\tvehicle_riding_deck_valid(&taken, biped_riding_vehicle_node_count(taken.vehicle_index));",
                          "\t*present = TRUE;", "deck-invalid-whole"),
    "deck-partial": ("network", "\tif (*present)\n\t\t*deck = taken;", "\t*deck = taken;", "deck-invalid-whole"),
    "deck-salt-after-lookup": ("network", "\t*present = vehicle_riding_deck_vehicle_named(taken.vehicle_index) &&\n\t\t",
                               "\t*present = ", "deck-zero-salt"),
    "deck-nodes-unchecked": ("network", "biped_riding_vehicle_node_count(taken.vehicle_index))", "3)",
                             "deck-invalid-whole"),
    "deck-not-finite-written": ("network", "if (state->deck_present && vehicle_riding_deck_values_valid(&state->deck))",
                                "if (state->deck_present)", "deck-not-finite"),
    "deck-verdict-air-only": ("riding", "\tmade.flags |= (byte)(vehicle_riding_cabin_verdict(cabin, state, air, FALSE) << VEHICLE_RIDING_DECK_VERDICT_SHIFT);\n", "\tif (!vehicle_riding_is_riding(state))\n\t\tmade.flags |= (byte)(vehicle_riding_cabin_verdict(cabin, state, air, FALSE) << VEHICLE_RIDING_DECK_VERDICT_SHIFT);\n", "deck-standing-verdict"),
    "deck-ticks-unchecked": ("riding", "deck->ticks >= 1 && deck->ticks <= RIDING_TICKS &&", "", "deck-invalid-whole"),
    "deck-cabin-bit-any-ticks": ("riding", "\t\t(!TEST_FLAG(deck->flags, _vehicle_riding_deck_cabin_bit) || deck->ticks != RIDING_TICKS) &&\n", "", "deck-cabin-bit"),
    "deck-high-bits-allowed": ("riding", "\t\t(deck->flags >> (_vehicle_riding_deck_cabin_bit + 1)) == 0 &&\n", "", "deck-cabin-bit"),
    "deck-cabin-bit-no-verdict": ("riding", "\t\t(!TEST_FLAG(deck->flags, _vehicle_riding_deck_cabin_bit) || verdict != _vehicle_riding_enclosure_unknown) &&\n", "", "deck-cabin-bit"),
}


def generated(control=None):
    sources = {"network": read(NETWORK), "riding": read(RIDING)}
    if control:
        owner, before, after, _ = NEGATIVE_CONTROLS[control]
        sources[owner] = mutated(sources[owner], before, after)
    network = sources["network"]
    header = read("port/linux/game/network_distributed.h")
    types = engine() + "\n" + without_includes(read("port/linux/game/vehicle_riding.h")) + "\n"
    types += enum_with(read("source/game/players.h"), "_player_powerup_active_camouflage") + "\n"
    types += ("#define DISTRIBUTED_INPUT_HISTORY "
              f"{constant(read('source/game/player_queues_new.h'), 'DISTRIBUTED_INPUT_HISTORY')}\n")
    types += "#define NO_PLAYER 0xFF\n"
    for name in ("DISTRIBUTED_UNIT_SCALE", "DISTRIBUTED_VELOCITY_SCALE"):
        types += re.search(r"^#define " + name + r" .*$", header, re.M).group(0) + "\n"
    types += structure(header, "distributed_vector") + "\n"
    types += enum_with(network, "_distributed_pose_facing,") + "\n"
    for member in ("_distributed_unit_alive_bit", "_distributed_unit_camouflaged_bit", "_distributed_unit_part_riding_bit",
                   "DISTRIBUTED_UNIT_STATE_MINIMUM_SIZE"):
        types += enum_with(network, member) + "\n"
    types += structure(network, "distributed_unit_state") + "\n"
    code = "".join(function(sources["riding"], name) + "\n" for name in RIDING_FUNCTIONS)
    code += "".join(function(network, name) + "\n" for name in NETWORK_FUNCTIONS)
    return (("types.inc", types), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("network_deck", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    status, output = run(build("network_deck", generated(control)), NEGATIVE_CONTROLS[control][-1])
    assert status == CHECK_FAILED, f"a fault ({control}) passed '{NEGATIVE_CONTROLS[control][-1]}': the test cannot see it"


if __name__ == "__main__":
    sys.exit(pytest.main(["-q", __file__]))
