"""Tests the Windows x64 host's switches between the host and the guest
(port/windows/host/win64_switch.S and the thunks of
tools/windows_host_thunks.py) with port/windows/tests/thunk_test.c, built
against Wine's Windows headers and import libraries and run under Wine,
headless (a console program in a Wine prefix of its own, no window). Skipped
where clang, lld-link, Wine or Wine's headers are missing."""
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
TESTS = ROOT / "port" / "windows" / "tests"
HOST = ROOT / "port" / "windows" / "host"
WINE_INCLUDE = Path("/usr/include/wine")
WINE_LIBRARIES = Path("/usr/lib/wine/x86_64-windows")

sys.path.insert(0, str(ROOT))
from tools.windows_x64_build import wine_crt_sections  # noqa: E402


def tools_present():
    return (shutil.which("clang") and shutil.which("lld-link") and shutil.which("wine")
            and (WINE_INCLUDE / "windows" / "windows.h").is_file() and (WINE_LIBRARIES / "libucrtbase.a").is_file())


@pytest.mark.skipif(not tools_present(), reason="needs clang, lld-link, Wine and Wine's headers")
def test_windows_host_thunks(tmp_path):
    thunks, table = tmp_path / "thunks.S", tmp_path / "table.c"
    subprocess.run([sys.executable, str(ROOT / "tools" / "windows_host_thunks.py"), str(thunks), str(table),
                    str(TESTS / "thunk_test.list"), "--declarations", str(TESTS / "thunk_test.h")], check=True)
    sections = tmp_path / "crt_sections.c"
    sections.write_text(wine_crt_sections(), encoding="utf-8")
    resource = subprocess.run(["clang", "-print-resource-dir"], capture_output=True, text=True, check=True).stdout.strip()
    flags = ["--target=x86_64-pc-windows-msvc", "-nostdinc", "-isystem", f"{resource}/include",
             "-isystem", str(WINE_INCLUDE / "msvcrt"), "-isystem", str(WINE_INCLUDE / "windows"), "-D_UCRT",
             "-O1", "-Wall", "-Werror", "-Wno-unused-function", f"-I{TESTS}"]
    objects = []
    for source in [TESTS / "thunk_test.c", HOST / "win64_switch.S", thunks, table, sections]:
        obj = tmp_path / (source.name + ".o")
        subprocess.run(["clang", *flags, "-c", str(source), "-o", str(obj)], check=True)
        objects.append(str(obj))
    exe = tmp_path / "thunk_test.exe"
    subprocess.run(["lld-link", "/nologo", f"/out:{exe}", "/subsystem:console", "/entry:mainCRTStartup",
                    "/nodefaultlib", *objects,
                    *[str(WINE_LIBRARIES / f"lib{name}.a") for name in ("ucrtbase", "kernel32", "ntdll", "compiler-rt")]],
                   check=True)
    environment = dict(os.environ, WINEPREFIX=str(tmp_path / "prefix"), WINEDEBUG="-all",
                       WINEDLLOVERRIDES="mscoree,mshtml=")
    for name in ("DISPLAY", "WAYLAND_DISPLAY"):
        environment.pop(name, None)
    try:
        result = subprocess.run(["wine", str(exe)], capture_output=True, text=True, env=environment, timeout=300)
    finally:
        subprocess.run(["wineserver", "-k"], env=environment, capture_output=True)
    assert result.returncode == 0 and "thunk test passed" in result.stdout, result.stdout + result.stderr[-2000:]
