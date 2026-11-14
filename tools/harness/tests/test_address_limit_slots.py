"""The real network_link_address_limit (network_link.c), a dedicated server's server.max_machines_per_address, over a
game whose machines are in the slots of their indices with gaps: every slot is counted, not the first machine_count,
which missed a machine after a gap and let a second machine of one address in."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["gap", "far-slots", "self", "unlimited"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the first machine_count slots alone (before fix-address-limit-slots)
    "first-machine-count": (("for (other = 0; other < HALO_PORT_MAXIMUM_NETWORK_MACHINES; other++)",
                             "for (other = 0; other < game->machine_count && other < HALO_PORT_MAXIMUM_NETWORK_MACHINES;"
                             " other++)"), "gap"),
    # a machine's own slot counted
    "self-counted": (("slot != machine_index &&", ""), "self"),
    # the limit's own machines let in
    "limit-plus-one": (("if (count < limit)", "if (count <= limit)"), "far-slots"),
}


def generated(fault=None):
    limits = read("port/linux/include/halo_port_limits.h")
    config = "\n".join([
        "#define HALO_PORT_MAXIMUM_NETWORK_MACHINES %d" % constant(limits, "HALO_PORT_MAXIMUM_NETWORK_MACHINES"),
        enum_with(read("source/networking/network_game_protocol.h"), "_rejection_code_address_full"),
        structure(read("source/networking/network_game_manager.h"), "network_machine"),
    ]) + "\n"
    code = function(read("port/linux/game/network_link.c"), "network_link_address_limit") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("address_limit_slots", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("address_limit_slots", generated(fault)), case)
    assert status == CHECK_FAILED, f"the limit with a fault ({control}) passed '{case}': the test cannot see it"
