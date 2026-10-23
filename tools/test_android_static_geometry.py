"""Check Android's pooled per-draw uploads and its static geometry cache.

Compile the production code of port/linux/src/d3d8_gl.c with fake GL: the
pooled uploads never write a buffer that a queued draw reads, and the static
cache uploads each range once, drops it on release and address reuse, and
streams a range whose GL storage cannot be had.
"""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


def between(text, start, end):
    first = text.index(start)
    return text[first:text.index(end, first)]


def compile_test(directory, source, include, name):
    compiler = shutil.which(os.environ.get("CC", "clang"))
    if not compiler:
        pytest.skip("clang required")
    work = directory / name
    work.mkdir()
    (work / Path(include[0]).name).write_text(include[1])
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else ["-m32"]
    executable = work / ("test.exe" if os.name == "nt" else "test")
    result = subprocess.run(
        [compiler, *target, "-std=gnu89", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
         "-Wno-unused-parameter", "-O2", "-I", str(work), str(ROOT / source), "-o", str(executable)],
        capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return executable


@pytest.fixture(scope="module")
def programs(tmp_path_factory):
    renderer = (ROOT / "port/linux/src/d3d8_gl.c").read_text()
    directory = tmp_path_factory.mktemp("android-static-geometry")
    static = between(renderer, "#ifdef HALO_ANDROID\n/* ---------- static geometry", "/* ---------- vertex data */")
    pooled = (between(renderer, "#ifdef HALO_GLES\n/* port: WebGL on Metal",
                      "#ifdef HALO_GLES\n/* stream_upload, with the D3DCOLOR")
              + between(renderer, "static unsigned long index_upload(", "static void attribute_format("))
    return {
        "static": compile_test(directory, "port/android/tests/static_geometry_test.c",
                               ("static_geometry.inc", static), "static"),
        "pooled": compile_test(directory, "port/android/tests/pooled_upload_test.c",
                               ("pooled_upload.inc", pooled), "pooled"),
    }


@pytest.mark.parametrize("case", ["reuse", "reload", "address_reuse", "allocation_failure", "regions"])
def test_static_geometry(programs, case):
    result = subprocess.run([str(programs["static"]), case], capture_output=True, text=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("case", ["frames", "large"])
def test_pooled_uploads(programs, case):
    result = subprocess.run([str(programs["pooled"]), case], capture_output=True, text=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
