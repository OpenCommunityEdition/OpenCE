"""Generated MCC resource files: identity, bounds, missing files and lifetime."""
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def resource_tool(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang required")
    work = tmp_path_factory.mktemp("mcc-resources")
    source = work / "main.c"
    source.write_text(r'''
#include "mcc_runtime.h"
#include "mcc_resources.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(int argc,char **argv) {
    struct mcc_runtime runtime={0};unsigned char bytes[32]={0};int ok;
    if(argc!=6)return 2;
    ok=mcc_resources_read(&runtime,(unsigned)atoi(argv[1]),argv[2],(uint32_t)strtoul(argv[3],NULL,0),
        (uint32_t)atoi(argv[4]),bytes);
    if(ok)fwrite(bytes,1,(size_t)atoi(argv[4]),stdout);
    mcc_resources_dispose(&runtime);mcc_resources_dispose(&runtime);
    if(runtime.resources)return 3;
    return ok==atoi(argv[5]) ? 0 : 4;
}
''')
    exe = work / "resources.exe"
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else []
    result = subprocess.run([clang] + target + ["-std=c89", "-Wall", "-Wextra", "-Werror", "-D_CRT_SECURE_NO_WARNINGS", "-I"+str(ROOT / "port/linux/game"),
                    str(source), str(ROOT / "port/linux/game/mcc_resources.c"), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


def fixture_blob(kind=2):
    names = b"sound\\test__0__0\0second\0"
    # Deliberately reverse the directory's offset order.
    return bytearray(struct.pack("<4I", kind, 24, 24+len(names), 2) + b"abcdefgh" + names +
                     struct.pack("<6I", 17, 4, 20, 0, 4, 16))


@pytest.mark.parametrize("case", ["valid", "bitmap", "prefix", "wrong_name", "wrong_type", "truncated",
    "metadata_overlap", "past_end", "names_outside", "unterminated", "large_count", "inner_offset",
    "too_long", "missing", "ce_only", "wrap_offset"])
def test_resource_reader(resource_tool, tmp_path, case):
    folder = tmp_path / ("custom_maps" if case == "ce_only" else "mcc_maps")
    folder.mkdir()
    kind = 1 if case == "bitmap" else 2
    blob = fixture_blob(kind)
    expected = case in {"valid", "bitmap", "prefix"}
    offset, length, name = 16, 4, r"sound\test__0__0"
    index = struct.unpack_from("<I", blob, 8)[0]
    if case == "wrong_name": name = "different"
    if case == "wrong_type": struct.pack_into("<I", blob, 0, 1)
    if case == "truncated": blob = blob[:-1]
    if case == "metadata_overlap": struct.pack_into("<I", blob, index+20, 24)
    if case == "past_end": struct.pack_into("<I", blob, 8, 0x7fffffff)
    if case == "names_outside": struct.pack_into("<I", blob, index, 0xffffffff)
    if case == "unterminated": blob[24:index] = b"x"*(index-24)
    if case == "large_count": struct.pack_into("<I", blob, 12, 0xffffffff)
    if case == "inner_offset": offset = 17
    if case == "too_long": length = 5
    if case == "prefix": length = 2
    if case == "wrap_offset": offset = 0xffffffff
    if case != "missing": (folder / ("bitmaps.map" if kind == 1 else "sounds.map")).write_bytes(blob)
    result = subprocess.run([str(resource_tool), str(kind), name, str(offset), str(length), str(int(expected))],
                            cwd=tmp_path, capture_output=True, timeout=10)
    assert result.returncode == 0, result.stderr
    assert result.stdout == (b"abcd"[:length] if expected else b"")
