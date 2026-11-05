"""The real render_particles (render_particles.c) over a view and a sprite builder that keep which particles are drawn
with the first-person weapon, whose pass draws them away from it without the depth test: a particle stuck to the
first-person weapon or shown only in first person is; one the weapon threw off (a casing), free in the world, is not,
so that it is hidden behind floors and walls as on the Xbox."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure  # noqa: E402

CASES = ["thrown-casing", "stuck-charge", "first-person-only", "third-person", "another-player"]

# a fault in the code and the case that must catch it
NEGATIVE_CONTROLS = {
    # the weapon's flag alone, a thrown casing drawn with the weapon (before fix-thrown-casings-depth)
    "flag-alone": ((" &&\n\t\t\t\t\tparticle->object_index != NONE));", "));"), "thrown-casing"),
    # a particle stuck to the weapon drawn in the world (before the port drew a charge with the viewmodel)
    "stuck-in-the-world": (("\t\t\t\t\tparticle->object_index != NONE));", "\t\t\t\t\tFALSE));"), "stuck-charge"),
    # a particle shown only in first person drawn in the world
    "first-person-only-in-the-world": (("(TEST_FLAG(\n\t\t\t\t\t\tparticle->flags,\n"
                                        "\t\t\t\t\t\t_particle_datum_dont_draw_third_person_bit) ||",
                                        "(FALSE ||"), "first-person-only"),
    # any player's particle drawn with the first-person weapon
    "anyones": (("particle->local_player_index == local_player_index;", "TRUE;"), "another-player"),
}


def generated(fault=None):
    source = read("source/render/render_particles.c")
    particles = read("source/effects/particles.h")
    sprites = read("source/render/render_sprite.h")
    config = "\n".join([
        "enum { MAXIMUM_RENDERED_PARTICLES = %d, MAXIMUM_RENDERED_PARTICLE_GROUPS = %d };" % (
            constant(source, "MAXIMUM_RENDERED_PARTICLES"), constant(source, "MAXIMUM_RENDERED_PARTICLE_GROUPS")),
        enum_with(particles, "_particle_datum_attached_to_local_player_bit"),
        enum_with(sprites, "_build_sprites_first_person_bit"),
        enum_with(sprites, "_build_sprite_u_mirror_bit"),
        structure(particles, "particle_datum"),
        structure(source, "rendered_particle_datum"),
    ]) + "\n"
    code = "\n".join(function(source, name) for name in ("compare_rendered_particles", "render_particles")) + "\n"
    if fault:
        code = mutated(code, *fault)
    return (("config.inc", config), ("under_test.inc", code))


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("first_person_particles", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("first_person_particles", generated(fault)), case)
    assert status == CHECK_FAILED, f"render_particles with a fault ({control}) passed '{case}': the test cannot see it"
