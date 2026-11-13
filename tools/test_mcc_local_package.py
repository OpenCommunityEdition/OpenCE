"""Local MCC layouts use the native resolver, metadata and PNG readers."""
from pathlib import Path
import json
import shutil
import subprocess
import sys
import pytest
from harness import function
from mcc_test_support import png

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "port/linux/src"


@pytest.fixture(scope="module")
def local_tool(tmp_path_factory):
    cc = shutil.which("clang")
    if not cc:
        pytest.skip("clang required")
    work = tmp_path_factory.mktemp("local-mcc")
    driver = r'''
#include "posix.h"
#include "steam_workshop.h"
#include "mcc_local_package.h"
#include "mcc_manifest.h"
#include "mcc_menu_metadata.h"
#include "mcc_preview_image.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int BOOL;
#define TRUE 1
#define FALSE 0
static const char *data_root;
static const char *platform_data_root(void) {return data_root;}
static const char *platform_save_root(void) {return data_root;}
static const char *platform_custom_edition_root(void) {return "";}
static void make_directories(const char *p) {(void)p;}
int config_boolean(const char *p) {(void)p;return 0;}
const char *config_string(const char *p) {(void)p;return "";}
#ifdef _WIN32
int steam_workshop_windows_roots(char paths[][STEAM_WORKSHOP_PATH_SIZE],int n) {(void)paths;(void)n;return 0;}
#endif
'''
    native = (SRC / "xbox_files.c").read_text()
    driver += "\n".join(function(native, name) for name in
                         ("trim_separators", "names_match", "translate_workshop", "translate_elsewhere", "platform_translate_path"))
    driver += r'''
int main(int argc,char **argv) {
    char path[1024],reason[128];struct mcc_preview_image image;
    static struct mcc_menu_metadata metadata;static struct mcc_manifest campaign;
    const struct steam_workshop_map_text *map;
    if(argc!=3)return 2;data_root=argv[1];
    platform_translate_path(argv[2],path,sizeof(path));printf("%s\n",path);
    if(!mcc_manifest_package_root("mcc_maps\\35530909\\arena",path))return 3;
    if(mcc_menu_metadata_load(path,0,&metadata) && (map=mcc_menu_metadata_find(&metadata,"arena")))
        printf("%s|%s\n",metadata.title,map->title);
    if(mcc_preview_image_load("mcc_maps\\35530909\\arena",0,&image)) {
        printf("%u|%u\n",image.width,image.height);free(image.pixels);
    }
    if(mcc_manifest_for_level("mcc_maps\\35530909\\arena",&campaign,reason)==1)
        printf("%s|%s\n",campaign.title,campaign.missions[0].name);
    return 0;
}
'''
    (work / "driver.c").write_text(driver)
    windows = sys.platform == "win32"
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if windows else ["-D_GNU_SOURCE"]
    files = ROOT / "port/windows/src/win32_files.c" if windows else SRC / "posix_files.c"
    zlib = ROOT / "port/third_party/zlib"
    sources = [work / "driver.c", files, *[SRC / (name + ".c") for name in
        ("steam_workshop", "mcc_local_package", "mcc_manifest", "mcc_menu_metadata", "mcc_preview_image",
         "posix_mcc_preview", "posix_mcc_share", "server_json")], *sorted(zlib.glob("*.c"))]
    exe = work / ("local.exe" if windows else "local")
    result = subprocess.run([cc, *flags, "-std=gnu11", "-DZ_PREFIX", "-I", str(SRC), "-I", str(zlib),
                             *map(str, sources), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


@pytest.mark.parametrize("nested", [False, True])
def test_local_layout_metadata_resources_and_legacy_paths(local_tool, tmp_path, nested):
    package = tmp_path / "mcc_maps/35530909"
    maps = package / "Maps" if nested else package
    maps.mkdir(parents=True)
    (maps / "arena.map").write_bytes(b"map identity")
    (package / "ModInfo.json").write_text(json.dumps({"Engine": "Halo1", "Title": {"Neutral": "My Mod"},
        "GameModContents": {"HasCampaign": True, "MultiplayerMaps": ["arena.json"]}}))
    (package / "arena.json").write_text(json.dumps({"MapFileName": "arena", "Title": {"Neutral": "My Arena"},
        "Images": {"Thumbnail": "thumb.png"}, "ScenarioFile": "levels/arena/arena", "CampaignMapKind": "Default"}))
    (package / "CampaignInfo.json").write_text(json.dumps({"Title": {"Neutral": "My Campaign"}, "CampaignMaps": ["arena.json"]}))
    (package / "thumb.png").write_bytes(png()[0])
    def run(request):
        result = subprocess.run([str(local_tool), str(tmp_path), request], capture_output=True, text=True, check=True)
        return result.stdout.splitlines()
    result = run("mcc_maps/35530909/arena.map")
    assert Path(result[0]) == maps / "arena.map"
    assert result[1:] == ["My Mod|My Arena", "3|2", "My Campaign|arena"]
    # A directly installed file wins over the nested copy, even if it is empty.
    (package / "arena.map").write_bytes(b"")
    assert Path(run("mcc_maps/35530909/arena.map")[0]) == package / "arena.map"
    resource = package / "halo1/maps"
    resource.mkdir(parents=True)
    (resource / "sounds.map").write_bytes(b"package sounds")
    assert Path(run("mcc_maps/35530909/sounds.map")[0]) == resource / "sounds.map"
    for family in ("maps", "custom_maps"):
        assert Path(run(family + "/arena.map")[0]) == tmp_path / family / "arena.map"
    assert (package / "ModInfo.json").exists() and (maps / "arena.map").exists()
