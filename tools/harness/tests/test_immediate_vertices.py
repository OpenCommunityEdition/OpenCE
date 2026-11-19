"""The real D3DDevice_Begin, immediate_emit, immediate_first_write and set_attribute (d3d8_gl.c), and End's first
vertex: each vertex is given the registers written since Begin as they stood at its register 0, including those
first written after it, and the first vertex every register."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["decal", "late-register", "written-once", "unwritten", "draws", "all-registers"]

# faults in the code (each a list of replacements) and the case that must catch them
NEGATIVE_CONTROLS = {
    # the vertices before a register's first write not given its earlier value
    "no-backfill": ([("for (vertex = 0; vertex < device.immediate_count; vertex++)\n\t\tmemcpy(",
                      "for (vertex = 0; vertex < 0; vertex++)\n\t\tmemcpy(")], "late-register"),
    # each vertex given only the first register written
    "first-register-only": ([("for (slot = 0; slot < device.immediate_register_count; slot++)",
                              "for (slot = 0; slot < 1 && slot < device.immediate_register_count; slot++)")],
                            "decal"),
    # the registers of the draw before kept at Begin
    "list-kept": ([("\tdevice.immediate_register_count = 0;\n", "")], "draws"),
    # the first vertex not given the registers not written
    "unwritten-not-given": ([("if (!(device.immediate_written & (1UL << index)))\n\t\t\tmemcpy(",
                              "if (0)\n\t\t\tmemcpy(")], "unwritten"),
    # the vertex completed before the write of register 0, not after it
    "emit-before-write": ([("\tif (device.immediate_active && (emit || reg == 0))\n\t\timmediate_emit();\n", ""),
                           ("\tdevice.attributes[reg][0] = a;",
                            "\tif (device.immediate_active && (emit || reg == 0))\n\t\timmediate_emit();\n"
                            "\tdevice.attributes[reg][0] = a;")], "decal"),
}


def generated(faults=()):
    renderer = read("port/linux/src/d3d8_gl.c").replace("\r\n", "\n")
    end = function(renderer, "D3DDevice_End")
    unwritten = end[end.index("\t/* (the registers not written"):end.index("\t/* (the fixed-function pipeline's")]
    code = "\n".join([
        function(renderer, "D3DDevice_Begin"),
        function(renderer, "immediate_emit"),
        function(renderer, "immediate_first_write"),
        function(renderer, "set_attribute"),
        function(renderer, "D3DDevice_SetVertexData4f"),
        "static void immediate_end_unwritten(void)\n{\n\tunsigned long index;\n\n" + unwritten + "}\n",
    ])
    for before, after in faults:
        code = mutated(code, before, after)
    return (("under_test.inc", code + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("immediate_vertices", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    faults, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("immediate_vertices", generated(tuple(faults))), case)
    assert status == CHECK_FAILED, f"immediate mode with a fault ({control}) passed '{case}': the test cannot see it"
