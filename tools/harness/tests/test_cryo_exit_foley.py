"""The real cryo_exit_foley_move (cryo_exit_foley.c): a10's short cryo scene (fast_setup, off Normal and in co-op)
started the foley of the Master Chief climbing out of his cryo tube, then opened the tube and had him climb out 45 ticks
later; on Normal the foley starts 45 ticks after he begins to climb, where its steps fall with his. The foley's start
is moved to 45 ticks after the climb out, the sleep it falls in split in two, and nothing else moves in time; a begin
laid out otherwise, already moved, or without room for the split, is left as it is."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["fast setup", "exact", "moved once", "normal", "another begin", "full", "no begin"]

FUNCTIONS = ["script_node_get", "script_string_get", "script_call_name_node", "script_node_calls",
             "script_node_starts_foley", "script_sleep_ticks", "script_sleep_copy", "cryo_exit_foley_move"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the foley started with the climb out (the first try at fix-cryo-exit-foley)
    "no-delay": (("ticks_before = (short)(CRYO_EXIT_FOLEY_DELAY - elapsed);", "ticks_before = 0;"), "fast setup"),
    # the sleep split not shortened: everything after the foley later
    "rest-not-shortened": (("\t\tsleep_ticks->short_value -= ticks_before;\n", ""), "fast setup"),
    # any looping sound's start moved
    "any-sound": (("return string && !strcmp(string, CRYO_EXIT_FOLEY_NAME);", "return string != NULL;"),
                  "another begin"),
    # moved whatever follows it (Normal's, where the tube opened long before)
    "no-open": (('if (!script_node_calls(syntax, strings, string_bytes, script_node_get(syntax, '
                 'foley->next_node_index), "unit_open"))', "if (0)"), "normal"),
    # moved past what may take time
    "past-anything": (('\t\telse if (!script_node_calls(syntax, strings, string_bytes, expression, "if"))\n'
                       '\t\t{\n\t\t\tbreak;\n\t\t}\n', ""), "another begin"),
}


def generated(fault=None):
    source = read("port/linux/game/cryo_exit_foley.c").replace("\r\n", "\n")
    hs = read("source/hs/hs.h").replace("\r\n", "\n")
    definitions = read("source/hs/hs_scenario_definitions.h").replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define (?:A10_SCENARIO_NAME|FAST_SETUP_SCRIPT_NAME|CRYO_EXIT_FOLEY_\w+) .*$",
                                   source, re.M))
    code = "\n\n".join([enum_with(hs, "_hs_function_name"), structure(definitions, "hs_syntax_node"),
                        enum_with(source, "_hs_syntax_node_variable_bit"), defines] +
                       [function(source, name) for name in FUNCTIONS]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_scripts_move_it():
    """a10's scripts, as they are made ready and checked, have fast_setup's foley moved."""
    hs = read("source/hs/hs.c").replace("\r\n", "\n")
    postprocess = function(hs, "hs_scenario_postprocess")
    check = postprocess.index("hs_scenario_functions_check(scenario);")
    assert postprocess.index("cryo_exit_foley_scripts_loaded(scenario, hs_syntax_data);", check) > check
    source = read("port/linux/game/cryo_exit_foley.c")
    assert '#define A10_SCENARIO_NAME "levels\\\\a10\\\\a10"' in source
    assert '#define FAST_SETUP_SCRIPT_NAME "fast_setup"' in source
    assert "#define CRYO_EXIT_FOLEY_DELAY 45" in source


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("cryo_exit_foley", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("cryo_exit_foley", generated(fault)), case)
    assert status == CHECK_FAILED, f"cryo_exit_foley_move with a fault ({control}) passed '{case}'"
