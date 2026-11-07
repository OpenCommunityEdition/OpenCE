"""The real first_person_legs_clear_of_eye (first_person_legs.c): the first-person legs are set back behind the eye
until none of their vertices is nearer it than the near clip plane and a little, so that a crouched walk's knees,
raised to the eye, are not cut open and seen from inside; the upper body, folded away, is not looked at."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["knee-at-eye", "legs-far", "knee-ahead", "upper-body", "chain"]
FUNCTIONS = ("first_person_legs_name_has", "first_person_legs_node", "first_person_legs_clear_of_eye")

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the legs not kept clear of the eye (before fix-first-person-legs-knees)
    "not-cleared": (("if (across < clearance * clearance)", "if (0)"), "knee-at-eye"),
    # the near clip plane left out of the clearance
    "no-near-plane": (("global_window_parameters.frustum.z_near + FIRST_PERSON_LEGS_CLEARANCE",
                       "FIRST_PERSON_LEGS_CLEARANCE"), "knee-at-eye"),
    # a vertex the upper body moves too counted as the legs'
    "upper-body-counted": (("(vertex->weight < 32767 &&\n\t\t\t\t(second", "(0 &&\n\t\t\t\t(second"), "upper-body"),
    # one look at the near vertices only: a set back that brings another to the eye kept
    "one-pass": (("\t\tmoved = FALSE;\n", "\t\tmoved = FALSE;\n\t\tbreak;\n"), "chain"),
}


def generated(fault=None):
    source = read("port/linux/game/first_person_legs.c")
    config = "\n".join([
        enum_with(source, "FIRST_PERSON_LEGS_MAXIMUM_NODES"),
        re.search(r"#define FIRST_PERSON_LEGS_CLEARANCE .*", source).group(0),
        re.search(r"#define FIRST_PERSON_LEGS_MAXIMUM_NEAR .*", source).group(0),
    ]) + "\n"
    header = read("port/linux/game/object_mesh.h")
    mesh = "\n".join([enum_with(header, "OBJECT_MESH_MAXIMUM_NODES"), structure(header, "object_mesh_vertex"),
                      structure(header, "object_mesh")]) + "\n"
    code = "\n".join(function(source, name) for name in FUNCTIONS) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("mesh.inc", mesh), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("first_person_legs_eye", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("first_person_legs_eye", generated(fault)), case)
    assert status == CHECK_FAILED, f"the legs with a fault ({control}) passed '{case}': the test cannot see it"
