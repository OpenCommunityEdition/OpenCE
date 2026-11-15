"""The real x20_chip_from_console (x20_chip.c): a10's bridge cinematic made Cortana's chip in Captain Keyes's right
hand behind him and moved it to his left, which he holds up before the camera, 30 ticks into the shot: the chip came
into his hand out of nowhere. The pipe's taking, the chip's making and its move to his left hand go to right after
her projection is destroyed, before the 50 ticks the script waits; the script's earlier destroy of the projection is
left alone, and a script laid out otherwise, or already changed, is not changed."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["handover", "once", "other scenes"]

FUNCTIONS = ["script_node_get", "script_string_get", "script_call_name_node", "script_node_calls",
             "script_call_argument", "script_sleep_ticks", "script_begin_name_node", "x20_chip_handover_at",
             "x20_chip_from_console"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the left hand's move left after the shot's sleep
    "left-hand-kept": (("\t\t\tscript_node_get(syntax, expressions[last])->next_node_index = expressions[left_detach];\n"
                        "\t\t\tscript_node_get(syntax, expressions[left_attach])->next_node_index = expressions[sleep];\n",
                        "\t\t\tscript_node_get(syntax, expressions[last])->next_node_index = expressions[sleep];\n"),
                       "handover"),
    # the first destroy of the projection taken for the handover's
    "first-destroy": (("\t\tif (x20_chip_handover_at(syntax, strings, string_bytes, expressions, destroyed))",
                       "\t\tif (x20_chip_handover_at(syntax, strings, string_bytes, expressions, destroyed) || "
                       "destroyed == 1)"), "handover"),
}


def generated(fault=None):
    source = read("port/linux/game/x20_chip.c").replace("\r\n", "\n")
    hs = read("source/hs/hs.h").replace("\r\n", "\n")
    definitions = read("source/hs/hs_scenario_definitions.h").replace("\r\n", "\n")
    defines = "\n".join(re.findall(r"^#define (?:X20_\w+) .*$", source, re.M))
    code = "\n\n".join([enum_with(hs, "_hs_function_name"), structure(definitions, "hs_syntax_node"),
                        enum_with(source, "_hs_syntax_node_variable_bit"), defines] +
                       [function(source, name) for name in FUNCTIONS]) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code),)


def test_scripts_change_it():
    """a10's scripts, as they are made ready, have final_words changed, once their functions are checked."""
    postprocess = function(read("source/hs/hs.c").replace("\r\n", "\n"), "hs_scenario_postprocess")
    checked = postprocess.index("hs_scenario_functions_check(scenario);")
    assert postprocess.index("x20_chip_scripts_loaded(scenario, hs_syntax_data);", checked) > checked
    source = read("port/linux/game/x20_chip.c").replace("\r\n", "\n")
    assert '#define A10_SCENARIO_NAME "levels\\\\a10\\\\a10"' in source
    assert '#define X20_CHIP_SCRIPT_NAME "final_words"' in source


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("x20_chip", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("x20_chip", generated(fault)), case)
    assert status == CHECK_FAILED, f"x20_chip with a fault ({control}) passed '{case}'"
