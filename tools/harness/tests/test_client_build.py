"""What a host keeps of a client's build as the client tells it (network_distributed.c's distributed_build_text):
another machine's text, kept printable, within its size, ended."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["printable", "lines", "bounds"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    "control-characters-kept": (("(character >= ' ' && character <= '~')", "(character)"), "printable"),
    "line-ends-everywhere": (("(lines && character == '\\n')", "(character == '\\n')"), "lines"),
    "no-size-limit": (("source[index] && length < size - 1", "source[index]"), "bounds"),
}


def generated(fault=None):
    code = function(read("port/linux/game/network_distributed.c"), "distributed_build_text")
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", code + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("client_build", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("client_build", generated(fault)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
