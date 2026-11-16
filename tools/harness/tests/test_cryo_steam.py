"""The real cryo_steam_vents_end_before_fade and cryo_steam_wake_without_steam (cryo_steam.c): a10's opening cinematic
destroyed its cryo tube's four vents after its fade to white, so that their steam drifted into the next scene, the
Chief's waking; and the short cryo scene that stands for the tutorial (fast_setup) made the tube's steam as the tube
opened, one vent left on for the rest of the level. The vents are still made 90 ticks into the tube's shot, as on the
Xbox, and destroyed as the fade begins, the white held 45 ticks; fast_setup's steam is not made. Scenes laid out
otherwise, or already changed, are left as they are."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["vents", "vents once", "other scenes", "wake", "wake once", "other wakes"]

FUNCTIONS = ["script_node_get", "script_string_get", "script_call_name_node", "script_node_calls",
             "script_call_argument", "script_sleep_ticks", "script_begin_name_node", "cryo_steam_name_index",
             "cryo_steam_cooperative_create", "cryo_steam_vents_end_before_fade", "cryo_steam_wake_without_steam"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # a scene whose vents are made after the fade changed too
    "after-fade": (("!(vents < fade && fade + 1 < vents_end)", "!(fade + 1 < vents_end)"), "other scenes"),
    # the white held as it was
    "white-kept": (("\twhite->short_value = CRYO_STEAM_WHITE_TICKS;\n", ""), "vents"),
    # the vents' end left after the fade
    "end-kept": (("\tscript_node_get(syntax, expressions[fade - 1])->next_node_index = expressions[vents_end];\n", ""),
                 "vents"),
    # any white changed
    "any-white": (("!white || white->short_value != CRYO_STEAM_MAP_WHITE_TICKS", "!white"), "other scenes"),
    # the steam taken out of a scene without all four
    "any-count": (("\t\tif (!first_tube_found[name_index] || !second_tube_found[name_index])\n",
                   "\t\tif (0)\n"), "other wakes"),
    # the second tube's steam under any condition
    "any-condition": (('!script_node_calls(syntax, strings, string_bytes, condition, "game_is_cooperative") ||', ""),
                      "other wakes"),
}


def generated(fault=None):
    source = read("port/linux/game/cryo_steam.c").replace("\r\n", "\n")
    hs = read("source/hs/hs.h").replace("\r\n", "\n")
    definitions = read("source/hs/hs_scenario_definitions.h").replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define (?:CRYO_\w+) .*$", source, re.M))
    names = "\n".join(re.findall(r"^static char const \*const cryo_steam_\w+\[\] =\n\{.*?^\};", source, re.M | re.S))
    code = "\n\n".join([enum_with(hs, "_hs_function_name"), structure(definitions, "hs_syntax_node"),
                        enum_with(source, "_hs_syntax_node_variable_bit"), defines, names] +
                       [function(source, name) for name in FUNCTIONS]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_scripts_change_them():
    """a10's scripts, as they are made ready and checked, have the cryo bay's steam changed, after the climb's foley."""
    hs = read("source/hs/hs.c").replace("\r\n", "\n")
    postprocess = function(hs, "hs_scenario_postprocess")
    foley = postprocess.index("cryo_exit_foley_scripts_loaded(scenario, hs_syntax_data);")
    assert postprocess.index("cryo_steam_scripts_loaded(scenario, hs_syntax_data);", foley) > foley
    source = read("port/linux/game/cryo_steam.c").replace("\r\n", "\n")
    loaded = function(source, "cryo_steam_scripts_loaded")
    assert '#define A10_SCENARIO_NAME "levels\\\\a10\\\\a10"' in source
    assert '#define CRYO_SCRIPT_NAME "cryo"' in source
    assert '#define FAST_SETUP_SCRIPT_NAME "fast_setup"' in source
    assert "csstrcmp(scenario_name, A10_SCENARIO_NAME)" in loaded
    assert loaded.index("CRYO_SCRIPT_NAME") < loaded.index("cryo_steam_vents_end_before_fade(")
    assert loaded.index("FAST_SETUP_SCRIPT_NAME") < loaded.index("cryo_steam_wake_without_steam(")


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("cryo_steam", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("cryo_steam", generated(fault)), case)
    assert status == CHECK_FAILED, f"cryo_steam with a fault ({control}) passed '{case}'"
