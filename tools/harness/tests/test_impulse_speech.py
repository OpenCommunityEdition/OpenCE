"""The real unit_notify_impulse_sound (unit_dialogue.c): a script's line played from a dead unit, which takes no line
but its death's, halted a debug build on the assertion that the line was its current speech (c10's flashback plays
lines of Johnson and Bisenti once they are dead). A dead unit's current speech is left as it was; a live unit's is the
line, marked played, as before."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["alive", "dead"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the original: on to the assertion for a dead unit
    "no-skip": (("\tif (unit->unit.speech.current.sound_definition_index != sound_definition_index &&\n"
                 "\t\tTEST_FLAG(unit->object.damage_flags, _object_dead_bit))\n\t{\n\t\treturn;\n\t}\n", ""), "dead"),
    # every line skipped
    "always-skip": (("\tif (unit->unit.speech.current.sound_definition_index != sound_definition_index &&\n"
                     "\t\tTEST_FLAG(unit->object.damage_flags, _object_dead_bit))\n",
                     "\tif (TRUE)\n"), "alive"),
}


def generated(fault=None):
    source = read("source/units/unit_dialogue.c").replace("\r\n", "\n")
    code = function(source, "unit_notify_impulse_sound") + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("impulse_speech", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("impulse_speech", generated(fault)), case)
    assert status == CHECK_FAILED, f"impulse_speech with a fault ({control}) passed '{case}'"
