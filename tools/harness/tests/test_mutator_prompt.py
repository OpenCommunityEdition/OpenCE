"""The question about a host's script (mutator_share.c): a controller's A or B, or on a touchscreen a tap, the
left half yes and the right no; none in its first moments."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["taps", "buttons"]

# a fault in the code, and the case that must catch it
NEGATIVE_CONTROLS = {
    "sides": (("(tapped && tap_x < 0.5f)", "(tapped && tap_x >= 0.5f)"), "taps"),
    "read": (("boolean tapped = halo_chat_taps(&tap_x, &tap_y) > 0 && !reading;",
              "boolean tapped = halo_chat_taps(&tap_x, &tap_y) > 0;"), "taps"),
}


def generated(fault=None):
    share = read("port/linux/game/mutator_share.c")
    text = enum_with(read("port/linux/include/halo_keyboard.h"), "HALO_CHAT_PAD_A") + "\n"
    text += enum_with(share, "MAXIMUM_SHARED_MUTATORS") + "\n" + enum_with(share, "_share_prompting") + "\n"
    text += structure(share, "port_mutator_share_file") + "\n" + structure(share, "port_mutator_share") + "\n"
    # (the state: mutator_share.c's static struct)
    text += re.search(r"^static struct\n\{\n\tshort state;.*?^\} mutator_share;\n", share, re.S | re.M).group(0)
    text += "static void port_mutators_share_script_allow(struct port_mutator_share const *entry);\n"
    text += "static void client_next_file(void);\n"
    code = function(share, "client_prompt_update")
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", text + code + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("mutator_prompt", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("mutator_prompt", generated(fault)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
