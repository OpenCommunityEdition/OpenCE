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
    (work / "cseries.h").write_text(
        "#include <stdlib.h>\n#define malloc(n) wrong_allocator(n)\n#define free(p) wrong_free(p)\n")
    (work / "errors.h").write_text("#define _error_silent 0\nvoid error(short,const char *,...);\n")
    source = work / "main.c"
    source.write_text(r'''
#include "mcc_runtime.h"
#include "mcc_resources.h"
#include "mcc_package.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void error(short level,const char *format,...) {(void)level;(void)format;}
int main(int argc,char **argv) {
    struct mcc_runtime runtime={0};unsigned char bytes[32]={0};int ok;
    if(argc==4 && !strcmp(argv[1],"checksum")) {
        uint32_t sum;
        if(!mcc_package_checksum(argv[2],(uint32_t)strtoul(argv[3],NULL,0),&sum))return 6;
        printf("%u\n",sum);return 0;
    }
    if(argc==4 && !strcmp(argv[1],"switch")) {
        int i;
        for(i=2;i<4;i++) {
            if(!mcc_package_name(argv[i],runtime.level_name))return 5;
            if(!mcc_resources_read(&runtime,2,"sound\\test__0__0",16,4,bytes))return 7;
            fwrite(bytes,1,4,stdout);
            mcc_resources_dispose(&runtime);
        }
        return 0;
    }
    if(argc<6 || argc>7)return 2;
    if(!mcc_package_name(argc==7 ? argv[6] : "mcc_maps/test",runtime.level_name))return 5;
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
                    "-I"+str(work), str(source), str(ROOT / "port/linux/game/mcc_package.c"), str(ROOT / "port/linux/game/mcc_resources.c"), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


def fixture_blob(kind=2):
    names = b"sound\\test__0__0\0second\0"
    # Deliberately reverse the directory's offset order.
    return bytearray(struct.pack("<4I", kind, 24, 24+len(names), 2) + b"abcdefgh" + names +
                     struct.pack("<6I", 17, 4, 20, 0, 4, 16))


@pytest.mark.parametrize("case", ["valid", "bitmap", "prefix", "wrong_name", "wrong_type", "truncated",
    "metadata_overlap", "past_end", "names_outside", "unterminated", "large_count", "inner_offset",
    "too_long", "missing", "ce_only", "wrap_offset", "relocated", "relocated_prefix", "ambiguous"])
def test_resource_reader(resource_tool, tmp_path, case):
    folder = tmp_path / ("custom_maps" if case == "ce_only" else "mcc_maps")
    folder.mkdir()
    kind = 1 if case == "bitmap" else 2
    blob = fixture_blob(kind)
    expected = case in {"valid", "bitmap", "prefix", "inner_offset", "relocated"}
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
    if case == "relocated": offset = 20
    if case == "relocated_prefix": offset, length = 20, 2
    if case == "ambiguous":
        offset = 18
        struct.pack_into("<I", blob, index, 0)
    if case != "missing": (folder / ("bitmaps.map" if kind == 1 else "sounds.map")).write_bytes(blob)
    result = subprocess.run([str(resource_tool), str(kind), name, str(offset), str(length), str(int(expected))],
                            cwd=tmp_path, capture_output=True, timeout=10)
    assert result.returncode == 0, result.stderr
    assert result.stdout == (b"abcd"[:length] if expected else b"")


def test_packages_never_fall_back_and_release_resources(resource_tool, tmp_path):
    root = tmp_path / "mcc_maps"
    for name in ("one", "two", "missing"):
        (root / name).mkdir(parents=True)
    (root / "sounds.map").write_bytes(fixture_blob())
    for name, value in (("one", b"ONE!"), ("two", b"TWO!")):
        blob = fixture_blob()
        blob[16:20] = value
        (root / name / "sounds.map").write_bytes(blob)
    switched = subprocess.run([str(resource_tool), "switch", "mcc_maps/one/a10", "mcc_maps/two/a10"],
                              cwd=tmp_path, capture_output=True, timeout=10)
    assert switched.returncode == 0, switched.stderr
    assert switched.stdout == b"ONE!TWO!"
    missing = subprocess.run([str(resource_tool), "2", r"sound\test__0__0", "16", "4", "0",
                              "mcc_maps/missing/a10"], cwd=tmp_path, capture_output=True, timeout=10)
    assert missing.returncode == 0
    assert missing.stdout == b""


def test_package_fingerprints_cover_contents_not_paths(resource_tool, tmp_path):
    root = tmp_path / "mcc_maps"
    for name in ("one", "two"):
        (root / name).mkdir(parents=True)
        (root / name / "sounds.map").write_bytes(fixture_blob())
    def checksum(name, base="1234"):
        result = subprocess.run([str(resource_tool), "checksum", "mcc_maps/" + name, base],
                                cwd=tmp_path, capture_output=True, timeout=10)
        assert result.returncode == 0, result.stderr
        return int(result.stdout)
    initial = checksum("one/a10")
    assert initial == checksum("two/a10")
    assert initial != checksum("one/a10", "1235")
    blob = fixture_blob()
    blob[16] ^= 1  # Identical length and resource names, different audio bytes.
    (root / "two/sounds.map").write_bytes(blob)
    assert initial != checksum("two/a10")
    (root / "one/bitmaps.map").write_bytes(b"")
    assert initial != checksum("one/a10")
    (root / "one/bitmaps.map").unlink()
    assert initial == checksum("one/a10")
    (root / "sounds.map").write_bytes(b"unrelated root resources")
    assert initial == checksum("one/a10")
    assert checksum("loose") == 1234
