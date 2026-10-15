"""The real parse_unicode_string (draw_string.c): "|n" ends a line, any other '|' is a character and the one after it is
read, and a string ending in '|' ends at its terminator, not past it."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["bar-n-ends-a-line", "bar-is-a-character", "bar-at-the-end"]

# a fault in parse_unicode_string and the case that must catch it
NEGATIVE_CONTROLS = {
    "takes-the-next-character": (("if (string[state->string_index] == 'n')\n\t\t{\n\t\t\tstate->string_index++;",
                                  "if (string[state->string_index++] == 'n')\n\t\t{"), "bar-at-the-end"),
    "bar-keeps-the-last-result": (("\t\telse\n\t\t{\n\t\t\tstate->result = _parsed_character;\n\t\t}\n", ""),
                                  "bar-is-a-character"),
}


def generated(fault=None):
    source = read("source/text/draw_string.c")
    config = enum_with(source, "_parsed_end_of_string") + "\n" + structure(source, "parse_string_state") + "\n"
    code = function(source, "parse_unicode_string")
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code + "\n"))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("unicode_line_breaks", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("unicode_line_breaks", generated(fault)), case)
    assert status == CHECK_FAILED, f"draw_string.c with a fault ({control}) passed '{case}': the test cannot see it"
