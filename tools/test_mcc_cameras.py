"""Exercise MCC camera admission using the real metadata normalizer."""
from pathlib import Path
import math
import shutil
import struct
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def camera_tool(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is required")
    work = tmp_path_factory.mktemp("mcc-cameras")
    (work / "cseries.h").write_text("#define TRUE 1\n#define FALSE 0\n#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))\n")
    (work / "errors.h").write_text("#define _error_silent 0\nvoid error(int,const char *,...);\n")
    source = work / "test.c"
    source.write_text(r'''
#include "mcc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int mcc_tags_prepare(struct mcc_runtime *);
void error(int level,char const *format,...) {(void)level;(void)format;}
int mcc_recording_prepare(unsigned char *p,uint32_t s,unsigned char v,unsigned char c,uint32_t *n) {
    (void)p;(void)s;(void)v;(void)c;(void)n;abort();return 0;
}
void *mcc_runtime_pointer(struct mcc_runtime *r,uint32_t address,uint32_t size) {
    uint32_t offset=address-r->report.tag_base;
    return address>=r->report.tag_base && offset<=r->used && size<=r->used-offset ? r->tags+offset : NULL;
}
int main(int argc,char **argv) {
    unsigned char data[4096];struct mcc_runtime r={0};FILE *f;int ok;
    if(argc!=3)return 2;
    f=fopen(argv[1],"rb");if(!f)return 3;
    r.used=(uint32_t)fread(data,1,sizeof(data),f);fclose(f);
    r.tags=r.tag_index=data;r.report.tag_base=0x50000000;r.report.tag_count=1;
    ok=mcc_tags_prepare(&r);
    f=fopen(argv[2],"wb");if(!f)return 4;fwrite(data,1,r.used,f);fclose(f);
    printf("%d\n",ok);return 0;
}
''')
    binary = work / ("camera.exe" if sys.platform == "win32" else "camera")
    game = ROOT / "port/linux/game"
    command = [compiler, "-std=gnu89", "-Wall", "-Wextra", "-Werror", "-Wno-multichar",
               "-I" + str(work), "-I" + str(game), str(source), str(game / "mcc_tags.c"), "-o", str(binary)]
    if sys.platform == "win32":
        command[1:1] = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return binary


def fixture(angles):
    data = bytearray(0x600 + len(angles) * 0x68)
    struct.pack_into("<I", data, 0, int.from_bytes(b"scnr", "big"))
    struct.pack_into("<I", data, 20, 0x50000020)
    struct.pack_into("<II", data, 0x20 + 0x4F0, len(angles), 0x50000600)
    for i, angle in enumerate(angles):
        point = 0x600 + i * 0x68
        data[point:point + 0x68] = bytes(range(0x68))
        struct.pack_into("<f", data, point + 0x40, angle)
    return data


def convert(tool, tmp, data):
    src, dst = tmp / "in.bin", tmp / "out.bin"
    src.write_bytes(data)
    result = subprocess.run([str(tool), str(src), str(dst)], capture_output=True, text=True, check=True)
    return int(result.stdout), dst.read_bytes()


def test_wide_mcc_lenses_normalized_without_changing_other_camera_fields(camera_tool, tmp_path):
    angles = [0, 0.001, math.radians(50), math.radians(70), math.pi / 2,
              math.radians(100), math.radians(179)]
    data = fixture(angles)
    expected = bytearray(data)
    for i in (5, 6):
        struct.pack_into("<f", expected, 0x600 + i * 0x68 + 0x40, math.pi / 2)
    assert convert(camera_tool, tmp_path, data) == (1, expected)
    assert convert(camera_tool, tmp_path, expected) == (1, expected)


@pytest.mark.parametrize("angle", [-1, 0.0001, math.pi, 4, math.inf, -math.inf, math.nan])
def test_invalid_lenses_refused(camera_tool, tmp_path, angle):
    data = fixture([angle])
    assert convert(camera_tool, tmp_path, data) == (0, data)


@pytest.mark.parametrize("kind", ["count", "address", "truncated", "empty"])
def test_camera_block_is_bounds_checked(camera_tool, tmp_path, kind):
    data = fixture([math.radians(100)])
    if kind == "count":
        struct.pack_into("<I", data, 0x20 + 0x4F0, 0xFFFFFFFF)
    elif kind == "address":
        struct.pack_into("<I", data, 0x20 + 0x4F4, 0xFFFFFFFF)
    elif kind == "truncated":
        data = data[:-1]
    else:
        struct.pack_into("<II", data, 0x20 + 0x4F0, 0, 0)
    assert convert(camera_tool, tmp_path, data) == (int(kind == "empty"), data)
