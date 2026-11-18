"""A pitch range that chooses from more than 32 permutations (a Custom Edition sound's, OpenH1 issue 1170) plays every
one of them before any repeats: the real sound_definitions.c keeps the played bits past its 32-bit mask apart from the
map's structure, reset with it as a map loads. A range of 32 or fewer picks as the game did, draw for draw, and the
continuations of a linked permutation never start a sound."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, function, mutated, read, structure, run  # noqa: E402
from harness.audio import permutation_picker  # noqa: E402

CASES = ["forty", "same-as-old", "pieces", "reloaded", "map-after-map", "allocation-failure", "table-full",
         "skip-boundaries", "retired-ranges"]
NEGATIVE_CONTROLS = {
    "no-reset-after-skip": (("\t\t\tif (beyond)\n\t\t\t\ttry_to_reset_permutations_beyond(range, beyond);\n", ""),
                            "skip-boundaries"),
    "no-slot-reuse": (("table->free_index = index + 1;", "table->free_index = 0;"), "retired-ranges"),
    # the old clamp: a range picks from its first 32 permutations only
    "old-clamp": (("\t\tbeyond = sound_pitch_range_played_beyond(range);\n",
                   "\t\tbeyond = NULL;\n"), "forty"),
    # the issue's smallest change: the permutations past 32 are not kept as played, so they may repeat
    "past-32-repeat": (("\t\tBIT_VECTOR_SET_FLAG(beyond, index - 32, TRUE);\n", "\t\t(void)beyond;\n"),
                       "forty"),
    # the bits past 32 of a range whose map loaded again kept, not reset with its played mask
    "stale-bits": (("\tfor (word = 0; word < SOUND_PLAYED_BEYOND_LONGS; word++)\n"
                    "\t\ttable->entries[index].bits[word] = 0;\n", ""), "reloaded"),
    # the entries not let go as a new map starts: map after map they run out
    "no-clear": (("\tsound_played_beyond_table.count = 0;\n", ""), "map-after-map"),
    # a range of 32 or fewer reset otherwise than the game did: its picks differ
    "small-reset-changed": (("\t\t\trange->played_permutation_mask = FLAG(range->previous_permutation_index);\n",
                             "\t\t\trange->played_permutation_mask = 0;\n"), "same-as-old"),
}


def generated(fault=None):
    definitions = read("source/sound/sound_definitions.h")
    tags = read("source/tag_files/tag_groups.h")
    types = "".join(structure(tags, name) + "\n" for name in ["tag_block", "tag_reference", "tag_data"])
    types += "".join(structure(definitions, name) + "\n" for name in
                     ["real_bounds", "sound_permutation", "sound_pitch_range", "sound_definition"])
    most = constant(definitions, "MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE")
    types += f"#define MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE {most}\n"
    code = permutation_picker()
    probe = function(code, "sound_pitch_range_permutation_played")
    code = "static unsigned long bit_tests;\n" + code.replace(probe, probe.replace("{",
        '{\n CHECK(++bit_tests < 4096, "excessive probing or a stuck picker");', 1))
    if fault:
        code = mutated(code, *fault)
    return (("types.inc", types), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("sound_played_beyond", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("sound_played_beyond", generated(fault)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
