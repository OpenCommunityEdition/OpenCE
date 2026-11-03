"""A relayed action's bytes on the wire (network_distributed.c): no byte of a mutator's script's buttons when none is
set, as in every game without a script, and one, flagged, when one is."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run, structure  # noqa: E402

CASES = ["none", "buttons"]

# a fault in the code, and the case that must catch it
NEGATIVE_CONTROLS = {
    "always-sent": (("\tif (relayed->script_buttons)\n\t{\n", "\t{\n"), "none"),
    "never-flagged": (("\t\tSET_FLAG(parts, DISTRIBUTED_RELAYED_ACTION_SCRIPT_BUTTONS_BIT, TRUE);\n", ""), "buttons"),
}


def generated(fault=None):
    netcode = read("port/linux/game/network_distributed.c")
    config = "#define DISTRIBUTED_INPUT_HISTORY %d\n" % constant(read("source/game/player_queues_new.h"),
                                                                "DISTRIBUTED_INPUT_HISTORY")
    config += "#define DISTRIBUTED_RELAYED_ACTION_SCRIPT_BUTTONS_BIT %d\n" % constant(
        netcode, "DISTRIBUTED_RELAYED_ACTION_SCRIPT_BUTTONS_BIT")
    config += "enum { DISTRIBUTED_RELAYED_ACTION_MINIMUM_SIZE = %d, DISTRIBUTED_RELAYED_ACTION_MAXIMUM_SIZE = %s };\n" % (
        constant(netcode, "DISTRIBUTED_RELAYED_ACTION_MINIMUM_SIZE"),
        "15 + 2 * (DISTRIBUTED_INPUT_HISTORY - 1)")
    text = structure(netcode, "distributed_relayed_action") + "\n"
    for name in ("distributed_put", "distributed_take", "distributed_relayed_action_write",
                 "distributed_relayed_action_read"):
        code = function(netcode, name)
        if fault and name == "distributed_relayed_action_write":
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


def test_maximum_size():
    """the most bytes an action takes, as the netcode declares it: the buttons' byte and every tick's buttons"""
    assert "DISTRIBUTED_RELAYED_ACTION_MAXIMUM_SIZE = 15 + 2 * (DISTRIBUTED_INPUT_HISTORY - 1)," in read(
        "port/linux/game/network_distributed.c")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("relayed_action", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("relayed_action", generated(fault)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
