"""A rested hull remains current when riders change from deck to world coordinates."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run


def generated(control=False):
    source = read("port/linux/game/network_objects.c")
    if control:
        source = mutated(source, "vehicle_riding_enabled() ? resting_start : objects_host_resting_cursor",
                         "objects_host_resting_cursor")
    config = ""
    for name in ("RESTING_STATES_PER_TICK", "REST_STATE_REPEATS", "REST_STATE_REPEAT_TICKS"):
        config += f"#define {name} {constant(source, name)}\n"
    config += f"#define MAXIMUM_TRACKED_OBJECTS {constant(read('port/linux/include/halo_port_capacity.h'), 'HALO_PORT_MAXIMUM_OBJECTS_PER_MAP')}\n"
    config += enum_with(source, "_host_state_to_all") + "\n"
    code = function(source, "distributed_host_rest_state_due") + "\n"
    code += function(source, "distributed_host_send_states")
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", ["sparse-hulls", "all-rested", "maximum-pool", "sparse-maximum-pool",
                                  "empty-pool", "riding-off-original"])
def test_case(case):
    status, output = run(build("resting_state_scan", generated()), case)
    assert status == 0, output


def test_negative_control():
    status, output = run(build("resting_state_scan", generated(True)), "sparse-hulls")
    assert status == CHECK_FAILED and "check failed:" in output, f"{status}: {output}"
