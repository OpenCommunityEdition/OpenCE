"""Presentation must target the window even when its source FBO is new.

Use the production framebuffer cache and presentation commands. A fresh map
load can create the source FBO while drawing its loading screen; a reused
disk cache can reach presentation without that draw.
"""
import os
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import function, read


@pytest.fixture(scope="module")
def present_framebuffer(tmp_path_factory):
    compiler = shutil.which(os.environ.get("CC", "clang"))
    if not compiler:
        pytest.skip("clang required")
    renderer = read("port/linux/src/d3d8_gl.c")
    present = function(renderer, "D3DDevice_Present")
    commands = present[present.index("\t\tglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);"):
                       present.index("\t\t/* (the HDR calibration screen")]
    work = tmp_path_factory.mktemp("present-framebuffer")
    (work / "cache.inc").write_text("\n".join(
        function(renderer, name) for name in ("device_record_new", "framebuffer_find", "framebuffer_get")))
    (work / "present.inc").write_text(commands)
    executable = work / ("test.exe" if os.name == "nt" else "test")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else ["-m32"]
    result = subprocess.run(
        [compiler, *target, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
         "-Wno-unused-parameter", "-I", str(work),
         str(Path(__file__).with_name("present_framebuffer.c")), "-o", str(executable)],
        capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return executable


@pytest.mark.parametrize("case", ["new", "primed", "changed", "hdr", "hdr-fallback"])
def test_presentation_targets_window(present_framebuffer, case):
    result = subprocess.run([str(present_framebuffer), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
