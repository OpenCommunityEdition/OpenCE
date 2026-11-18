"""The real x10_warthogs_wake_once, x10_warthogs_run_shortened and x10_warthogs_driver_placed (x10_warthogs.c): a10's
hangar scene, Sergeant Johnson's speech, woke its warthog's script (x10_hog_4) on Legendary twice, the second wake ending
the script's sleep, so the warthog was destroyed before it was seen; the script slept for the warthog's whole run, which
ends in a crash into the hangar's far wall; and the scene's two warthog drivers were placed with Johnson's variant.
Legendary's own wake is taken out, the sleep made one of 120 ticks, and the drivers placed with no variant; scripts laid
out otherwise, already changed, and other placements are left as they are."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["legendary", "woken once", "other wakes", "run", "shortened once", "other runs", "drivers"]

FUNCTIONS = ["script_node_get", "script_string_get", "script_call_name_node", "script_node_calls",
             "script_node_calls_with", "script_begin_name_node", "x10_warthogs_wake_once",
             "x10_warthogs_run_shortened", "x10_warthogs_driver_placed"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # Legendary's wake left in (the map's)
    "wake-kept": (("\t\t\t\t\tprevious->next_node_index = inner->next_node_index;\n", ""), "legendary"),
    # an if's wake taken out with no wake after the if
    "no-wake-after": (('script_node_calls_with(syntax, strings, string_bytes, next, "wake", HOG_SCRIPT_NAME))',
                       "TRUE)"), "other wakes"),
    # any script's wake taken out of the if
    "any-wake": (('script_node_calls_with(syntax, strings, string_bytes, inner, "wake", HOG_SCRIPT_NAME)',
                  'script_node_calls(syntax, strings, string_bytes, inner, "wake")'), "other wakes"),
    # the sleep's ticks left at none
    "no-ticks": (("\t\t\tticks->short_value = X10_HOG_RUN_TICKS;\n", ""), "run"),
    # the call's node left a call (its name read as ticks)
    "still-a-call": (("\t\t\tSET_FLAG(ticks->flags, _hs_syntax_node_primitive_bit, TRUE);\n", ""), "run"),
    # a wait before the run changed
    "before-play": (("else if (played && script_node_calls(", "else if ((played || TRUE) && script_node_calls("), "other runs"),
    # a wait for any vehicle's recording changed
    "any-vehicle": (('script_node_calls_with(syntax, strings, string_bytes, ticks, "recording_time", HOG_VEHICLE_NAME)',
                     'script_node_calls(syntax, strings, string_bytes, ticks, "recording_time")'), "other runs"),
    # a driver of any variant placed with none
    "any-variant": (("!name || !object || object->variant_number != JOHNSON_VARIANT", "!name || !object"),
                    "drivers"),
    # Johnson placed with no variant too
    "any-name": (("if (!strcmp(name, x10_warthog_driver_names[driver_index]))", "if (1)"), "drivers"),
}


def generated(fault=None):
    source = read("port/linux/game/x10_warthogs.c").replace("\r\n", "\n")
    hs = read("source/hs/hs.h").replace("\r\n", "\n")
    definitions = read("source/hs/hs_scenario_definitions.h").replace("\r\n", "\n")
    scenario = read("source/scenario/scenario_definitions.h").replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define (?:HOG_\w+|X10_HOG_RUN_TICKS|JOHNSON_VARIANT) .*$", source, re.M))
    names = re.search(r"^static char const \*const x10_warthog_driver_names\[\] =\n\{.*?^\};", source, re.M | re.S)
    code = "\n\n".join([enum_with(hs, "_hs_function_name"), structure(definitions, "hs_syntax_node"),
                        structure(scenario, "scenario_object_datum"),
                        enum_with(source, "_hs_syntax_node_variable_bit"), defines, names.group(0)] +
                       [function(source, name) for name in FUNCTIONS]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_scripts_change_them():
    """a10's scripts, as they are made ready and checked, have the hangar scene's warthog and drivers changed."""
    hs = read("source/hs/hs.c").replace("\r\n", "\n")
    postprocess = function(hs, "hs_scenario_postprocess")
    check = postprocess.index("hs_scenario_functions_check(scenario);")
    assert postprocess.index("x10_warthogs_scripts_loaded(scenario, hs_syntax_data);", check) > check
    source = read("port/linux/game/x10_warthogs.c").replace("\r\n", "\n")
    loaded = function(source, "x10_warthogs_scripts_loaded")
    assert '#define A10_SCENARIO_NAME "levels\\\\a10\\\\a10"' in source
    assert '#define HANGAR_SCRIPT_NAME "hangar_2"' in source
    assert '#define HOG_SCRIPT_NAME "x10_hog_4"' in source
    assert '#define HOG_VEHICLE_NAME "x10_warthog_2"' in source
    assert "#define X10_HOG_RUN_TICKS 120" in source
    assert "#define JOHNSON_VARIANT 100" in source
    assert "csstrcmp(scenario_name, A10_SCENARIO_NAME)" in loaded
    assert loaded.index("HANGAR_SCRIPT_NAME") < loaded.index("x10_warthogs_wake_once(")
    assert loaded.index("HOG_SCRIPT_NAME") < loaded.index("x10_warthogs_run_shortened(")
    assert "scenario_get_object_type_scenario_datums(scenario, _object_type_biped, &element_size)" in loaded
    assert "x10_warthogs_driver_placed(" in loaded


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("x10_warthogs", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("x10_warthogs", generated(fault)), case)
    assert status == CHECK_FAILED, f"x10_warthogs with a fault ({control}) passed '{case}'"
