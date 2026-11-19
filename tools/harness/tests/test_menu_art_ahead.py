"""The menus' pictures decoded ahead on a thread of their own (menu_files.c, menu_art_ahead.c): drawn from their texels
without decoding again, forgetting waits for the thread, what is held stays within its bound, one drawn while queued
is decoded once, one that does not decode is not drawn.

    python tools/harness/tests/test_menu_art_ahead.py
"""

import functools
import os
import subprocess
import sys
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, mutated, read, run  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
CASES = ["ahead", "forgotten", "bounded", "drawn-queued", "broken"]

# a fault in the code, and the case that must catch it
NEGATIVE_CONTROLS = {
    "forget-does-not-wait": ("void halo_menus_art_forget(void)\n{\n\tlong index;\n\n\tahead_stop();\n",
                             "void halo_menus_art_forget(void)\n{\n\tlong index;\n\n", "forgotten"),
    "no-bound": ("\tif (ahead_bytes + width * height * 4 > MAXIMUM_AHEAD_BYTES)\n", "\tif (0)\n", "bounded"),
    "queued-left-queued": ("\t/* (one still queued is decoded here instead) */\n\tahead_free(index);\n",
                           "\tif (art[index].ahead != _ahead_queued)\n\t\tahead_free(index);\n", "drawn-queued"),
    "texels-not-used": ("\tif (!decoded)\n\t{\n\t\tunsigned long size = 0;",
                        "\tif (decoded)\n\t\tfree(pixels);\n\t{\n\t\tunsigned long size = 0;", "ahead"),
}


def under_test(fault=None):
    menu = read("port/linux/src/menu_files.c").replace("\r\n", "\n")
    if fault:
        menu = mutated(menu, fault[0], fault[1])
    return (("under_test.inc", menu[menu.index("/* ---------- art */"):]),)


_builds = tempfile.TemporaryDirectory(prefix="halo-menu-art-")


@functools.lru_cache(maxsize=None)
def build(generated):
    """menu_art_ahead.c with real threads (the Windows build's POSIX layer on Windows)"""
    work = Path(_builds.name) / f"{abs(hash(generated)):x}"
    work.mkdir(parents=True, exist_ok=True)
    for name, text in generated:
        (work / name).write_text(text)
    executable = work / "menu_art_ahead.exe"
    command = [os.environ.get("CC", "clang"), "-m32", "-std=gnu99", "-O2", "-Wall", "-Werror", "-Wno-unused-function",
               "-Wno-unused-variable", "-I", str(ROOT / "tools/harness/include"), "-I", str(work),
               str(ROOT / "tools/harness/tests/menu_art_ahead.c"), "-o", str(executable)]
    if sys.platform == "win32":
        command += ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS",
                    "-I", str(ROOT / "port/windows/include/posix"), str(ROOT / "port/windows/src/win32_posix.c"),
                    "-lwinmm"]
    else:
        command += ["-pthread"]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f"menu_art_ahead.c does not compile:\n{result.stderr}")
    return executable


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build(under_test()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    before, after, case = NEGATIVE_CONTROLS[control]
    status, output = run(build(under_test((before, after))), case)
    assert status == CHECK_FAILED, f"the code with a fault ({control}) passed '{case}': the test cannot see it"


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
