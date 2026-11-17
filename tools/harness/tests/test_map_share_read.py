"""How much a map sharing connection (map_share.c) reads in a call: all its socket has, a download's blocks included,
not one input buffer (which held a download to about a block a frame), and no more than MAXIMUM_READS_PER_CALL buffers
at once."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["reads-all-waiting", "reads-bounded"]

# a fault in connection_read, and the case that must catch it
NEGATIVE_CONTROLS = {
    "one-buffer-a-call": (("(result <= 0 || ++reads >= MAXIMUM_READS_PER_CALL)", "1"), "reads-all-waiting"),
    "unbounded": (("++reads >= MAXIMUM_READS_PER_CALL", "0"), "reads-bounded"),
}


def generated(fault=None):
    source = read("port/linux/game/map_share.c")
    config = "#define P2P_MAP_SHARE_PORT %d\n" % constant(read("port/linux/src/p2p.h"), "P2P_MAP_SHARE_PORT")
    config += "#define HALO_PORT_MAXIMUM_NETWORK_MACHINES %d\n" % constant(
        read("port/linux/include/halo_port_limits.h"), "HALO_PORT_MAXIMUM_NETWORK_MACHINES")
    config += enum_with(read("port/linux/game/map_share.h"), "MAP_SHARE_HASH_BYTES") + "\n"
    config += enum_with(read("source/bungie_net/network/transport.h"), "_transport_result_operation_would_block")
    config += "\n" + enum_with(source, "MAXIMUM_READS_PER_CALL") + "\n" + enum_with(source, "_frame_block") + "\n"
    code = function(source, "get_long") + "\n"
    read_code = function(source, "connection_read")
    if fault:
        read_code = mutated(read_code, *fault)
    return (("config.inc", config), ("under_test.inc", code + read_code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("map_share_read", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("map_share_read", generated(fault)), case)
    assert status == CHECK_FAILED, f"map_share.c with a fault ({control}) passed '{case}': the test cannot see it"
