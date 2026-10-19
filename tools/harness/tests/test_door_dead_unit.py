"""The door's test of whether a unit is dead (device_machines.c) read a byte of a 16 bit word; it must read the word.
The big-endian run reads a byte at an offset as a big-endian machine does; the original's byte read is the negative
control and fails there."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, read, run  # noqa: E402

CASES = ["dead-unit-refused", "living-unit-opens"]


def generated(big_endian, old=False):
    source = read("source/devices/device_machines.c")
    at = source.index("TEST_FLAG(unit->object.damage_flags, _object_function2_active_bit)")
    condition = source[at:source.index(")", source.index("_object_function2_active_bit", at)) + 1]
    if old:
        condition = "TEST_FLAG(*((byte *)unit + 0xb6), _object_function2_active_bit)"
        if big_endian:
            # a byte at offset 0xb6 of the word is its high byte
            condition = "TEST_FLAG(*((byte *)unit + 0xb6 + 1 - 0) , _object_function2_active_bit)"
    code = "enum { _object_function2_active_bit = 2 };\nstatic int unit_can_open(struct unit_datum *unit)\n{\n\treturn !(" + condition + ");\n}\n"
    return (("under_test.inc", code),)


@pytest.mark.parametrize("big_endian", [False, True])
@pytest.mark.parametrize("case", CASES)
def test_case(case, big_endian):
    status, output = run(build("door_dead_unit", generated(big_endian)), case)
    assert status == 0, output


def test_little_endian_original_agrees():
    for case in CASES:
        status, output = run(build("door_dead_unit", generated(False, old=True)), case)
        assert status == 0, output


@pytest.mark.parametrize("case", ["dead-unit-refused"])
def test_negative_control_original_byte_read_on_big_endian(case):
    status, output = run(build("door_dead_unit", generated(True, old=True)), case)
    assert status == CHECK_FAILED, f"the original's byte read passed '{case}' as a big-endian machine runs it: {output}"
