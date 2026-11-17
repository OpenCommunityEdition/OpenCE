"""Mod sound files with more than 32 variations load and play each variation before repeating."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import (CHECK_FAILED, build, constant, enum_with, function, mutated, read,  # noqa: E402
                     run, structure)
from harness.audio import permutation_picker  # noqa: E402

CASES = ["forty-variations", "thirty-two", "maximum"]
NEGATIVE_CONTROLS = {
    # the old refusal (#1170): a mod's sound picking from more than 32 permutations left out
    "refuse-past-32": (("\t\t\t\trange->actual_permutation_count < 1 ||\n",
                        "\t\t\t\trange->actual_permutation_count < 1 || range->actual_permutation_count > 32 ||\n"),
                       "forty-variations"),
    # a range of more than 32 loaded as all played: those past 32 play first, then the first 32 start over
    "all-played-past-32": (("range->actual_permutation_count > 32 ? 0 : 0xFFFFFFFF;", "0xFFFFFFFF;"),
                           "forty-variations"),

}


def generated(fault=None):
    definitions = read("source/sound/sound_definitions.h")
    tags = read("source/tag_files/tag_groups.h")
    mods_header = read("port/linux/game/mods.h")
    types = "".join(structure(tags, name) + "\n" for name in ["tag_block", "tag_reference", "tag_data"])
    types += "".join(structure(definitions, name) + "\n" for name in
                     ["real_bounds", "sound_permutation", "sound_pitch_range", "sound_definition"])
    types += enum_with(definitions, "_sound_definition_linked_permutations_bit") + "\n"
    for name in ["MAXIMUM_SOUND_DATA_SIZE", "MAXIMUM_PITCH_RANGES_PER_SOUND", "MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE",
                 "MAXIMUM_PERMUTATIONS_PER_RANDOM_PITCH_RANGE", "MAXIMUM_SOUND_MOUTH_DATA_SIZE",
                 "MAXIMUM_SOUND_SUBTITLE_DATA_SIZE", "SOUND_DEFINITION_VERSION"]:
        types += f"#define {name} {constant(definitions, name)}\n"
    types += "#define SOUND_DEFINITION_TAG 0x736E6421\n"
    types += "".join(line + "\n" for line in mods_header.splitlines()
                     if line.startswith(("#define MOD_TAG_FILE_", "#define MAXIMUM_MOD_TAG_FILE_SIZE")))
    encoder = read("port/linux/game/custom_edition_sounds.c")
    code = encoder[encoder.index("/* ---------- constants */"):]
    mods = read("port/linux/game/mods.c")
    code += "\n".join(function(mods, name) for name in ["mods_read_be16", "mods_read_be32", "mods_read_short",
                                                         "mods_read_real", "mods_in_file", "mods_finite_real",
                                                         "mods_tag_file_header_valid"]) + "\n"
    loader = read("port/linux/game/mod_sounds.c")
    code += loader[loader.index("/* ---------- constants */"):loader.index("/* Gives `definition` the values")]
    code += permutation_picker()
    if fault:
        code = mutated(code, *fault)
    return (("types.inc", types), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("mod_sound_variations", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("mod_sound_variations", generated(fault)), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"
