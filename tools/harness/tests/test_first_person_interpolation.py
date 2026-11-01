"""Exercise the production first-person snapshot and matrix blending code."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import function, mutated, read, structure


CASES = ["fast-motion", "weapon-change", "graph-change", "same-tick-change",
         "node-count", "gap", "disabled", "reset", "players", "camera", "invalid", "capacity", "missing",
         "chamber-reset", "large-rotation"]


def generated():
    source = read("port/linux/game/render_interpolation.c")
    math = read("source/math/matrix_math.c")
    matrices = "\n".join(function(math, name) for name in
                         ["matrix4x3_inverse", "matrix4x3_rotation_from_vectors",
                          "matrix4x3_from_point_and_vectors", "matrix4x3_multiply"])
    types = "\n".join(structure(source, name) for name in
                      ["interpolation_quaternion", "interpolation_rotation",
                       "interpolated_first_person"])
    blend = "\n".join(function(source, name) for name in
                      ["lerp", "point_lerp", "vector_length", "basis_is_rotation",
                       "quaternion_from_basis", "basis_from_quaternion",
                       "rotation_from_matrix", "matrix_blend_rotations", "distance_squared"])
    # Reset's object/camera storage is stubbed; the first-person reset runs intact.
    cutoff = re.search(r"^#define FIRST_PERSON_HALF_TURN_COSINE .*", source, re.M).group(0)
    return cutoff + "\n" + matrices + "\n" + types + "\n" + blend + "\n" + (
        "static struct interpolated_first_person interpolated_first_person[MAXIMUM_LOCAL_PLAYERS];\n"
    ) + function(source, "render_interpolation_reset") + "\n" + function(source, "render_interpolation_first_person")


def compile_fixture(work, source):
    compiler = shutil.which(os.environ.get("CC", "clang"))
    if not compiler:
        pytest.skip("clang required")
    (work / "under_test.inc").write_text(source)
    fixture = Path(__file__).with_name("first_person_interpolation.c")
    binary = work / ("test.exe" if sys.platform == "win32" else "test")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else ["-m32", "-lm"]
    subprocess.run([compiler, "-std=gnu89", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-Wno-unused-function", "-I", str(work), "-I", str(fixture.parent.parent / "include"),
                    str(fixture), *flags, "-o", str(binary)], check=True)
    return binary


@pytest.fixture(scope="module")
def executable(tmp_path_factory):
    return compile_fixture(tmp_path_factory.mktemp("first-person-interpolation"), generated())


@pytest.mark.parametrize("case", CASES)
def test_first_person_interpolation(executable, case):
    result = subprocess.run([str(executable), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


def test_old_distance_cutoff_fails(tmp_path):
    source = mutated(generated(), "REAL_MAX))", "0.25f * 0.25f))")
    binary = compile_fixture(tmp_path, source)
    result = subprocess.run([str(binary), "fast-motion"], capture_output=True, text=True, timeout=10)
    assert result.returncode == 1 and "position" in result.stderr, result.stdout + result.stderr


def test_blending_chamber_reset_fails(tmp_path):
    source = mutated(generated(), "< FIRST_PERSON_HALF_TURN_COSINE", "< 0.0f")
    binary = compile_fixture(tmp_path, source)
    result = subprocess.run([str(binary), "chamber-reset"], capture_output=True, text=True, timeout=10)
    assert result.returncode == 1 and "half-turn" in result.stderr, result.stdout + result.stderr
