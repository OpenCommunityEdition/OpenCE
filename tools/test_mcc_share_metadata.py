"""Share only the referenced MCC metadata, using the real readers and decoder."""
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys

import pytest
from mcc_test_support import png

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "port/linux/src"
ZLIB = ROOT / "port/third_party/zlib"

DRIVER = r'''
#include "mcc_share_metadata.h"
#include "mcc_preview_image.h"
#include "mcc_manifest.h"
#include "mcc_menu_metadata.h"
#include "server_json.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void platform_translate_path(const char *in,char *out,unsigned long size) {
 (void)in; if(size) out[0]=0;
}
static const char *test_library = "";
static int test_campaigns;
int config_boolean(const char *name) {
 return !strcmp(name,"game.mcc_campaigns") ? test_campaigns : 1;
}
const char *config_string(const char *name) {
 return !strcmp(name,"paths.steam") ? test_library : "";
}
#ifdef _WIN32
int steam_workshop_windows_roots(char (*paths)[STEAM_WORKSHOP_PATH_SIZE], int maximum) {
 (void)paths; (void)maximum; return 0;
}
#endif
int main(int argc,char **argv) {
 static struct mcc_share_metadata_plan plan;
 static struct mcc_share_metadata_entry received[MCC_SHARE_METADATA_FILES];
 unsigned char keep[MCC_SHARE_METADATA_FILES],digest[32]; char reason[128]; int i,ok,count;
 if(argc<5) return 2;
 if(!strcmp(argv[1],"scan")) {
  test_library=argv[2]; test_campaigns=atoi(argv[3]);
  printf("%d\n",steam_workshop_scan());
  for(i=0;i<steam_workshop_count();i++) printf("%s\n",steam_workshop_map(i)->name);
 } else if(!strcmp(argv[1],"manifest")) {
  static struct mcc_manifest manifest;
  ok=mcc_manifest_load(argv[2],&manifest,reason);
  printf("%d %d\n%s\n",ok,manifest.count,reason);
  for(i=0;i<manifest.count;i++) printf("%s\n",manifest.missions[i].name);
 } else if(!strcmp(argv[1],"menu")) {
  static struct mcc_menu_metadata metadata;
  ok=mcc_menu_metadata_load(argv[2],atoi(argv[3]),&metadata);
  printf("%d %d\n",ok,metadata.count);
  for(i=0;i<metadata.count;i++) printf("%s\n",metadata.maps[i].name);
 } else if(!strcmp(argv[1],"digest")) {
  ok=mcc_share_metadata_digest(argv[2],atoi(argv[3]),argv[4],digest,reason);
  printf("%d\n",ok);for(i=0;i<32;i++)printf("%02x",digest[i]);printf("\n%s\n",reason);
 } else if(!strcmp(argv[1],"plan")) {
  ok=mcc_share_metadata_plan(argv[2],atoi(argv[3]),argv[4],&plan,reason);
  printf("%d %d\n",ok,plan.count);
  if(ok) for(i=0;i<plan.count;i++) printf("%u %u %s\n",plan.files[i].kind,plan.files[i].size,plan.files[i].path);
  else printf("%s\n",reason);
 } else {
  count=argc-5;
  if(count>MCC_SHARE_METADATA_FILES) return 3;
  for(i=0;i<count;i++) if(sscanf(argv[i+5],"%u %u %127[^\n]",&received[i].kind,&received[i].size,received[i].path)!=3) return 4;
  ok=mcc_share_metadata_validate(argv[2],atoi(argv[3]),argv[4],received,count,keep,reason);
  printf("%d\n",ok);
  for(i=0;i<count;i++) printf("%u ",keep[i]);
  printf("\n%s\n",reason);
 }
 return 0;
}
'''


@pytest.fixture(scope="module")
def metadata_tool(tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    assert compiler, "C compiler required"
    work = tmp_path_factory.mktemp("mcc-share-metadata")
    (work / "driver.c").write_text(DRIVER)
    windows = sys.platform == "win32"
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if windows else ["-D_GNU_SOURCE"]
    if os.environ.get("MCC_SHARE_METADATA_SANITIZE"):
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
    exe = work / ("metadata.exe" if windows else "metadata")
    files = ROOT / "port/windows/src/win32_files.c" if windows else SRC / "posix_files.c"
    sources = [work / "driver.c", SRC / "mcc_share_metadata.c", SRC / "mcc_preview_image.c",
               SRC / "mcc_menu_metadata.c", SRC / "posix_mcc_preview.c", SRC / "mcc_manifest.c",
               SRC / "server_json.c", SRC / "steam_workshop.c",
               ROOT / "port/linux/game/mcc_sha256.c", files, *sorted(ZLIB.glob("*.c"))]
    result = subprocess.run([compiler, *flags, "-std=gnu11", "-DZ_PREFIX", "-I", str(SRC), "-I", str(ZLIB),
                             *map(str, sources), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    yield exe
    # These generated tool/fixture files belong solely to this module's run.
    shutil.rmtree(work)


def run(tool, operation, root, campaign, level="first", entries=()):
    result = subprocess.run([str(tool), operation, str(root), str(int(campaign)), level,
                             *(f"{kind} {size} {name}" for kind, size, name in entries)],
                            capture_output=True, text=True, timeout=20)
    assert result.returncode == 0, result.stderr
    return result.stdout.splitlines()


def plan(tool, root, campaign=False, level="first"):
    lines = run(tool, "plan", root, campaign, level)
    ok, count = map(int, lines[0].split())
    entries = [(int(k), int(size), name) for k, size, name in (line.split(" ", 2) for line in lines[1:])] if ok else []
    assert len(entries) == count
    return ok, entries


def validate(tool, root, entries, campaign=False, level="first"):
    lines = run(tool, "validate", root, campaign, level, entries)
    return int(lines[0]), list(map(int, lines[1].split()))


def package(root, campaign=False, count=2):
    (root / "meta").mkdir(parents=True)
    (root / "assets").mkdir()
    names = ["first", "second", *(f"mission{i}" for i in range(2, count))][:count]
    paths = [f"META/{name}.json" for name in names]
    contents = {"HasCampaign": True} if campaign else {"MultiplayerMaps": paths}
    (root / "ModInfo.json").write_text(json.dumps({"Engine": "Halo1", "GameModContents": contents}))
    if campaign:
        (root / "CampaignInfo.json").write_text(json.dumps({"CampaignMaps": paths}))
    for name in names:
        (root / f"meta/{name}.json").write_text(json.dumps({"ScenarioFile": f"levels/{name}",
            "Images": {"Thumbnail": f"assets/{name}.png"}}))
        (root / f"assets/{name}.png").write_bytes(png()[0])


def test_map_only_folder(metadata_tool, tmp_path):
    (tmp_path / "first.map").write_bytes(b"map fixture, unused by metadata reader")
    assert plan(metadata_tool, tmp_path) == (1, [])
    assert validate(metadata_tool, tmp_path, []) == (1, [])
    assert validate(metadata_tool, tmp_path, [(1, 2, "extra.json")])[0] == 0
    assert run(metadata_tool, "digest", tmp_path, -1)[:2] == ["1", "0" * 64]
    assert plan(metadata_tool, tmp_path / "missing")[0] == 0
    assert plan(metadata_tool, tmp_path / "first.map")[0] == 0


@pytest.mark.parametrize("campaign", [False, True])
def test_only_referenced_files(metadata_tool, tmp_path, campaign):
    package(tmp_path, campaign)
    (tmp_path / "arbitrary.json").write_text('{"command":"do not execute"}')
    (tmp_path / "assets/unrelated.png").write_bytes(png()[0])
    ok, entries = plan(metadata_tool, tmp_path, campaign)
    assert ok
    paths = {name.lower() for _, _, name in entries}
    assert paths == {"modinfo.json", "meta/first.json", "assets/first.png"} | (
        {"campaigninfo.json", "meta/second.json"} if campaign else set())
    assert validate(metadata_tool, tmp_path, entries, campaign) == (1, [1] * len(entries))
    for addition in [(1, 2, "arbitrary.json"), (2, 3, "assets/unrelated.png"), (1, 2, "../outside.json")]:
        assert validate(metadata_tool, tmp_path, [*entries, addition], campaign)[0] == 0


def test_required_metadata_and_optional_picture(metadata_tool, tmp_path):
    package(tmp_path, True)
    ok, entries = plan(metadata_tool, tmp_path, True)
    assert ok
    assert validate(metadata_tool, tmp_path, entries[1:], True)[0] == 0
    assert validate(metadata_tool, tmp_path, [*entries, entries[0]], True)[0] == 0
    aliased = [(k, size, name.replace("/", "\\").upper()) for k, size, name in entries]
    assert validate(metadata_tool, tmp_path, aliased, True)[0] == 1
    assert validate(metadata_tool, tmp_path, [*entries, aliased[2]], True)[0] == 0
    altered = [(2 if k == 1 else k, size, name) for k, size, name in entries]
    assert validate(metadata_tool, tmp_path, altered, True)[0] == 0
    (tmp_path / "assets/first.png").write_bytes(b"not an image")
    ok, keep = validate(metadata_tool, tmp_path, entries, True)
    assert ok and keep == [int(k != 2) for k, _, _ in entries]
    assert validate(metadata_tool, tmp_path, [e for e in entries if e[0] != 2], True)[0] == 1
    (tmp_path / "meta/second.json").write_text("{broken")
    assert plan(metadata_tool, tmp_path, True)[0] == 0
    assert validate(metadata_tool, tmp_path, entries, True)[0] == 0


@pytest.mark.parametrize("bad", ["../outside.json", "/absolute.json", "C:/file.json", "meta/NUL.json", "meta/file.json:stream", "meta/file.json."])
def test_campaign_path_refusal(metadata_tool, tmp_path, bad):
    package(tmp_path, True)
    (tmp_path / "CampaignInfo.json").write_text(json.dumps({"CampaignMaps": ["meta/first.json", bad]}))
    assert plan(metadata_tool, tmp_path, True)[0] == 0



@pytest.mark.parametrize("name", ["caf\u00e9", "NUL", "CON ", "conin$", "COM0", "LPT9"])
def test_nonportable_required_json_refused(metadata_tool, tmp_path, name):
    package(tmp_path, True)
    path = f"meta/{name}.json"
    # Unicode filenames are real files on Windows too. Device aliases must
    # never be opened there; POSIX can hold them and still must refuse sharing.
    if name == "caf\u00e9" or sys.platform != "win32":
        (tmp_path / path).write_bytes((tmp_path / "meta/second.json").read_bytes())
    (tmp_path / "CampaignInfo.json").write_text(json.dumps({"CampaignMaps": ["meta/first.json", path]}))
    assert plan(metadata_tool, tmp_path, -1)[0] == 0
    assert run(metadata_tool, "digest", tmp_path, -1)[0] == "0"


@pytest.mark.parametrize("name", ["caf\u00e9", "NUL", "CON ", "conin$", "COM0", "LPT9"])
def test_nonportable_optional_png_uses_placeholder(metadata_tool, tmp_path, name):
    package(tmp_path, True)
    path = f"assets/{name}.png"
    image = tmp_path / path
    if name == "caf\u00e9" or sys.platform != "win32":
        image.write_bytes(png()[0])
    first = tmp_path / "meta/first.json"
    data = json.loads(first.read_text())
    data["Images"]["Thumbnail"] = path
    first.write_text(json.dumps(data))
    ok, entries = plan(metadata_tool, tmp_path, -1)
    assert ok and all(kind != 2 for kind, _, _ in entries)
    assert validate(metadata_tool, tmp_path, entries, -1) == (1, [1] * len(entries))
    before = run(metadata_tool, "digest", tmp_path, -1)
    assert before[0] == "1"
    if name == "caf\u00e9" or sys.platform != "win32":
        image.unlink()
    assert run(metadata_tool, "digest", tmp_path, -1)[:2] == before[:2]


def test_campaign_bounds_and_identity(metadata_tool, tmp_path):
    package(tmp_path, True, 64)
    ok, entries = plan(metadata_tool, tmp_path, True)
    assert ok and sum(k == 1 for k, _, _ in entries) == 66
    assert sum(k == 2 for k, _, _ in entries) == 1
    (tmp_path / "CampaignInfo.json").write_text(json.dumps({"CampaignMaps": ["meta/first.json"] * 65}))
    assert plan(metadata_tool, tmp_path, True)[0] == 0
    (tmp_path / "CampaignInfo.json").write_text(json.dumps({"CampaignMaps": ["meta/first.json", "META/first.json"]}))
    assert plan(metadata_tool, tmp_path, True)[0] == 0


def test_metadata_digest_and_automatic_family(metadata_tool, tmp_path):
    package(tmp_path, True)
    def digest():
        lines = run(metadata_tool, "digest", tmp_path, -1)
        assert lines[0] == "1"
        return lines[1]
    original = digest()
    assert original != "0" * 64
    assert plan(metadata_tool, tmp_path, -1) == plan(metadata_tool, tmp_path, True)
    (tmp_path / "assets/first.png").write_bytes(b"invalid optional picture")
    (tmp_path / "workshop_id.txt").write_text("not a valid advisory identifier")
    assert digest() == original
    (tmp_path / "workshop_id.txt").unlink()
    # File location does not alter metadata identity.
    moved = tmp_path.parent / (tmp_path.name + "-copy")
    shutil.copytree(tmp_path, moved)
    assert run(metadata_tool, "digest", moved, -1)[1] == original
    data = json.loads((tmp_path / "CampaignInfo.json").read_text())
    data["CampaignMaps"].reverse()
    (tmp_path / "CampaignInfo.json").write_text(json.dumps(data))
    assert digest() != original


def test_mixed_package_selects_map_metadata(metadata_tool, tmp_path):
    package(tmp_path, True)
    (tmp_path / "ModInfo.json").write_text(json.dumps({"Engine": "Halo1", "GameModContents":
        {"HasCampaign": True, "MultiplayerMaps": ["meta/arena.json"]}}))
    (tmp_path / "meta/arena.json").write_text(json.dumps({"ScenarioFile": "levels/arena",
        "Images": {"Thumbnail": "assets/arena.png"}}))
    (tmp_path / "assets/arena.png").write_bytes(png()[0])
    ok, entries = plan(metadata_tool, tmp_path, -1, "arena")
    assert all(name==name.lower() for _, _, name in entries)
    assert ok and {name.lower() for _, _, name in entries} == {
        "modinfo.json", "campaigninfo.json", "meta/first.json", "meta/second.json", "meta/arena.json",
        "assets/arena.png"}
    # Transfer exactly the allowlist, not a copy of the original package: omitted
    # campaign JSON previously made this valid host plan impossible to receive.
    staged = tmp_path / "received"
    staged.mkdir()
    for _, size, name in entries:
        assert size
        source = tmp_path.joinpath(*name.lower().split("/"))
        if name.lower() == "modinfo.json":
            source = tmp_path / "ModInfo.json"
        elif name.lower() == "campaigninfo.json":
            source = tmp_path / "CampaignInfo.json"
        target = staged / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
    assert validate(metadata_tool, staged, entries, -1, "arena") == (1, [1] * len(entries))
    original_digest = run(metadata_tool, "digest", tmp_path, -1, "arena")
    received_digest = run(metadata_tool, "digest", staged, -1, "arena")
    assert original_digest[:2] == received_digest[:2] and original_digest[0] == "1"


@pytest.mark.parametrize("text", ["", "[]", "{", '{"Engine":"Halo2"}', '"' + "x" * (256 * 1024) + '"'],
                         ids=["empty", "array", "incomplete", "wrong-engine", "oversized"])
def test_invalid_present_modinfo(metadata_tool, tmp_path, text):
    (tmp_path / "ModInfo.json").write_text(text)
    assert plan(metadata_tool, tmp_path)[0] == 0


@pytest.mark.parametrize("text,valid", [("123456\nAuthor title\n", True), ("123456", True), ("https://example.org", False),
    ("1\nrun\nextra", False), ("1\n" + "x" * 40, False), ("1\n\0", False), ("9" * 21, False)])
def test_workshop_sidecar(metadata_tool, tmp_path, text, valid):
    (tmp_path / "workshop_id.txt").write_bytes(text.encode())
    ok, entries = plan(metadata_tool, tmp_path)
    assert ok == valid
    if valid:
        assert entries == [(3, len(text), "workshop_id.txt")]


def test_metadata_link_escape(metadata_tool, tmp_path):
    root = tmp_path / "root"
    package(root, True)
    external = tmp_path / "external.json"
    external.write_text((root / "meta/first.json").read_text())
    (root / "meta/first.json").unlink()
    try:
        (root / "meta/first.json").symlink_to(external)
    except OSError:
        pytest.skip("This host does not permit file symlinks")
    try:
        assert plan(metadata_tool, root, True)[0] == 0
    finally:
        (root / "meta/first.json").unlink()


@pytest.mark.skipif(sys.platform != "win32", reason="Windows junction containment")
def test_metadata_junction_escape(metadata_tool, tmp_path):
    root = tmp_path / "root"
    package(root, True)
    external = tmp_path / "external"
    assert external.resolve().is_relative_to(tmp_path.resolve())
    assert (root / "meta").resolve().is_relative_to(tmp_path.resolve())
    (root / "meta").rename(external)
    result = subprocess.run(["cmd", "/c", "mklink", "/J", str(root / "meta"), str(external)],
                            capture_output=True, text=True)
    if result.returncode:
        pytest.skip("This host does not permit directory junctions: " + result.stderr)
    try:
        assert plan(metadata_tool, root, True)[0] == 0
    finally:
        os.rmdir(root / "meta")


def test_bounded_json_and_png_mutations(metadata_tool, tmp_path):
    """Deterministic malformed corpus through the actual readers, not copied parsers."""
    package(tmp_path, True)
    good_json = (tmp_path / "meta/first.json").read_bytes()
    good_png = (tmp_path / "assets/first.png").read_bytes()
    rng = random.Random(0x4D4343)
    for target, good in [(tmp_path / "meta/first.json", good_json), (tmp_path / "assets/first.png", good_png)]:
        for _ in range(40):
            mutated = bytearray(good)
            for _ in range(rng.randrange(1, 5)):
                mutated[rng.randrange(len(mutated))] = rng.randrange(256)
            target.write_bytes(mutated)
            # A mutation may still be valid; the reader must finish without a fault.
            plan(metadata_tool, tmp_path, True)
        target.write_bytes(good)


@pytest.mark.parametrize("kind", ["Default", "default", "DEFAULT", "dEfAuLt", None])
def test_campaign_kind_across_native_metadata_paths(metadata_tool, tmp_path, monkeypatch, kind):
    # A later mission must not hide the whole campaign due to capitalization.
    monkeypatch.setenv("HOME", str(tmp_path / "no-home"))
    monkeypatch.delenv("XDG_DATA_HOME", raising=False)
    monkeypatch.delenv("STEAM_COMPAT_CLIENT_INSTALL_PATH", raising=False)
    root = tmp_path / "steamapps/workshop/content/976730/111"
    package(root, True)
    (root / "maps").mkdir()
    for name in ("first", "second"):
        (root / "maps" / (name + ".map")).write_bytes(b"catalog fixture, not playable")
    second = root / "meta/second.json"
    data = json.loads(second.read_text())
    if kind is not None:
        data["CampaignMapKind"] = kind
    second.write_text(json.dumps(data))
    original = second.read_bytes()
    assert run(metadata_tool, "manifest", root, True)[0] == "1 2"
    assert run(metadata_tool, "menu", root, True) == ["1 2", "first", "second"]
    assert run(metadata_tool, "scan", tmp_path, True) == ["2", "first", "second"]
    assert run(metadata_tool, "scan", tmp_path, False) == ["0"]
    ok, entries = plan(metadata_tool, root, True)
    assert ok and validate(metadata_tool, root, entries, True)[0] == 1
    before = run(metadata_tool, "digest", root, True)
    assert before[0] == "1" and second.read_bytes() == original
    data["CampaignMapKind"] = "DeFaUlT"
    second.write_text(json.dumps(data))
    after = run(metadata_tool, "digest", root, True)
    assert after[0] == "1" and after[1] != before[1]  # identity still hashes original JSON bytes


@pytest.mark.parametrize("kind", ["Hub", "hub", "Flashback", "Cutscene", "", " Default", "Default ",
                                   "Default/", "defau\u0131t", 0, True, None, [], {}])
def test_unsupported_campaign_kind_stays_refused(metadata_tool, tmp_path, kind):
    root = tmp_path / "steamapps/workshop/content/976730/111"
    package(root, True)
    (root / "maps").mkdir()
    second = root / "meta/second.json"
    data = json.loads(second.read_text()); data["CampaignMapKind"] = kind
    second.write_text(json.dumps(data))
    assert run(metadata_tool, "manifest", root, True)[0] == "-1 0"
    assert run(metadata_tool, "menu", root, True) == ["1 1", "first"]
    assert plan(metadata_tool, root, True)[0] == 0
    result = subprocess.run([str(metadata_tool), "scan", str(tmp_path), "1", "first"],
                            capture_output=True, text=True, timeout=20)
    assert result.returncode == 0 and result.stdout.splitlines() == ["0"]
    assert "111" in result.stderr and "Unsupported campaign mission kind" in result.stderr
