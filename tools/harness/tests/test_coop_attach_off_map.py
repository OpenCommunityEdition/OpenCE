"""A co-op client skips the host's attach of an item that is held off the map."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["on-map", "off-map"]


def generated(fault=None):
    coop = read("port/linux/game/network_coop.c")
    objects = read("source/objects/objects.c")
    code = "\n".join([
        function(objects, "objects_scripting_attach"),
        function(coop, "client_apply_attach"),
    ])
    if fault:
        code = mutated(code, *fault)
    declarations = "\n".join([
        enum_with(read("source/objects/objects.h"), "_object_connected_to_map_bit,"),
        enum_with(read("source/objects/object_types.h"), "_object_mask_all ="),
        enum_with(coop, "_coop_attach,"),
        structure(coop, "distributed_coop_event"),
    ])
    return (("declarations.inc", declarations + "\n"), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_attach(case):
    status, output = run(build("coop_attach_off_map", generated()), case)
    assert status == 0, output


def test_negative_control():
    # without the off-map check
    fault = ("if (event->type == _coop_attach && !TEST_FLAG(", "if (FALSE && !TEST_FLAG(")
    status, output = run(build("coop_attach_off_map", generated(fault)), "off-map")
    assert status == CHECK_FAILED, output
