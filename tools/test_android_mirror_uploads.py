"""Check Android geometry streaming and preserved desktop/web caching.

Compile the production mirror and frame rotation. Model driver storage
renaming, not a particular phone; this does not establish the device cause.
"""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

from harness import function, mutated

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def mirror_uploads(tmp_path_factory):
    compiler = shutil.which(os.environ.get("CC", "clang"))
    if not compiler:
        pytest.skip("clang required")
    renderer = (ROOT / "port/linux/src/d3d8_gl.c").read_text()
    mirror = renderer[renderer.index("#define MIRROR_SEGMENT_SIZE"):
                      renderer.index("/* the smallest and largest index of an index range")]
    present = function(renderer, "D3DDevice_Present")
    rotation = present[present.index("host_gl_fence_frame("):
                       present.index("if (xgpu_capabilities.atomic_counters && device.ring_test_count")]
    directory = tmp_path_factory.mktemp("android-mirror-regression")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else ["-m32"]
    binaries = {}
    for variant, defines in (("android", ["-DHALO_ANDROID=1"]), ("android-cached", ["-DHALO_ANDROID=1"]),
                             ("gles", []), ("web", ["-DHALO_WEB=1"]),
                             ("desktop", ["-DTEST_DESKTOP=1"]), ("unguarded", [])):
        work = directory / variant
        work.mkdir()
        selected = mutated(mirror,
                           "unused && device.frame - mirror.subdata_frame[segment] >= STREAM_BUFFER_RING",
                           "unused") if variant == "unguarded" else mirror
        if variant == "android-cached":
            selected = mutated(selected,
                               "/* Persistent geometry caching causes corruption on affected Android\n"
                               "\tdevices. Use current vertex/index bytes through the streaming path. */\n"
                               "\treturn FALSE;",
                               "/* Control: leave the cache enabled. */")
        (work / "mirror.inc").write_text(selected)
        (work / "rotation.inc").write_text(rotation)
        executable = work / ("test.exe" if os.name == "nt" else "test")
        result = subprocess.run(
            [compiler, *target, *defines, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
             "-Wno-unused-function", "-O2", "-I", str(work),
             str(ROOT / "port/android/tests/mirror_upload_test.c"), "-o", str(executable)],
            capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr
        binaries[variant] = executable
    return binaries


@pytest.mark.parametrize("case", ["reuse", "rollover"])
@pytest.mark.parametrize("variant", ["android", "android-cached", "gles", "web", "desktop", "unguarded"])
def test_mirror_upload_ordering(mirror_uploads, case, variant):
    result = subprocess.run([str(mirror_uploads[variant]), case], capture_output=True, text=True, timeout=15)
    if variant == "android-cached":
        assert result.returncode == 1, result.stdout + result.stderr
        assert result.stderr.rstrip().endswith(
            ": !mirror_range(PLATFORM_CONTIGUOUS_BASE, 300 * 4096, &buffer, &offset, &generation)"), result.stderr
    elif variant == "unguarded":
        assert result.returncode == 1, result.stdout + result.stderr
        assert "repair_uploads=0" in result.stderr, result.stderr
        assert result.stderr.rstrip().endswith(": overlaps == 0 && mismatches == 0"), result.stderr
        assert "overlaps=0 " not in result.stderr and "bad_draws=0 " not in result.stderr, result.stderr
    else:
        assert result.returncode == 0, result.stdout + result.stderr
        if variant == "android":
            assert "bypassed=4800 allocations=0 uploads=0 watch_reads=0 protections=0" in result.stderr, result.stderr
        elif variant == "desktop":
            assert "mapped=0 " in result.stderr, result.stderr
