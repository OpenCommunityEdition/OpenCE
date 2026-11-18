"""The real function_call_convert (custom_edition_scripts.c) over a Custom Edition map's compiled script: a call of
Halo PC's player_effect_set_max_vibrate is given this build's player_effect_set_max_rumble, and its name in the string
data is made this build's, unless another value shares that text or this build's name would not fit."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

SOURCE = "port/linux/game/custom_edition_scripts.c"
FUNCTIONS = ("script_string_get", "missing_value", "node_make_inert", "engine_global_missing", "function_find_by_name",
             "script_string_shared", "function_call_convert")

# each case, and the table it is built with: the sources', or one whose Halo PC name is shorter than this build's
CASES = {
    "alias-finds-rumble": None,
    "name-replaced": None,
    "any-case": None,
    "unknown-inert": None,
    "shared-text-inert": None,
    "longer-refused": ('"player_effect_set_max_vibrate"', '"player_effect_set_max_vib"'),
}

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # no alias: the call does nothing (as before the fix)
    "no-alias": (("function_index = function_find_by_name(name, &build_name);",
                  "function_index = hs_find_function_by_name(name);\n\tbuild_name = NULL;"), "alias-finds-rumble"),
    # the name in the string data kept: the game stops as it loads the map ("missing function")
    "name-kept": (("strcpy(conversion->strings + name_node->string_offset, build_name);", ""), "name-replaced"),
    # the Halo PC name matched in its own case only
    "case-matters": (("!csstrcasecmp(name, halo_pc_function_names", "!strcmp(name, halo_pc_function_names"),
                     "any-case"),
    # a text that another value shares written over
    "shared-written": (("build_name && script_string_shared(conversion, name_node->string_offset, (long)strlen(name))",
                        "FALSE"), "shared-text-inert"),
    # this build's name written however long it is
    "no-length-check": (("strlen(halo_pc_function_names[name_index][1]) <= strlen(name)", "TRUE"), "longer-refused"),
}


def generated(table=None, fault=None):
    source = read(SOURCE)
    names = re.search(r"^static char const \*const halo_pc_function_names\[\]\[2\] =\n\{.*?^\};", source,
                      re.M | re.S).group(0)
    if table:
        names = mutated(names, *table)
    hs = read("source/hs/hs.h")
    config = "\n".join([
        enum_with(hs, "_hs_function_name"),
        re.search(r"^#define hs_type_valid\(type\).*$", hs, re.M).group(0),
        structure(read("source/hs/hs_scenario_definitions.h"), "hs_syntax_node"),
        enum_with(source, "_hs_syntax_node_primitive_bit"),
        "#define HS_EXTERNAL_GLOBAL_DESIGNATOR_BIT %d" % constant(source, "HS_EXTERNAL_GLOBAL_DESIGNATOR_BIT"),
        "#define MAXIMUM_MISSING_NAME_MESSAGES %d" % constant(source, "MAXIMUM_MISSING_NAME_MESSAGES"),
        structure(source, "scripts_conversion"),
        names,
    ]) + "\n"
    code = "\n".join(function(source, name) for name in FUNCTIONS) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("custom_edition_max_vibrate", generated(CASES[case])), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("custom_edition_max_vibrate", generated(CASES[case], fault)), case)
    assert status == CHECK_FAILED, f"the relinker with a fault ({control}) passed '{case}': the test cannot see it"
