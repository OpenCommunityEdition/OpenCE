"""The real widget pool (widgets.c) over fake objects: fills, refuses past its size, frees and reuses its slots."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402

CASES = ["finds-the-type", "fills-and-refuses", "frees-and-reuses", "failed-widget-frees-its-slot"]

# a fault in widgets.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "keeps-a-failed-widgets-slot": (("datum_delete(widget_data, widget_index);", ";"), "widgets_new",
                                    "failed-widget-frees-its-slot"),
    "never-frees-on-delete": (("datum_delete(widget_data, widget_index);", ";"), "widgets_delete", "frees-and-reuses"),
}


def generated(fault=None, faulty_function=None):
    widgets = read("source/objects/widgets/widgets.c")
    groups = re.findall(r"^\s*'(\w{3}[\w!])',\s*$", widgets, re.M)
    assert len(groups) == 5, f"widget types not found in widgets.c: {groups}"
    config = "".join(f"#define GROUP_TAG_{index} '{group}'\n" for index, group in enumerate(groups))
    config += f"#define MAXIMUM_WIDGETS_PER_MAP {constant(widgets, 'MAXIMUM_WIDGETS_PER_MAP')}\n"
    text = ""
    for name in ("widget_type_definition_get", "tag_group_to_widget_type", "widgets_new", "widgets_delete"):
        code = function(widgets, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("widget_pool", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("widget_pool", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"widgets.c with a fault ({control}) passed '{case}': the test cannot see it"

