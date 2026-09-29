"""Tests for the native Linux build tooling (port/linux, tools/linux_*.py)."""

import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

from tools import linux_build, linux_link_check, linux_msvc_semantics, msvc_deps_filter
from tools.download_tool import uasm_url
from tools.project_x86 import (
    Object,
    ObjectStatus,
    ProjectConfig,
    SolutionConfig,
    generate_build_ninja,
)


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


# ---------- /showIncludes filter


def test_deps_filter_resolves_windows_style_paths(tmp_path, monkeypatch):
    write(tmp_path / "source" / "cseries" / "cseries.h", "")
    write(tmp_path / "xbox" / "include" / "PopPack.h", "")
    monkeypatch.chdir(tmp_path)
    msvc_deps_filter.resolve.cache_clear()
    msvc_deps_filter.directory_entries.cache_clear()

    prefix = msvc_deps_filter.INCLUDE_PREFIX
    assert msvc_deps_filter.filter_line(f"{prefix} source/cseries\\cseries.h\n") == \
        f"{prefix} source/cseries/cseries.h\n"
    assert msvc_deps_filter.filter_line(f"{prefix} xbox/include\\POPPACK.H\n") == \
        f"{prefix} xbox/include/PopPack.h\n"
    # wibo reports absolute paths on drive Z:, in lower case
    absolute = "z:" + str(tmp_path).lower().replace("/", "\\") + "\\xbox\\include\\poppack.h"
    assert msvc_deps_filter.filter_line(f"{prefix} {absolute}\n") == \
        f"{prefix} xbox/include/PopPack.h\n"
    # unknown files and ordinary output pass through untouched
    assert msvc_deps_filter.filter_line(f"{prefix} missing\\file.h\n") == f"{prefix} missing\\file.h\n"
    assert msvc_deps_filter.filter_line("cseries.c\n") == "cseries.c\n"


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


# ---------- MASM stand-in and build graph


def test_uasm_release_url():
    assert uasm_url("v2.57r") == \
        "https://github.com/Terraspace/UASM/releases/download/v2.57r/uasm257_linux64.zip"


def make_solution(tmp_path, monkeypatch, **settings) -> SolutionConfig:
    monkeypatch.chdir(tmp_path)
    monkeypatch.setattr(sys, "argv", ["configure.py"])
    sln = SolutionConfig()
    sln.build_dir = Path("build")
    sln.config_dir = Path("config")
    sln.tools_dir = Path("tools")
    sln.baserom = Path("target.exe")
    sln.objdiff_path = Path("objdiff-cli")
    sln.csplit_path = Path("csplit")
    for key, value in settings.items():
        setattr(sln, key, value)
    project = ProjectConfig(cflags=[], defines=[], include_dirs=[], asmflags=[])
    project.name = "libcmt"
    project.guid = "test"
    project.objects = [Object(ObjectStatus.Matching, Path("libs/libcmt/llshr.asm"))]
    sln.projects = [project]
    return sln


def test_uasm_assembles_asm_units_when_no_masm_exists(tmp_path, monkeypatch):
    sln = make_solution(tmp_path, monkeypatch, uasm_tag="v2.57r", wrapper=Path("wine"))
    monkeypatch.setattr(SolutionConfig, "use_uasm", lambda self: True)
    generate_build_ninja(sln)
    ninja = re.sub(r"\$\n\s+", "", Path("build.ninja").read_text(encoding="utf-8"))
    assert "command = build/tools/uasm/uasm -nologo -c -coff $asmflags -Fo$out $in" in ninja
    assert "tool = uasm" in ninja
    assert "llshr.obj: ml libs/libcmt/llshr.asm | build/tools/uasm" in ninja


def test_build_graph_without_port_has_no_linux_target(tmp_path, monkeypatch):
    sln = make_solution(tmp_path, monkeypatch)
    generate_build_ninja(sln)
    ninja = Path("build.ninja").read_text(encoding="utf-8")
    assert "linux_cc" not in ninja
    assert "build linux:" not in ninja


# ---------- the disc image importer (port/linux/src/xiso.c)


# The importer is built for 32-bit Windows and Linux, but its disc reading is
# plain POSIX file I/O, so it can be compiled for the test host with a small
# harness. platform.h is a stub: the importer only uses platform_log from it,
# and that keeps the Xbox SDK headers out of the test.
XISO_HARNESS = r"""
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "posix.h"
#include "xiso.h"

void platform_log(const char *format, ...)
{
	(void)format;
}

int posix_seek(int descriptor, posix_long offset_low, posix_long offset_high, int whence,
	posix_ulong *position_low, posix_ulong *position_high)
{
	off_t offset = (off_t)(unsigned int)offset_low | ((off_t)(unsigned int)offset_high << 32);
	off_t result = lseek(descriptor, offset, whence);

	if (result < 0)
		return -1;
	*position_low = (posix_ulong)result;
	*position_high = (posix_ulong)((unsigned long long)result >> 32);
	return 0;
}

int posix_fstat(int descriptor, struct posix_file_information *information)
{
	struct stat status;

	if (fstat(descriptor, &status) != 0)
		return -1;
	memset(information, 0, sizeof(*information));
	information->size_low = (posix_ulong)((unsigned long long)status.st_size & 0xFFFFFFFFu);
	information->size_high = (posix_ulong)((unsigned long long)status.st_size >> 32);
	return 0;
}

int posix_make_directory(const char *path)
{
	return mkdir(path, 0755);
}

int main(int argc, char **argv)
{
	if (argc < 3)
		return 2;
	if (!strcmp(argv[1], "probe"))
	{
		struct xiso_probe_result result;
		int read = xiso_probe(argv[2], &result);

		printf("read=%d halo=%d maps=%d data=%llu file=%llu complete=%d\n",
			read, result.halo, result.map_count, result.data_size, result.file_size, result.complete);
		return 0;
	}
	if (!strcmp(argv[1], "extract"))
	{
		char error[512];
		int ok;

		memset(error, 0, sizeof(error));
		ok = xiso_extract_maps(argv[2], argv[3], NULL, NULL, error, sizeof(error));
		printf("ok=%d error=%s\n", ok, error);
		return ok ? 0 : 1;
	}
	return 2;
}
"""

REPOSITORY = Path(__file__).resolve().parent.parent
XISO_SOURCE = REPOSITORY / "port" / "linux" / "src" / "xiso.c"


@pytest.fixture(scope="module")
def xiso_harness(tmp_path_factory):
    """The importer compiled for this host, or a skip where there is no clang."""
    if shutil.which("clang") is None:
        pytest.skip("clang is needed to compile the disc image importer")
    directory = tmp_path_factory.mktemp("xiso")
    (directory / "platform.h").write_text(
        "#ifndef TEST_PLATFORM_H\n#define TEST_PLATFORM_H\n"
        "void platform_log(const char *format, ...);\n#endif\n",
        encoding="latin-1",
    )
    shutil.copy2(XISO_SOURCE, directory / "xiso_under_test.c")
    (directory / "harness.c").write_text(XISO_HARNESS, encoding="latin-1")
    binary = directory / "xiso_harness"
    built = subprocess.run(
        [
            "clang", "-std=gnu11", "-O0", "-g", "-Wall",
            "-I", str(directory),
            "-I", str(REPOSITORY / "port" / "linux" / "src"),
            "-o", str(binary), str(directory / "harness.c"), str(directory / "xiso_under_test.c"),
        ],
        capture_output=True, text=True,
    )
    if built.returncode != 0:
        # some hosts (a macOS SDK newer than their clang's linker) cannot
        # link an executable although they compile the source; the tests then
        # have nothing to run. CI's Arch Linux container links it.
        pytest.skip("cannot link the disc image harness on this host:\n" + built.stderr.strip())
    return binary


SECTOR_SIZE = 2048


def directory_table(entries) -> bytes:
    """An XDVDFS directory: entries (sector, size, attributes, name) chained
    through their right-subtree offsets, as the Xbox disc format stores them."""
    offsets = []
    offset = 0
    for _, _, _, name in entries:
        offsets.append(offset)
        offset += (14 + len(name) + 3) & ~3
    table = bytearray(offset)
    for index, (sector, size, attributes, name) in enumerate(entries):
        at = offsets[index]
        table[at + 4:at + 8] = sector.to_bytes(4, "little")
        table[at + 8:at + 12] = size.to_bytes(4, "little")
        table[at + 12] = attributes
        table[at + 13] = len(name)
        table[at + 14:at + 14 + len(name)] = name.encode("ascii")
        if index + 1 < len(offsets):
            table[at + 2:at + 4] = (offsets[index + 1] // 4).to_bytes(2, "little")
    return bytes(table)


def halo_image(files=None, maps_name="maps", ui=True) -> bytes:
    """A small but valid XDVDFS image holding a maps folder."""
    if files is None:
        files = {"ui.map": b"ui" * 16, "a10.map": b"a10" * 32}
    if not ui:
        files = {name: data for name, data in files.items() if name != "ui.map"}
    payloads = list(files.items())
    sectors = []
    cursor = 35
    for _, payload in payloads:
        sectors.append(cursor)
        cursor += max(1, (len(payload) + SECTOR_SIZE - 1) // SECTOR_SIZE)
    maps_table = directory_table(
        [(sectors[i], len(payloads[i][1]), 0, payloads[i][0]) for i in range(len(payloads))]
    )
    root_table = directory_table([(34, len(maps_table), 0x10, maps_name)])
    assert len(root_table) <= SECTOR_SIZE
    image = bytearray(cursor * SECTOR_SIZE)
    descriptor = 32 * SECTOR_SIZE
    image[descriptor:descriptor + 20] = b"MICROSOFT*XBOX*MEDIA"
    image[descriptor + 0x7EC:descriptor + 0x7EC + 20] = b"MICROSOFT*XBOX*MEDIA"
    image[descriptor + 20:descriptor + 24] = (33).to_bytes(4, "little")
    image[descriptor + 24:descriptor + 28] = len(root_table).to_bytes(4, "little")
    image[33 * SECTOR_SIZE:33 * SECTOR_SIZE + len(root_table)] = root_table
    image[34 * SECTOR_SIZE:34 * SECTOR_SIZE + len(maps_table)] = maps_table
    for (_, payload), sector in zip(payloads, sectors):
        image[sector * SECTOR_SIZE:sector * SECTOR_SIZE + len(payload)] = payload
    return bytes(image)


def probe(xiso_harness, path):
    result = subprocess.run([xiso_harness, "probe", path], capture_output=True, text=True, check=True)
    return {key: int(value) for key, value in (part.split("=") for part in result.stdout.split())}


def test_xiso_probe_recognises_a_halo_disc(xiso_harness, tmp_path):
    image = tmp_path / "halo.iso"
    image.write_bytes(halo_image())
    fields = probe(xiso_harness, image)
    assert fields["read"] == 1
    assert fields["halo"] == 1
    assert fields["maps"] == 2
    assert fields["complete"] == 1
    assert fields["file"] == image.stat().st_size


def test_xiso_probe_rejects_a_disc_without_halo_maps(xiso_harness, tmp_path):
    image = tmp_path / "other.iso"
    image.write_bytes(halo_image(maps_name="content"))
    fields = probe(xiso_harness, image)
    assert fields["read"] == 1
    assert fields["halo"] == 0


def test_xiso_probe_requires_ui_map(xiso_harness, tmp_path):
    image = tmp_path / "nohud.iso"
    image.write_bytes(halo_image(ui=False))
    assert probe(xiso_harness, image)["halo"] == 0


def test_xiso_probe_marks_a_truncated_image_incomplete(xiso_harness, tmp_path):
    image = tmp_path / "cut.iso"
    image.write_bytes(halo_image()[:-SECTOR_SIZE])
    fields = probe(xiso_harness, image)
    assert fields["halo"] == 1
    assert fields["complete"] == 0


def test_xiso_probe_ignores_a_file_that_is_not_a_disc(xiso_harness, tmp_path):
    image = tmp_path / "notes.txt"
    image.write_bytes(b"not an Xbox disc image" * 64)
    fields = probe(xiso_harness, image)
    assert fields["read"] == 1
    assert fields["halo"] == 0


def test_xiso_extracts_the_maps_folder(xiso_harness, tmp_path):
    files = {"ui.map": b"ui-data", "a10.map": b"a10-data"}
    image = tmp_path / "halo.iso"
    image.write_bytes(halo_image(files=files))
    destination = tmp_path / "data"
    destination.mkdir()
    subprocess.run([xiso_harness, "extract", image, destination], check=True)
    for name, payload in files.items():
        assert (destination / "maps" / name).read_bytes() == payload
    assert not (destination / "maps.partial").exists()
