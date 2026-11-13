"""The real glow_update (glow.c): a glow's trailing particles are made and aged by the ticks since the last frame, not
by the game's whole time (which made them up to the limit every frame and aged each past its lifetime by the next)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["generation", "between", "lifetime", "views"]

GENERATE = "glow->accumulated_trailing_particle_generation_ticks += trailing_ticks;"
AGE = "particle->ticks_in_existence += trailing_ticks;"

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # made by the game's whole time (before fix-glow-trailing-ticks)
    "made-by-game-time": ((GENERATE, GENERATE.replace("trailing_ticks;", "(short)game_time_get();")),
                          "generation"),
    # aged by the game's whole time (before fix-glow-trailing-ticks)
    "aged-by-game-time": ((AGE, AGE.replace("trailing_ticks;", "(short)game_time_get();")), "lifetime"),
    # made every frame, ticks or not
    "made-every-frame": ((GENERATE, GENERATE.replace("trailing_ticks;", "1;")), "between"),
    # never made
    "never-made": ((GENERATE, GENERATE.replace("trailing_ticks;", "0;")), "generation"),
    # each view of a frame adds its ticks (before the once-a-frame count)
    "every-view": (("return (short)PIN(since, 0, game_time_get_elapsed());", "return game_time_get_elapsed();"),
                   "views"),
}


def generated(fault=None):
    source = read("source/objects/widgets/glow.c")
    config = "\n".join([
        enum_with(source, "MAXIMUM_GLOW_MARKERS ="),
        enum_with(source, "_glow_definition_trailing_particles_fade_over_time_bit,"),
        enum_with(source, "_glow_particle_trailing_bit,"),
        structure(source, "glow_particle"),
        structure(source, "glow_datum"),
        structure(source, "glow_definition"),
    ]) + "\n"
    code = "\n".join(function(source, name) for name in (
        "glow_trailing_particle_update_color", "glow_trailing_particle_update_size",
        "glow_trailing_particle_update_velocity", "glow_trailing_particle_update_position",
        "glow_trailing_particle_age", "glow_update_trailing_ticks", "glow_update")) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("glow_trailing_ticks", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("glow_trailing_ticks", generated(fault)), case)
    assert status == CHECK_FAILED, f"glow_update with a fault ({control}) passed '{case}': the test cannot see it"
