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
static int read_map(void *context,uint32_t offset,uint32_t size,void *out) {
    return !fseek(context,(long)offset,SEEK_SET) && fread(out,1,size,context)==size;
}
int main(int argc,char **argv) {
    struct mcc_runtime runtime={0};unsigned char bytes[32]={0};int ok;
    if(argc==5 && !strcmp(argv[1],"probe")) {
        struct mcc_cache_source source;uint32_t sum=0;
        FILE *f=fopen(argv[2],"rb");if(!f)return 8;
        fseek(f,0,SEEK_END);source.size=(uint64_t)ftell(f);source.context=f;source.read=read_map;
        ok=mcc_resources_probe("mcc_maps/package/a10",&source,&sum);fclose(f);
        if(ok)printf("%u\n",sum);
        return ok==atoi(argv[3]) ? 0 : 9;
    }
    if((argc==4 || argc==5) && !strcmp(argv[1],"checksum")) {
        uint32_t sum;
        if(!mcc_package_name(argv[2],runtime.level_name))return 6;
        runtime.report.identity.checksum=(uint32_t)strtoul(argv[3],NULL,0);
        if(!mcc_resources_checksum(&runtime,argc==5 ? (unsigned)atoi(argv[4]) : 2,&sum))return 6;
        mcc_resources_dispose(&runtime);
        printf("%u\n",sum);return 0;
    }
    if(argc==4 && !strcmp(argv[1],"switch")) {
        int i;
        for(i=2;i<4;i++) {
            if(!mcc_package_name(argv[i],runtime.level_name))return 5;
            if(!mcc_resources_read(&runtime,2,"sound\\test__0__0",16,4,bytes,NULL))return 7;
            fwrite(bytes,1,4,stdout);
            mcc_resources_dispose(&runtime);
        }
        return 0;
    }
    if(argc<6 || argc>8)return 2;
    if(!mcc_package_name(argc>=7 ? argv[6] : "mcc_maps/test",runtime.level_name))return 5;
    ok=mcc_resources_read(&runtime,(unsigned)atoi(argv[1]),argv[2],(uint32_t)strtoul(argv[3],NULL,0),
        (uint32_t)atoi(argv[4]),bytes,argc==8 ? argv[7] : NULL);
    if(ok)fwrite(bytes,1,(size_t)atoi(argv[4]),stdout);
    mcc_resources_dispose(&runtime);mcc_resources_dispose(&runtime);
    if(runtime.resources)return 3;
    return ok==atoi(argv[5]) ? 0 : 4;
}
''')
    exe = work / "resources.exe"
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else []
    result = subprocess.run([clang] + target + ["-std=c89", "-Wall", "-Wextra", "-Werror", "-D_CRT_SECURE_NO_WARNINGS", "-I"+str(ROOT / "port/linux/game"),
                    "-I"+str(work), str(source), str(ROOT / "port/linux/game/mcc_package.c"), str(ROOT / "port/linux/game/mcc_resources.c"), str(ROOT / "port/linux/game/mcc_cache_format.c"), "-o", str(exe)], capture_output=True, text=True)
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


def test_package_overrides_root_fallback_and_release_resources(resource_tool, tmp_path):
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
    missing = subprocess.run([str(resource_tool), "2", r"sound\test__0__0", "16", "4", "1",
                              "mcc_maps/missing/a10"], cwd=tmp_path, capture_output=True, timeout=10)
    assert missing.returncode == 0
    assert missing.stdout == b"abcd"
    switched = subprocess.run([str(resource_tool), "switch", "mcc_maps/one/a10", "mcc_maps/missing/a10"],
                              cwd=tmp_path, capture_output=True, timeout=10)
    assert switched.returncode == 0
    assert switched.stdout == b"ONE!abcd"


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
    assert initial == checksum("one/a10")  # No external bitmaps requested.
    (root / "one/bitmaps.map").unlink()
    assert initial == checksum("one/a10")
    blob = fixture_blob()
    blob[16:20] = b"ROOT"
    (root / "sounds.map").write_bytes(blob)
    assert initial == checksum("one/a10")
    assert checksum("loose") != 1234  # Loose maps also fingerprint used resources.


@pytest.mark.parametrize("kind", [1, 2])
@pytest.mark.parametrize("override", ["absent", "valid", "empty", "damaged", "wrong_entry", "directory"])
def test_fallback_only_for_absent_files(resource_tool, tmp_path, kind, override):
    root = tmp_path / "mcc_maps"
    package = root / "package"
    package.mkdir(parents=True)
    filename = "bitmaps.map" if kind == 1 else "sounds.map"
    (root / filename).write_bytes(fixture_blob(kind))
    local = package / filename
    if override == "directory":
        local.mkdir()
    elif override != "absent":
        blob = fixture_blob(kind)
        blob[16:20] = b"PACK"
        if override == "empty": blob = b""
        if override == "damaged": blob = blob[:-1]
        if override == "wrong_entry": blob[24] = ord("X")
        local.write_bytes(blob)
    expected = override in {"absent", "valid"}
    result = subprocess.run([str(resource_tool), str(kind), r"sound\test__0__0", "16", "4",
                             str(int(expected)), "mcc_maps/package/a10"],
                            cwd=tmp_path, capture_output=True, timeout=10)
    assert result.returncode == 0, result.stderr
    assert result.stdout == (b"PACK" if override == "valid" else b"abcd" if expected else b"")


def test_effective_resources_determine_fingerprint(resource_tool, tmp_path):
    root = tmp_path / "mcc_maps"
    for package in ("one", "two"):
        (root / package).mkdir(parents=True)
    def checksum(level, mask, expected=0):
        result = subprocess.run([str(resource_tool), "checksum", "mcc_maps/"+level, "1234", str(mask)],
                                cwd=tmp_path, capture_output=True, timeout=10)
        assert result.returncode == expected, result.stderr
        return result.stdout
    assert checksum("one/a10", 0).strip() == b"1234"

    checksum("one/a10", 2, 6)  # Required resources absent, never admit as checksum zero.
    (root / "sounds.map").write_bytes(fixture_blob())
    shared = checksum("one/a10", 2)
    assert shared == checksum("loose", 2) == checksum("two/a10", 2)
    (root / "two/sounds.map").write_bytes(fixture_blob())
    assert shared == checksum("two/a10", 2)  # Location is irrelevant.
    blob = fixture_blob()
    blob[16] ^= 1
    (root / "sounds.map").write_bytes(blob)
    assert shared != checksum("one/a10", 2)
    assert shared == checksum("two/a10", 2)  # Override masks root changes.
    (root / "bitmaps.map").write_bytes(fixture_blob(1))
    both = checksum("two/a10", 3)  # Package sound plus root bitmap.
    assert both != shared
    (root / "two/bitmaps.map").write_bytes(fixture_blob(1))
    assert both == checksum("two/a10", 3)
    (root / "loc.map").write_bytes(b"not used")
    assert both == checksum("two/a10", 3)
    checksum("two/a10", 4, 6)
    assert checksum("one/a10", 0).strip() == b"1234"


@pytest.mark.parametrize("kind,suffix", [(1, "__pixels"), (2, "__permutations")])
@pytest.mark.parametrize("case", ["whole", "slice", "past_end", "before", "wrong_group", "duplicate", "member_conflict"])
def test_standard_grouped_payloads(resource_tool, tmp_path, kind, suffix, case):
    root = tmp_path / "mcc_maps"
    (root / "package").mkdir(parents=True)
    group = "tag"+suffix
    names = (group+"\0member\0").encode()
    entries = [(0, 8, 16)]
    if case == "duplicate": entries.append((0, 8, 16))
    if case == "member_conflict": entries.append((len(group)+1, 1, 16))
    blob = struct.pack("<4I", kind, 24, 24+len(names), len(entries))+b"abcdefgh"+names
    blob += b"".join(struct.pack("<3I", *entry) for entry in entries)
    (root / ("bitmaps.map" if kind == 1 else "sounds.map")).write_bytes(blob)
    offset, size = (16, 8) if case == "whole" else (18, 3)
    if case == "past_end": offset, size = 22, 3
    if case == "before": offset = 15
    expected = case in {"whole", "slice"}
    result = subprocess.run([str(resource_tool), str(kind), "member", str(offset), str(size),
                             str(int(expected)), "mcc_maps/package/a10", "wrong" if case == "wrong_group" else group],
                            cwd=tmp_path, capture_output=True, timeout=10)
    assert result.returncode == 0, result.stderr
    assert result.stdout == ((b"abcdefgh" if case == "whole" else b"cde") if expected else b"")


@pytest.mark.parametrize("kind", [1, 2])
@pytest.mark.parametrize("case", ["singleplayer", "multiplayer", "identical", "ambiguous", "partial", "split",
    "package", "package_bad", "root", "root_bad", "no_match", "unused", "malformed_shared"])
def test_shared_set_selection(resource_tool, tmp_path, kind, case):
    from test_mcc_cache_format import resource_dependency_map, TAG_OFFSET
    root = tmp_path / "mcc_maps"
    package = root / "package"
    package.mkdir(parents=True)
    filename = "bitmaps.map" if kind == 1 else "sounds.map"
    first = root / "_shared/singleplayer" / filename
    second = root / "_shared/multiplayer" / filename
    first.parent.mkdir(parents=True)
    second.parent.mkdir(parents=True)
    data = resource_dependency_map("bitm" if kind == 1 else "snd!", int(case != "unused"), 4)
    # Two references prevent a cache matching only the first from being accepted.
    data.extend(bytes(0x100))
    struct.pack_into("<I", data, 8, len(data))
    struct.pack_into("<I", data, 20, 0xA00)
    if kind == 1:
        struct.pack_into("<I", data, TAG_OFFSET+0x740+0x60, 2)
        data[TAG_OFFSET+0x830:TAG_OFFSET+0x860] = data[TAG_OFFSET+0x800:TAG_OFFSET+0x830]
        offsets = (0x818, 0x848)
        names = ("res_0", "res_1")
    else:
        struct.pack_into("<I", data, TAG_OFFSET+0x800+0x3C, 2)
        data[TAG_OFFSET+0x8CC:TAG_OFFSET+0x948] = data[TAG_OFFSET+0x850:TAG_OFFSET+0x8CC]
        offsets = (0x898, 0x914)
        names = ("res__0__0", "res__0__1")
    for offset,value in zip(offsets,(16,20)):
        struct.pack_into("<I", data, TAG_OFFSET+offset, value)
    (package / "a10.map").write_bytes(data)
    def blob(members=(0,1), payload=b"abcdefgh"):
        labels=b"".join(names[i].encode()+b"\0" for i in members)
        directory=bytearray();offset=0
        for i in members:
            directory += struct.pack("<3I",offset,4,16+i*4)
            offset += len(names[i])+1
        return struct.pack("<4I",kind,24,24+len(labels),len(members))+payload+labels+directory
    expected = case not in {"ambiguous", "split", "package_bad", "root_bad", "no_match", "malformed_shared"}
    first.write_bytes(blob())
    if case == "multiplayer": first.unlink();second.write_bytes(blob())
    if case == "identical": second.write_bytes(blob())
    if case == "ambiguous": second.write_bytes(blob(payload=b"ABCDEFGH"))
    if case == "partial": first.write_bytes(blob((0,)));second.write_bytes(blob())
    if case == "split": first.write_bytes(blob((0,)));second.write_bytes(blob((1,)))
    if case.startswith("package"):
        second.write_bytes(blob(payload=b"ABCDEFGH"))
        (package / filename).write_bytes(b"invalid" if case.endswith("bad") else blob())
    if case.startswith("root"):
        second.write_bytes(blob(payload=b"ABCDEFGH"))
        (root / filename).write_bytes(b"invalid" if case.endswith("bad") else blob())
    if case == "no_match": first.write_bytes(blob((0,)))
    if case in {"malformed_shared", "unused"}: first.write_bytes(b"invalid");second.write_bytes(b"invalid")
    result = subprocess.run([str(resource_tool), "probe", str(package / "a10.map"), str(int(expected)), "unused"],
                            cwd=tmp_path, capture_output=True, timeout=10)
    assert result.returncode == 0, result.stdout+result.stderr
    if expected and case != "unused":
        # Moving identical chosen data into the package preserves the fingerprint.
        (package / filename).write_bytes(blob())
        override = subprocess.run([str(resource_tool), "probe", str(package / "a10.map"), "1", "unused"],
                                 cwd=tmp_path, capture_output=True, timeout=10)
        assert override.returncode == 0
        assert override.stdout == result.stdout
