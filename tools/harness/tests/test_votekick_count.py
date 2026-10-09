"""The real count of network_votekick.c: who may vote, whose votes are one person's (by address, and by a hardware id
that only ever takes votes away), and how many kick."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, run  # noqa: E402

CASES = ["majority", "even-teams", "two-never-kick", "same-address-one-vote", "same-hardware-id-one-vote",
         "copied-hardware-id-lowers-nothing", "target-never-votes", "eligibility"]
FUNCTIONS = ("votekick_same_address", "votekick_same_person", "votekick_may_vote", "votekick_count",
             "votekick_votes_needed")

# a fault in network_votekick.c, the function it is in, and the case that must catch it
NEGATIVE_CONTROLS = {
    "half-is-enough": (("electorate / 2 + 1", "(electorate + 1) / 2"), "votekick_votes_needed", "majority"),
    "no-minimum": (("needed < MINIMUM_VOTES ? MINIMUM_VOTES : needed", "needed"), "votekick_votes_needed",
                   "two-never-kick"),
    "target-not-counted": (("votekick_may_vote(&voters[index]) || voters[index].target;",
                            "votekick_may_vote(&voters[index]);"), "votekick_count", "even-teams"),
    "votes-by-address-only": (("votekick_may_vote(&voters[other]) &&\n\t\t\t\tvotekick_same_person(",
                               "votekick_may_vote(&voters[other]) &&\n\t\t\t\tvotekick_same_address("),
                              "votekick_count", "same-hardware-id-one-vote"),
    "electorate-by-hardware-id": (("votekick_same_address(&voters[index], &voters[other]))\n\t\t\t{\n\t\t\t\tcounted = FALSE;\n\t\t\t}\n\t\t}\n\t\tif (counted)\n\t\t\t(*electorate)++;",
                                   "votekick_same_person(&voters[index], &voters[other]))\n\t\t\t{\n\t\t\t\tcounted = FALSE;\n\t\t\t}\n\t\t}\n\t\tif (counted)\n\t\t\t(*electorate)++;"),
                                  "votekick_count", "copied-hardware-id-lowers-nothing"),
    "target-votes": (("if (voters[other].target && votekick_same_person(", "if (0 && votekick_same_person("),
                     "votekick_count", "target-never-votes"),
    "newcomers-vote": (("return voter->eligible && !voter->target", "return !voter->target"), "votekick_may_vote",
                       "eligibility"),
}


def generated(fault=None, faulty_function=None):
    source = read("port/linux/game/network_votekick.c")
    config = ""
    for name in ("VOTEKICK_HARDWARE_ID_SIZE", "MINIMUM_VOTES"):
        config += f"#define {name} {constant(source, name)}\n"
    match = re.search(r"^struct votekick_voter\n\{.*?^\};\n", source, re.M | re.S)
    assert match, "struct votekick_voter not found in network_votekick.c"
    config += match.group(0)
    text = ""
    for name in FUNCTIONS:
        code = function(source, name)
        if fault and name == faulty_function:
            code = mutated(code, *fault)
        text += code + "\n"
    return (("config.inc", config), ("under_test.inc", text))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("votekick_count", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, faulty_function, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("votekick_count", generated(fault, faulty_function)), case)
    assert status == CHECK_FAILED, f"network_votekick.c with a fault ({control}) passed '{case}': the test cannot see it"
