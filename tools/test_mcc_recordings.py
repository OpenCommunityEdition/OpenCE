"""Generated recording streams exercise the production MCC-only conversion.

No map assets are included. Cover both playback codecs, all control versions,
invalid selections after variable-sized events, truncation and tag bounds.
"""
from pathlib import Path
import shutil
import struct
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def recording_tool(tmp_path_factory):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang is needed for MCC recording tests")
    work = tmp_path_factory.mktemp("mcc-recordings")
    (work / "cseries.h").write_text("#define TRUE 1\n#define FALSE 0\n#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))\n")
    (work / "errors.h").write_text("#define _error_silent 0\nvoid error(int level, char const *format, ...);\n")
    source = work / "runner.c"
    source.write_text(r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mcc_recordings.h"
#include "mcc_runtime.h"
int mcc_tags_prepare(struct mcc_runtime *r);
void error(int level,char const *format,...) {(void)level;(void)format;}
void *mcc_runtime_pointer(struct mcc_runtime *r,uint32_t address,uint32_t size) {
    uint32_t offset=address-r->report.tag_base;
    return address>=r->report.tag_base && offset<=r->used && size<=r->used-offset ? r->tags+offset : NULL;
}
static void put(unsigned char *p,uint32_t value) {memcpy(p,&value,4);}
int main(int argc,char **argv) {
    unsigned char data[65536];uint32_t size,corrected=0;int ok;
    FILE *f;struct mcc_runtime r={0};unsigned char *sc,*rec;
    if(argc!=6)return 2;
    memset(data,0,sizeof(data));f=fopen(argv[3],"rb");if(!f)return 3;
    size=(uint32_t)fread(data+0x700,1,sizeof(data)-0x700,f);fclose(f);
    if(!strcmp(argv[5],"stream"))
        ok=mcc_recording_prepare(data+0x700,size,atoi(argv[1]),atoi(argv[2]),&corrected);
    else {
        r.tags=r.tag_index=data;r.used=0x700+size;r.report.tag_base=0x50000000;r.report.tag_count=1;
        put(data,'scnr');put(data+20,0x50000020);sc=data+0x20;rec=data+0x600;
        put(sc+0x36C,1);put(sc+0x370,0x50000600);
        rec[0x20]=(unsigned char)atoi(argv[1]);rec[0x22]=(unsigned char)atoi(argv[2]);
        put(rec+0x2C,size);put(rec+0x38,0x50000700);
        if(!strcmp(argv[5],"bad-block"))put(sc+0x36C,0xFFFFFFFF);
        if(!strcmp(argv[5],"bad-pointer"))put(rec+0x38,0xFFFFFFFF);
        if(!strcmp(argv[5],"bad-size"))put(rec+0x2C,0xFFFFFFFF);
        if(!strcmp(argv[5],"disabled")) {rec[0x20]=0;put(rec+0x2C,0);put(rec+0x38,0);}
        ok=mcc_tags_prepare(&r);
    }
    f=fopen(argv[4],"wb");if(!f)return 4;fwrite(data+0x700,1,size,f);fclose(f);
    printf("%d %u\n",ok,(unsigned)corrected);return 0;
}
''')
    binary = work / ("recordings.exe" if sys.platform == "win32" else "recordings")
    game = ROOT / "port/linux/game"
    command = [clang, "-std=gnu89", "-Wall", "-Wextra", "-Werror", "-Wno-multichar",
               "-I" + str(work), "-I" + str(game), str(source), str(game / "mcc_recordings.c"),
               str(game / "mcc_tags.c"), "-o", str(binary)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return binary


def stream(version=4, control=4, weapon=-256):
    data = bytearray(52 + sum(4 if i == 1 else 2 for i in range(1, control)) + (12 if version == 4 else 0))
    struct.pack_into("<h", data, 4, weapon)
    return data


def event(version, kind, payload=b"", time=0):
    if version == 4:
        return bytes([(kind << 2) | time]) + ({0: b"", 1: b"", 2: b"\x11", 3: b"\x23\x01"}[time]) + payload
    return struct.pack("<HH", kind, 37) + payload


def convert(tool, tmp, data, version=4, control=4, mode="stream"):
    src, dst = tmp / "in.bin", tmp / "out.bin"
    src.write_bytes(data)
    result = subprocess.run([str(tool), str(version), str(control), str(src), str(dst), mode],
                            capture_output=True, text=True, check=True)
    return tuple(map(int, result.stdout.split())), dst.read_bytes()


@pytest.mark.parametrize("version", [1, 2, 3, 4])
@pytest.mark.parametrize("control", [0, 1, 2, 3, 4])
def test_invalid_initial_selection(recording_tool, tmp_path, version, control):
    data = stream(version, control) + event(version, 1)
    expected = bytearray(data);expected[4:6] = b"\xff\xff"
    assert convert(recording_tool, tmp_path, data, version, control) == ((1, 1), expected)


@pytest.mark.parametrize("version", [1, 2, 3, 4])
@pytest.mark.parametrize("weapon", [-1, 0, 1, 2, 3])
def test_valid_selections_unchanged(recording_tool, tmp_path, version, weapon):
    data = stream(version, weapon=weapon) + event(version, 5, struct.pack("<h", weapon)) + event(version, 1)
    assert convert(recording_tool, tmp_path, data, version) == ((1, 0), data)


@pytest.mark.parametrize("weapon", [-32768, -2, 4, 32767])
def test_invalid_slot_boundaries(recording_tool, tmp_path, weapon):
    data = stream(weapon=weapon) + event(4, 5, struct.pack("<h", weapon)) + event(4, 1)
    expected = bytearray(data);expected[4:6] = expected[-3:-1] = b"\xff\xff"
    assert convert(recording_tool, tmp_path, data) == ((1, 2), expected)


@pytest.mark.parametrize("version", [1, 2, 3, 4])
@pytest.mark.parametrize("time", [0, 1, 2, 3])
def test_selection_events_and_all_payload_sizes(recording_tool, tmp_path, version, time):
    data = stream(version, weapon=2)
    for kind in range(2, 23):
        if version == 4:
            size = 1 if kind < 4 else 2 if kind < 6 else 8 if kind == 6 else 2 if kind < 15 else 4
        else:
            size = 2 if kind < 6 else 0 if kind in (7, 8) else 8 if kind == 6 or kind >= 16 else 12
        data += event(version, kind, b"\x7f" * size, time)
    data += event(version, 1, time=time)
    result, converted = convert(recording_tool, tmp_path, data, version)
    assert result == (1, 1)
    assert len(converted) == len(data)
    assert sum(a != b for a, b in zip(converted, data)) == 2
    assert convert(recording_tool, tmp_path, converted, version) == ((1, 0), converted)


@pytest.mark.parametrize("version", [1, 2, 3, 4])
def test_every_truncation_fails_without_mutation(recording_tool, tmp_path, version):
    data = stream(version) + event(version, 5, b"\x00\xff", 3) + event(version, 6, b"\0" * 8) + event(version, 1)
    for size in range(len(data)):
        assert convert(recording_tool, tmp_path, data[:size], version) == ((0, 0), data[:size])


@pytest.mark.parametrize("version,control,tail", [(0, 4, b"\x04"), (5, 4, b"\x04"), (4, 5, b"\x04"),
                                                   (4, 4, b"\xfc"), (1, 4, b"\xff\xff\0\0")])
def test_unknown_formats_fail_without_mutation(recording_tool, tmp_path, version, control, tail):
    data = stream() + tail
    assert convert(recording_tool, tmp_path, data, version, control) == ((0, 0), data)


@pytest.mark.parametrize("mode", ["tags", "bad-block", "bad-pointer", "bad-size", "disabled"])
def test_checked_mcc_scenario_integration(recording_tool, tmp_path, mode):
    data = stream() + event(4, 1)
    result, converted = convert(recording_tool, tmp_path, data, mode=mode)
    assert result[0] == (mode in ("tags", "disabled"))
    expected = bytearray(data)
    if mode == "tags":expected[4:6] = b"\xff\xff"
    assert converted == expected
