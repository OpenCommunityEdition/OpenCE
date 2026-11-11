"""The rider's turn takes one sine and cosine for all eight vectors, and a support that rides nothing asks no claim.

    python -m pytest -q tools/harness/tests/test_riding_turn.py
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run  # noqa: E402

CASES = ["turn-eight-matches-one", "turn-trig-once", "turn-all-eight", "claim-none-no-fill", "claim-ridden-best",
         "claim-competing-decks"]

# a fault in the taken functions (vehicle_riding.c, vehicle_riding_bipeds.c), and the case that must catch it
NEGATIVE_CONTROLS = {
    # (a swapped multiply gives the same bits, as products commute: the order that matters is the temporary's, the
    # second line reading the first's result)
    "turn-vectors-reordered": (("vectors[index]->j = i*sine + vectors[index]->j*cosine;",
                                "vectors[index]->j = vectors[index]->i*sine + vectors[index]->j*cosine;"),
                               "turn-eight-matches-one"),
    "turn-vectors-drops-one": (("vehicle_riding_turn_vectors(vectors, 8, dyaw);", "vehicle_riding_turn_vectors(vectors, 7, dyaw);"),
                               "turn-all-eight"),
    "claim-early-out-on-any": (("if (physics->riding_object_index == NONE)\n\t\treturn best;", "if (TRUE)\n\t\treturn best;"),
                               "claim-competing-decks"),
}


def generated(fault=None):
    riding = read("port/linux/game/vehicle_riding.c")
    bipeds = read("port/linux/game/vehicle_riding_bipeds.c")
    counted = "#define vehicle_riding_claim(...) (claim_calls++, vehicle_riding_claim(__VA_ARGS__))\n"
    text = "\n".join([function(riding, "vehicle_riding_turn_vector"), function(riding, "vehicle_riding_turn_vectors"),
                       function(riding, "vehicle_riding_claim"), counted, function(bipeds, "biped_riding_turn"),
                       function(bipeds, "biped_riding_claimed_support")])
    if fault:
        text = mutated(text, *fault)
    return (("under_test.inc", text),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("riding_turn", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("riding_turn", generated(fault)), case)
    assert status == CHECK_FAILED, f"fault {control} passed {case}: {output}"
