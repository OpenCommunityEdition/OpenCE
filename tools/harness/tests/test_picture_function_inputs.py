"""A frame drawn between ticks shows an object's functions from the inputs its tick used, not from what was written
into them since (an invisible weapon's values in its vehicle's).

    python tools/harness/tests/test_picture_function_inputs.py
"""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["tick-inputs", "tick-unchanged", "picture-moves", "additive-tick-inputs", "no-kept-inputs", "forgotten"]

# a fault in the code, and the case that must catch it
NEGATIVE_CONTROLS = {
    "picture-reads-the-object-now": (("real const *incoming = inputs->values;",
                                      "real const *incoming = object->object.incoming_function_values;"), "tick-inputs"),
    "additive-picture-reads-the-object-now": (("huh,\n\t\t\t\tincoming,\n\t\t\t\tobject->object.outgoing_function_values,",
                                               "huh,\n\t\t\t\tobject->object.incoming_function_values,\n"
                                               "\t\t\t\tobject->object.outgoing_function_values,"),
                                              "additive-tick-inputs"),
    "keeps-no-inputs": (("csmemcpy(inputs->values, object->object.incoming_function_values, sizeof(inputs->values));",
                         ""), "tick-inputs"),
    "ignores-the-object-index": (("if (inputs->object_index != object_index)\n\t\treturn;", ""), "no-kept-inputs"),
    "forgets-nothing": (("object_function_inputs[index].object_index = NONE;", ";"), "forgotten"),
    "picture-at-the-tick": (("huh - back * 0.033333335f", "huh"), "picture-moves"),
}


def under_test(fault=None):
    definitions = read("source/objects/object_definitions.h")
    objects_h = read("source/objects/objects.h")
    periodic = read("source/math/periodic_functions.c")
    enums = "\n".join((enum_with(definitions, "_object_function_invert_function_bit"),
                       enum_with(definitions, "_object_function_scale_to_fit_bounds"),
                       enum_with(periodic, "_periodic_function_one"),
                       enum_with(read("source/math/periodic_functions.h"), "_transition_function_linear")))
    enums += "".join(f"\n#define {name} {constant(source, name)}" for source, name in (
        (definitions, "NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS"), (definitions, "NUMBER_OF_INCOMING_OBJECT_FUNCTIONS"),
        (read("source/tag_files/tag_files.h"), "TAG_STRING_LENGTH"), (objects_h, "MAXIMUM_OBJECTS_PER_MAP"))) + "\n"
    structures = structure(definitions, "object_function_definition") + "\n"
    objects = read("source/objects/objects.c")
    text = structure(objects, "object_function_inputs") + "\n"
    text += re.search(r"^static struct object_function_inputs object_function_inputs\[\w+\];", objects, re.M).group(0)
    text += "\n"
    for name in ("object_function_input_value", "object_function_wrap", "object_function_addition",
                 "object_function_inputs_forget", "object_function_values_for_render",
                 "object_compute_function_values"):
        text += function(objects, name) + "\n"
    return (("enums.inc", enums), ("structures.inc", structures),
            ("under_test.inc", mutated(text, *fault) if fault else text))


def test_forgotten_on_load_and_map_start():
    """game_state.c forgets the kept inputs before a game state is loaded and when a map starts."""
    game_state = read("source/saved games/game_state.c")
    procs = re.search(r"before_load_procs\[\] =\s*\{(.*?)\};", game_state, re.S).group(1)
    assert re.search(r"\bobject_function_inputs_forget\b", procs), "not forgotten before a load"
    assert "object_function_inputs_forget();" in function(game_state, "game_state_initialize_for_new_map"), \
        "not forgotten when a map starts"


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("picture_function_inputs", under_test()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("picture_function_inputs", under_test(fault)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"


if __name__ == "__main__":
    sys.exit(pytest.main(["-q", __file__]))
