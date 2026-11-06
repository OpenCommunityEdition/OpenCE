"""Tests of how configure.py generates the desktop guest builds
(tools/linux_guest_build.py): the 64-bit x86 one, not its computer's default
build, downloads nothing unless asked for, and a build that cannot be
generated (here, a compiler that does not run) is a target that stops with
the reason instead of stopping configure.py for every target."""
import io
import json
import shutil
import sys
from pathlib import Path
from types import SimpleNamespace

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import linux_guest_build  # noqa: E402
from tools.ninja_syntax import Writer  # noqa: E402

X86_64 = next(target for target in linux_guest_build.LINUX_GUESTS if target.name == "linux_x86_64")


@pytest.fixture
def tree(tmp_path, monkeypatch):
    """a folder that configure.py runs in, with the build's settings and the
    system's OpenGL headers, on a 64-bit x86 computer; fetches are recorded"""
    (tmp_path / "port" / "linux" / "host").mkdir(parents=True)
    shutil.copy(ROOT / "port" / "linux" / "port.json", tmp_path / "port" / "linux" / "port.json")
    headers = tmp_path / "gl"
    for name in ("GLES3/gl32.h", "GLES2/gl2ext.h", "EGL/egl.h", "KHR/khrplatform.h", "GL/glcorearb.h"):
        (headers / name).parent.mkdir(parents=True, exist_ok=True)
        (headers / name).write_text("")
    monkeypatch.chdir(tmp_path)
    monkeypatch.setattr(linux_guest_build, "GL_HEADERS", headers)
    monkeypatch.setattr(linux_guest_build, "native_linux_guest", lambda: X86_64)
    fetches = []
    monkeypatch.setattr(linux_guest_build, "fetch_third_party", lambda third_party: fetches.append(third_party))
    return fetches


def generate(capsys, **settings):
    output = io.StringIO()
    linux_guest_build.generate_linux_guest_build(Writer(output), SimpleNamespace(**settings), X86_64)
    return output.getvalue(), capsys.readouterr().err


def test_not_asked_for_downloads_nothing(tree, capsys):
    text, err = generate(capsys)
    assert tree == []
    assert "build linux_x86_64: linux_x86_64_unavailable" in text and "--linux-x86-64" in text
    # (quietly: every configure.py of an x86 computer's default build gets here)
    assert err == ""


def test_fetched_sources_are_asked_for(tree, tmp_path):
    third_party = tmp_path / "third_party"
    assert not linux_guest_build.requested(SimpleNamespace(), X86_64, third_party)
    (third_party / f"musl-{linux_guest_build.MUSL_VERSION}").mkdir(parents=True)
    (third_party / "SDL3").mkdir()
    assert linux_guest_build.requested(SimpleNamespace(), X86_64, third_party)
    assert linux_guest_build.requested(SimpleNamespace(linux_x86_64=True), X86_64, tmp_path / "none")
    assert linux_guest_build.requested(SimpleNamespace(linux_x86_64_cc="clang"), X86_64, tmp_path / "none")


def test_compiler_that_cannot_run_is_this_target_unavailable(tree, capsys):
    text, err = generate(capsys, linux_x86_64=True, linux_x86_64_cc="/nonexistent/clang")
    assert tree == [Path("build/linux_x86_64/third_party")]
    assert "(linux_x86_64) unavailable" in err and "/nonexistent/clang" in err
    # (only the stopping target: nothing of the build written halfway)
    builds = [line for line in text.splitlines() if line.startswith("build ")]
    assert builds == ["build linux_x86_64: linux_x86_64_unavailable"], text
