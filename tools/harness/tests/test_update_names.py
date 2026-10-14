"""The names the self-updater takes from a release's zip (updater.c): the tree's and the flat release's files, and
none that would leave the game's folder."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["release-names", "leaving-names"]

# a fault in updater_name_inside, and the case that must catch it
NEGATIVE_CONTROLS = {
    "lets-dot-dot-through": (("|| (length == 2 && part[0] == '.' && part[1] == '.')", ""), "leaving-names"),
    "lets-backslashes-through": (("strchr(name, '\\\\') || ", ""), "leaving-names"),
    "lets-absolute-through": (("if (!length ||", "if ((!length && part != name) ||"), "leaving-names"),
    "refuses-folders": (("if (!part[length])", "if (part == name && !part[length])"), "release-names"),
}


def generated(fault=None):
    code = function(read("port/linux/src/updater.c"), "updater_name_inside")
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", "#include <string.h>\n" + code + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("update_names", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("update_names", generated(fault)), case)
    assert status == CHECK_FAILED, f"updater.c with a fault ({control}) passed '{case}': the test cannot see it"
