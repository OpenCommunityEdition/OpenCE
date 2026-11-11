"""Record the production stock renderer and feedback capture; no retail assets."""
from pathlib import Path
import re
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from harness import function


def test_stock_loading_framing(tmp_path):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is required")
    feature = (ROOT / "port/linux/src/loading_widescreen.c").read_text()
    feature = re.sub(r'^#include "[^\n]+"\s*$', '', feature, flags=re.M)
    game = (ROOT / "source/interface/progress_bar.c").read_text()
    renderer = function(game, "progress_bar_render")
    capture = function((ROOT / "port/linux/src/d3d8_gl.c").read_text(), "front_buffer_take")
    source = (ROOT / "tools/harness/tests/loading_widescreen.c").read_text()
    source = source.replace("/* FEATURE */", feature).replace("/* RENDERER */", renderer)
    source = source.replace("/* CAPTURE */", capture)
    # Unobserved device calls still execute through harmless test stubs.
    ignored = ["SetRenderState", "SetTextureStageState", "SetTexture", "Clear", "SetVertexShader"]
    macros = [f"#define IDirect3DDevice8_{name}(...) ((void)0)" for name in ignored]
    source = source.replace("/* UNUSED DEVICE CALLS */", "\n".join(macros))
    path = tmp_path / "loading.c"
    path.write_text(source)
    binary = tmp_path / "loading.exe"
    command = [compiler, "-std=gnu89", "-O2", "-Wall", "-Werror", "-Wno-unused-function",
               "-Wno-unused-variable", str(path), "-o", str(binary)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"]
    else:
        command += ["-lm"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=20)
    assert result.returncode == 0, result.stdout + result.stderr
