"""Tests for the native Linux build tooling (port/linux, tools/linux_*.py)."""

import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

from tools import linux_build, linux_link_check, linux_msvc_semantics


# ---------- MSVC semantics header


def test_strip_cplusplus_keeps_c_branches_only():
    text = "\n".join([
        "a",
        "#ifdef __cplusplus", "cpp1", "#else", "c1", "#endif",
        "#if FOO", "b", "#else", "c", "#endif",
        "#ifndef __cplusplus", "c2", "#else", "cpp2", "#endif",
        "#if defined(__cplusplus)", "cpp3", "#if X", "cpp4", "#endif", "#endif",
        "#ifdef _WIN64", "win64", "#endif",
        "end",
    ])
    assert linux_msvc_semantics.strip_cplusplus(text).split() == ["a", "c1", "b", "c", "c2", "end"]


def write(path: Path, text: str) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="latin-1")
    return path


def test_scan_finds_prototype_scope_tags_and_inline_functions(tmp_path):
    header = write(tmp_path / "a.h", """
        struct location;
        void f(struct scenario *s, union color *c);
        /* struct commented_out */
        __inline long prototyped(long x) { return x; }
        __inline long private_helper(long x) { return x; }
        long prototyped(long x);
        #ifdef __cplusplus
        __inline long cpp_only(long x) { return x; }
        #endif
    """)
    files = [header]
    tags = linux_msvc_semantics.scan_tags(files)
    assert tags == {"location": {"struct"}, "scenario": {"struct"}, "color": {"union"}}
    assert linux_msvc_semantics.scan_inline_functions(files, all_inlines=False) == {"prototyped"}
    assert linux_msvc_semantics.scan_inline_functions(files, all_inlines=True) == {
        "prototyped", "private_helper",
    }


def test_render_skips_tags_used_as_both_struct_and_union():
    text = linux_msvc_semantics.render(
        {"a": {"struct"}, "b": {"union"}, "mixed": {"struct", "union"}}, {"f"}
    )
    assert "struct a;" in text
    assert "union b;" in text
    assert "mixed" not in text
    assert "#pragma weak f" in text


# ---------- Xbox SDK declarations (port/include/xdk)


XDK_INCLUDE = Path(__file__).resolve().parent.parent / "port" / "include" / "xdk"


def test_xdk_headers_use_the_sdk_spellings():
    # each port gives the SDK's keywords and names its own meaning (its
    # prefix header), so none of one port's must be written into them
    for header in sorted(XDK_INCLUDE.glob("*.h")):
        text = re.sub(r"/\*.*?\*/", "", header.read_text(encoding="utf-8"), flags=re.S)
        assert "__attribute__" not in text, header.name
        assert not re.findall(r"\bhalo_\w+", text), header.name


@pytest.mark.skipif(shutil.which("clang") is None, reason="clang is needed to compile the headers")
def test_xdk_headers_compile_for_the_game(tmp_path):
    root = XDK_INCLUDE.parent.parent.parent
    source = write(tmp_path / "unit.c", "".join(
        f"#include <{name}>\n" for name in ("xtl.h", "xbdm.h", "xkbd.h", "d3d8perf.h")
    ))
    flags = [flag for flag in linux_build.LINUX_ABI_FLAGS + linux_build.GAME_FLAGS if flag != "-w"]
    for defines in ([], ["-DDEBUG_KEYBOARD"], ["-DNOD3D", "-DNODSOUND"]):
        subprocess.run(
            ["clang", *flags, *defines, "-Werror", "-fsyntax-only",
             "-include", str(root / "port/linux/include/halo_linux_prefix.h"),
             "-I", str(root / "port/linux/include"), "-idirafter", str(XDK_INCLUDE), str(source)],
            check=True,
        )


def pdb_writer():
    """An xdk_headers Writer over an empty type table: basic types only."""
    from tools import pdb200_types, xdk_headers
    pdb = object.__new__(pdb200_types.Pdb)
    pdb.types, pdb.aggregates = {}, {}
    return xdk_headers, xdk_headers.Writer(pdb)


def test_xdk_headers_regroups_anonymous_members():
    # the PDB lists an anonymous structure's or union's members in their
    # container, at their offsets
    xdk_headers, writer = pdb_writer()
    member = xdk_headers.Member
    ulong, int64 = 0x22, 0x13
    union = writer.render(writer.group(
        [member("LowPart", ulong, 0), member("HighPart", ulong, 4), member("QuadPart", int64, 0)], True), "", "u")
    assert union == ["struct {", "    unsigned long LowPart;", "    unsigned long HighPart;", "};",
                     "__int64 QuadPart;"]
    structure = writer.render(writer.group(
        [member("Tag", ulong, 0), member("Low", ulong, 4), member("High", ulong, 8),
         member("Whole", int64, 4), member("After", ulong, 12)], False), "", "s")
    assert structure == ["unsigned long Tag;", "union {", "    struct {", "        unsigned long Low;",
                         "        unsigned long High;", "    };", "    __int64 Whole;", "};",
                         "unsigned long After;"]


# ---------- weak reference link check


@pytest.mark.skipif(shutil.which("clang") is None or shutil.which("nm") is None,
                    reason="clang and nm are needed to build test objects")
def test_link_check_rejects_undefined_weak_references(tmp_path):
    def compile_object(name: str, text: str) -> Path:
        source = write(tmp_path / f"{name}.c", text)
        output = tmp_path / f"{name}.o"
        subprocess.run(["clang", "-c", "-o", str(output), str(source)], check=True)
        return output

    caller = compile_object("caller", "#pragma weak helper\nint helper(int);\nint call(void) { return helper(1); }\n")
    local = compile_object("local", "static int helper(int x) { return x; }\nint use(void) { return helper(2); }\n")
    provider = compile_object("provider", "#pragma weak helper\nint helper(int x) { return x; }\n")

    def check(*objects: Path) -> int:
        response = write(tmp_path / "objects.rsp", "\n".join(f"'{o}'" for o in objects))
        return subprocess.run(
            [sys.executable, linux_link_check.__file__, str(response)], capture_output=True
        ).returncode

    # a file-local copy elsewhere does not satisfy the reference
    assert check(caller, local) == 1
    assert check(caller, local, provider) == 0


# ---------- game sources


def test_game_sources_leave_out_the_excluded(tmp_path, monkeypatch):
    write(tmp_path / "source" / "a.c", "")
    write(tmp_path / "source" / "zlib" / "b.c", "")
    write(tmp_path / "source" / "zlib" / "example.c", "")
    write(tmp_path / "source" / "a.h", "")
    monkeypatch.chdir(tmp_path)
    config = {"game": {"root": "source", "exclude": ["source/zlib/example.c"],
                       "defines": ["DEBUG"], "include_dirs": ["source", "source/saved games"]}}
    assert linux_build.game_sources(config) == [Path("source/a.c"), Path("source/zlib/b.c")]
    assert linux_build.game_defines_and_includes(config) == '-DDEBUG -Isource -I"source/saved games"'
