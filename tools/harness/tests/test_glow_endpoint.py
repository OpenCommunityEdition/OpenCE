"""Original glow interval lookup accepts the endpoint its particle updates admit."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import read, function, build, run, mutated


def implementation():
    code = function(read("source/objects/widgets/glow.c"), "get_particle_world_position")
    start = code.index("\tfor (marker_index = 0;")
    end = code.index("\n", code.index("\tparticle->parent_marker_index ="))
    return ("static short marker_at(struct glow_datum *glow, struct glow_particle *particle)\n"
            "{ short marker_index;\n" + code[start:end] + "\nreturn particle->parent_marker_index; }\n")


@pytest.mark.parametrize("case", ["endpoint", "interior", "invalid"])
def test_glow_interval(case):
    status, output = run(build("glow_endpoint", (("under_test.inc", implementation()),)), case)
    assert status == (1 if case == "invalid" else 0), output
    if case == "invalid":
        assert "marker interval missing" in output, output


def test_original_endpoint_defect_is_detected():
    code = mutated(implementation(), "marker_index == glow->number_of_markers - 2", "FALSE")
    status, output = run(build("glow_endpoint", (("under_test.inc", code),)), "endpoint")
    assert status == 1 and "marker interval missing" in output, output
