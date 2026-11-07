"""Actual MCC package reader/PNG decoder, with generated (non-game) assets."""
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import zlib

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "port/linux/src"
ZLIB = ROOT / "port/third_party/zlib"


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def png(width=3, height=2, channels=4, filter_type=0, *, extra=b"", raw=None):
    pixels = bytes((i * 37 + 11) % 256 for i in range(width * height * channels))
    stride = width * channels
    def paeth(a, b, c):
        p = a + b - c
        return min((a, b, c), key=lambda v: abs(p - v))
    rows = bytearray()
    for y in range(height):
        rows.append(filter_type)
        for x in range(stride):
            a = pixels[y * stride + x - channels] if x >= channels else 0
            b = pixels[(y - 1) * stride + x] if y else 0
            c = pixels[(y - 1) * stride + x - channels] if y and x >= channels else 0
            predictor = (0, a, b, (a + b) // 2, paeth(a, b, c))[filter_type] if filter_type < 5 else 0
            rows.append((pixels[y * stride + x] - predictor) % 256)
    header = struct.pack(">IIBBBBB", width, height, 8, 6 if channels == 4 else 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + extra + chunk(b"IDAT", zlib.compress(rows if raw is None else raw)) + chunk(b"IEND", b""), pixels


DRIVER = r'''
#include "mcc_preview_image.h"
#include "mcc_manifest.h"
#include "mcc_menu_metadata.h"
#include "posix.h"
#include "server_json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void platform_translate_path(const char *in,char *out,unsigned long size) {
 (void)in; snprintf(out,size,"%s",getenv("TEST_MAP_FILE"));
}
int steam_workshop_package_folder(const char *id,char *out,unsigned long size) {
 const char *p=getenv("TEST_STEAM_FOLDER"); (void)id;
 if(!p || !*p) return 0; snprintf(out,size,"%s",p); return 1;
}
static void report(int ok,struct mcc_preview_image *image) {
 unsigned i; uint32_t hash=2166136261u;
 for(i=0;i<image->width*image->height;i++) {hash^=image->pixels[i];hash*=16777619u;}
 printf("%d %u %u %u\n",ok,image->width,image->height,hash);free(image->pixels);
}
int main(int argc,char **argv) {
 struct mcc_preview_image image; unsigned char *data; FILE *file; unsigned length; int ok,w,h;
 if(argc<3) return 2;
 if(!strcmp(argv[1],"labels")) {
  static struct mcc_menu_metadata metadata;const struct steam_workshop_map_text *map;
  if(!mcc_menu_metadata_load(argv[2],0,&metadata))return 4;
  map=mcc_menu_metadata_find(&metadata,"level");if(!map)return 5;
  printf("%s|%s\n",metadata.title,map->title);return 0;
 }
 if(!strcmp(argv[1],"load")) {ok=mcc_preview_image_load(argv[2],atoi(argv[3]),&image);report(ok,&image);return 0;}
 if(!strcmp(argv[1],"read")) {data=malloc(1024);printf("%d\n",posix_mcc_preview_read(argv[2],argv[3],data,1024));free(data);return 0;}
 if(!strcmp(argv[1],"fit")) {ok=mcc_preview_fit(atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),&w,&h);printf("%d %d %d\n",ok,ok?w:0,ok?h:0);return 0;}
 data=malloc(MCC_PREVIEW_FILE_LIMIT+1); file=fopen(argv[2],"rb"); if(!file) return 3;
 length=(unsigned)fread(data,1,MCC_PREVIEW_FILE_LIMIT+1,file);fclose(file);
 ok=mcc_preview_png(data,length,&image);report(ok,&image);free(data);return 0;
}
'''


@pytest.fixture(scope="session")
def preview_tool(tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("C compiler required")
    work = tmp_path_factory.mktemp("mcc-previews")
    steam = (SRC / "steam_workshop.c").read_text()
    parser = '\n'.join(function(steam, name) for name in (
        "copy_text", "parse", "path_is_plain", "slashes", "neutral_title", "steam_workshop_read_item", "steam_workshop_read_map"))
    (work / "driver.c").write_text(DRIVER + '\n' + parser)
    windows = sys.platform == "win32"
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if windows else ["-D_GNU_SOURCE"]
    if os.environ.get("MCC_PREVIEW_SANITIZE"):
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    exe = work / ("preview.exe" if windows else "preview")
    files = ROOT / "port/windows/src/win32_files.c" if windows else SRC / "posix_files.c"
    sources = [work / "driver.c", SRC / "mcc_preview_image.c", SRC / "mcc_menu_metadata.c", SRC / "posix_mcc_preview.c",
               SRC / "mcc_manifest.c", SRC / "server_json.c", files, *sorted(ZLIB.glob("*.c"))]
    result = subprocess.run([compiler, *flags, "-std=gnu11", "-DZ_PREFIX", "-I", str(SRC), "-I", str(ZLIB),
                             *map(str, sources), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


def run(tool, *args, **env):
    result = subprocess.run([str(tool), *map(str, args)], env=dict(os.environ, **env), capture_output=True, text=True, timeout=15)
    assert result.returncode == 0, result.stderr
    return tuple(map(int, result.stdout.split()))


@pytest.mark.parametrize("channels", [3, 4])
@pytest.mark.parametrize("filter_type", range(5))
def test_png_pixels(preview_tool, tmp_path, channels, filter_type):
    data, pixels = png(channels=channels, filter_type=filter_type)
    path = tmp_path / "image.png"; path.write_bytes(data)
    expected = 2166136261
    for i in range(0, len(pixels), channels):
        r, g, b = pixels[i:i+3]; a = pixels[i+3] if channels == 4 else 255
        expected = ((expected ^ (a << 24 | r << 16 | g << 8 | b)) * 16777619) & 0xffffffff
    assert run(preview_tool, "png", path) == (1, 3, 2, expected)


def test_invalid_png(preview_tool, tmp_path):
    data, _ = png()
    header = lambda w,h,d,c,i: chunk(b"IHDR", struct.pack(">IIBBBBB",w,h,d,c,0,0,i))
    cases = [b"", b"<svg/>", data + b"trailing", data[:-1], data[:40],
             data[:16]+b"\xff\xff\xff\xff"+data[20:], png(filter_type=5)[0],
             png(raw=b"\0")[0], png(raw=b"\0"*100)[0],
             png(extra=chunk(b"EVIL", b""))[0], png(extra=chunk(b"tRNS",b"\0"*6))[0],
             png(extra=chunk(b"texT",b"bad reserved bit"))[0], png(extra=chunk(b"1234",b"bad name"))[0],
             png(extra=chunk(b"PLTE",b"\0"*3)*2)[0]]
    for w,h,d,c,i in [(0,1,8,6,0),(2049,1,8,6,0),(1,2049,8,6,0),(1,1,16,6,0),(1,1,8,3,0),(1,1,8,6,1)]:
        cases.append(data[:8]+header(w,h,d,c,i)+data[33:])
    # Valid CRCs around truncated/extra zlib streams and illegal chunk order.
    compressed = zlib.compress(b"\0"*26)
    cases += [data[:33]+chunk(b"IDAT",compressed[:-1])+chunk(b"IEND",b""),
              data[:33]+chunk(b"IDAT",compressed+b"extra")+chunk(b"IEND",b""),
              data[:33]+chunk(b"IDAT",compressed[:3])+chunk(b"tEXt",b"x\0y")+chunk(b"IDAT",compressed[3:])+chunk(b"IEND",b""),
              data[:33]+data[8:33]+data[33:]]
    path=tmp_path/"bad.png"
    for case in cases:
        path.write_bytes(case)
        assert run(preview_tool,"png",path)[:3] == (0,0,0)
    path.write_bytes(b"\0" * (8*1024*1024+1))
    assert run(preview_tool,"png",path)[0] == 0


def package(root, *, campaign=False, large="assets/Large.PNG", thumb="thumb.png"):
    (root/"meta").mkdir(parents=True)
    (root/"assets").mkdir()
    content = {"HasCampaign": True} if campaign else {"MultiplayerMaps": ["META/level.json"]}
    (root/"ModInfo.json").write_text(json.dumps({"Engine":"Halo1","Title":{"Neutral":"Offline package"},"GameModContents":content}))
    (root/"CampaignInfo.json").write_text(json.dumps({"CampaignMaps":["META/level.json"]}))
    (root/"meta/level.json").write_text(json.dumps({"ScenarioFile":"levels/test/level", "Title":{"Neutral":"Authored level"}, "Images":{"Large":large,"Thumbnail":thumb}}))
    (root/"assets/large.png").write_bytes(png(6,3)[0])
    (root/"thumb.png").write_bytes(png(3,2)[0])


@pytest.mark.parametrize("campaign", [False, True])
def test_metadata_and_fallback(preview_tool, tmp_path, campaign):
    package(tmp_path,campaign=campaign)
    def load():
        return run(preview_tool,"load",r"mcc_maps\package\level",int(campaign), TEST_MAP_FILE=str(tmp_path/"level.map"))
    assert load()[:3] == (1,6,3)
    (tmp_path/"assets/large.png").write_bytes(b"broken")
    assert load()[:3] == (1,3,2)
    (tmp_path/"thumb.png").unlink()
    assert load()[:3] == (0,0,0)


def test_package_identity_and_missing_metadata(preview_tool, tmp_path):
    local=tmp_path/"local"; steam=tmp_path/"steam"
    package(local); package(steam)
    (steam/"maps").mkdir()
    assert run(preview_tool,"load",r"mcc_maps\12345\level",0, TEST_MAP_FILE=str(steam/"maps/level.map"), TEST_STEAM_FOLDER=str(steam/"maps"))[:3] == (1,6,3)
    (local/"ModInfo.json").unlink()
    assert run(preview_tool,"load",r"mcc_maps\12345\level",0, TEST_MAP_FILE=str(local/"level.map"), TEST_STEAM_FOLDER=str(steam/"maps"))[0] == 0
    assert run(preview_tool,"load",r"custom_maps\level",0, TEST_MAP_FILE=str(steam/"maps/level.map"))[0] == 0


def test_authored_offline_labels(preview_tool,tmp_path):
    package(tmp_path)
    assert subprocess.check_output([str(preview_tool),"labels",str(tmp_path)],text=True).strip()=="Offline package|Authored level"


@pytest.mark.parametrize("count", [75, 1024])
def test_large_package_preserves_late_labels_and_preview(preview_tool, tmp_path, count):
    package(tmp_path)
    names = []
    for index in range(count - 1):
        name = f"meta/other{index}.json"
        (tmp_path / name).write_text(json.dumps({"ScenarioFile": f"other{index}"}))
        names.append(name)
    names.append("meta/level.json")
    mod = tmp_path / "ModInfo.json"
    data = json.loads(mod.read_text())
    data["GameModContents"]["MultiplayerMaps"] = names
    mod.write_text(json.dumps(data))
    assert subprocess.check_output([str(preview_tool), "labels", str(tmp_path)], text=True).strip() == "Offline package|Authored level"
    assert run(preview_tool, "load", r"mcc_maps\package\level", 0,
               TEST_MAP_FILE=str(tmp_path / "level.map"))[:3] == (1, 6, 3)


def test_ambiguous_or_wrong_metadata(preview_tool,tmp_path):
    package(tmp_path)
    mod=tmp_path/"ModInfo.json"
    original=json.loads(mod.read_text())
    def load(campaign=0):
        return run(preview_tool,"load",r"mcc_maps\package\level",campaign, TEST_MAP_FILE=str(tmp_path/"level.map"))[0]
    mod.write_text(json.dumps(dict(original,Engine="Halo2")))
    assert load()==0
    for paths in (["meta/level.json"]*2, ["meta/level.json"]*65, ["unlisted.json"]):
        mod.write_text(json.dumps(dict(original,GameModContents={"MultiplayerMaps":paths})))
        assert load()==0
    mod.write_text(json.dumps(dict(original,GameModContents={"HasCampaign":True})))
    mission=tmp_path/"meta/level.json"; metadata=json.loads(mission.read_text())
    for kind in ("Cinematic", None, 1):
        mission.write_text(json.dumps(dict(metadata,CampaignMapKind=kind)))
        assert load(1)==0


@pytest.mark.parametrize("path", ["../outside.png", "/outside.png", "C:/outside.png", "assets/../thumb.png",
    "assets//large.png", "assets/./large.png", "thumb.png:stream", "NUL.png", "assets/", "thumb.png.", "thumb.png "])
def test_path_refusal(preview_tool,tmp_path,path):
    package(tmp_path,large=path,thumb="")
    assert run(preview_tool,"load",r"mcc_maps\package\level",0, TEST_MAP_FILE=str(tmp_path/"level.map"))[0] == 0


def test_link_escape(preview_tool,tmp_path):
    root=tmp_path/"root"; package(root,large="link/large.png",thumb="")
    outside=tmp_path/"outside"; outside.mkdir(); (outside/"large.png").write_bytes(png()[0])
    try:
        (root/"link").symlink_to(outside,target_is_directory=True)
    except OSError:
        if sys.platform != "win32": raise
        result = subprocess.run(["cmd","/c","mklink","/J",str(root/"link"),str(outside)],capture_output=True,text=True)
        assert result.returncode == 0, result.stdout + result.stderr
    try:
        assert run(preview_tool,"load",r"mcc_maps\package\level",0, TEST_MAP_FILE=str(root/"level.map"))[0] == 0
        # The same containment applies to a map JSON, not just its image.
        (outside/"map.json").write_text(json.dumps({"ScenarioFile":"level","Images":{"Large":"assets/large.png"}}))
        (root/"ModInfo.json").write_text(json.dumps({"Engine":"Halo1","GameModContents":{"MultiplayerMaps":["link/map.json"]}}))
        assert run(preview_tool,"load",r"mcc_maps\package\level",0, TEST_MAP_FILE=str(root/"level.map"))[0] == 0
    finally:
        if sys.platform == "win32" and not (root/"link").is_symlink(): os.rmdir(root/"link")
        else: (root/"link").unlink()


@pytest.mark.skipif(sys.platform == "win32", reason="POSIX pipes and case-sensitive aliases")
def test_nonregular_and_ambiguous(preview_tool,tmp_path):
    os.mkfifo(tmp_path/"pipe")
    assert run(preview_tool,"read",tmp_path,"pipe") == (0,)
    (tmp_path/"one").write_text("a"); (tmp_path/"ONE").write_text("b")
    assert run(preview_tool,"read",tmp_path,"one") == (0,)
    (tmp_path/"link").symlink_to(tmp_path/"one")
    assert run(preview_tool,"read",tmp_path,"link") == (0,)


def test_file_capacity(preview_tool,tmp_path):
    path=tmp_path/"bounded"
    for size,expected in ((0,0),(1024,1024),(1025,0)):
        path.write_bytes(b"x"*size)
        assert run(preview_tool,"read",tmp_path,"bounded")== (expected,)
    (tmp_path/"directory").mkdir()
    assert run(preview_tool,"read",tmp_path,"directory")== (0,)


@pytest.mark.parametrize("shape,expected", [((668,292),(140,61)),((316,160),(140,70)),((100,200),(57,114)),((1,2048),(1,114)),((2048,1),(140,1))])
def test_fit(preview_tool,shape,expected):
    assert run(preview_tool,"fit",*shape,140,114) == (1,*expected)


def test_bad_fit(preview_tool):
    for values in [(0,1,140,114),(1,0,140,114),(2049,1,140,114),(1,1,0,114),(1,1,140,-1),(1,1,40000,114)]:
        assert run(preview_tool,"fit",*values)[0] == 0
