"""The Steam Workshop catalog (port/linux/src/steam_workshop.c): the item and map json, the library list, and a scan
of a made-up Steam folder through the real directory code."""
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "port/linux/src"

DRIVER = r'''
#include "steam_workshop.h"
#include "mcc_manifest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
int config_boolean(const char *name) {
 if(!strcmp(name,"game.mcc_campaigns")) return getenv("TEST_MCC_CAMPAIGNS") != NULL;
 const char *value=getenv("TEST_WORKSHOP_ENABLED");(void)name;
 return !value || strcmp(value,"0");
}
const char *config_string(const char *name)
{
	const char *value = !strcmp(name, "paths.steam") ? getenv("TEST_STEAM_SETTING") : NULL;
	return value ? value : "";
}
static char *slurp(const char *path, int *length)
{
	FILE *file = fopen(path, "rb");
	char *text = malloc(1 << 20);

	if (!file) return NULL;
	*length = (int)fread(text, 1, (1 << 20) - 1, file);
	text[*length] = 0;
	fclose(file);
	return text;
}
int main(int argc, char **argv)
{
	int length, index;
	char *text;

	if (argc == 3 && !strcmp(argv[1], "item"))
	{
		static struct steam_workshop_item_text item;

		text = slurp(argv[2], &length);
		printf("ok=%d\n", steam_workshop_read_item(text, length, &item));
		printf("title=%s\nhalo1=%d\nmaps=%d\n", item.title, item.halo1, item.map_count);
		for (index = 0; index < item.map_count; index++)
			printf("json=%s\n", item.map_json[index]);
	}
	else if (argc == 3 && !strcmp(argv[1], "map"))
	{
		struct steam_workshop_map_text map;

		text = slurp(argv[2], &length);
		printf("ok=%d\nname=%s\ntitle=%s\nthumbnail=%s\n", steam_workshop_read_map(text, length, &map), map.name,
			map.title, map.thumbnail);
	}
	else if (argc == 3 && !strcmp(argv[1], "vdf"))
	{
		static char paths[8][STEAM_WORKSHOP_PATH_SIZE];
		int count;

		text = slurp(argv[2], &length);
		count = steam_workshop_read_libraries(text, length, paths, 8);
		printf("count=%d\n", count);
		for (index = 0; index < count; index++)
			printf("path=%s\n", paths[index]);
	}
	else if (argc == 2 && !strcmp(argv[1], "scan"))
	{
		char folder[512] = {0}, title[64] = {0};

		printf("count=%d\n", steam_workshop_scan());
		for (index = 0; index < steam_workshop_count(); index++)
		{
			const struct steam_workshop_map *map = steam_workshop_map(index);

			printf("map=%s|%s|%s|%s|%s|%s\n", map->item_id, map->item_title, map->name, map->title, map->path,
				map->preview);
		}
		printf("again=%d\n", steam_workshop_count());
		printf("folder=%d %s\n", steam_workshop_package_folder("111", folder, sizeof(folder)), folder);
		printf("none=%d\n", steam_workshop_package_folder("999", folder, sizeof(folder)));
		printf("title=%d %s\n", steam_workshop_item_title("222", title, sizeof(title)), title);
		printf("small=%d\n", steam_workshop_package_folder("111", folder, 8));
	}
	else if (argc == 3 && !strcmp(argv[1], "translate"))
	{
		char path[1024];
		platform_translate_path(argv[2], path, sizeof(path));
		printf("path=%s\n", path);
	}
	else if (argc == 3 && (!strcmp(argv[1], "manifest") || !strcmp(argv[1], "manifest-level")))
	{
		static struct mcc_manifest campaign;
		char reason[128];
		int result = !strcmp(argv[1], "manifest") ? mcc_manifest_load(argv[2], &campaign, reason) :
			mcc_manifest_for_level(argv[2], &campaign, reason);
		printf("result=%d\nreason=%s\ncount=%d\ntitle=%s\n", result, reason, campaign.count, campaign.title);
		for(index=0;index<campaign.count;index++) printf("mission=%s|%s\n",campaign.missions[index].name,campaign.missions[index].title);
	}
	return 0;
}
'''


WINDOWS_REGISTRY = r'''
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static LSTATUS WINAPI test_registry(HKEY key, LPCSTR subkey, LPCSTR value, DWORD flags,
                                   LPDWORD type, PVOID data, LPDWORD size)
{
    const char *path;
    DWORD needed;
    assert(!getenv("TEST_REGISTRY_MUST_NOT_READ"));
    assert(!strcmp(subkey, "Software\\Valve\\Steam"));
    assert(type == NULL && (flags & RRF_RT_REG_SZ));
    if (key == HKEY_CURRENT_USER) {
        assert(!strcmp(value, "SteamPath") && flags == RRF_RT_REG_SZ);
        path = getenv("TEST_STEAM_REG_USER");
    } else {
        assert(key == HKEY_LOCAL_MACHINE && !strcmp(value, "InstallPath"));
        assert(flags == (RRF_RT_REG_SZ | RRF_SUBKEY_WOW6432KEY) ||
               flags == (RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY));
        path = getenv(flags & RRF_SUBKEY_WOW6432KEY ? "TEST_STEAM_REG_32" : "TEST_STEAM_REG_64");
    }
    if (!path) return ERROR_FILE_NOT_FOUND;
    if (!strcmp(path, "!denied")) return ERROR_ACCESS_DENIED;
    if (!strcmp(path, "!type")) return ERROR_UNSUPPORTED_TYPE;
    needed = (DWORD)strlen(path) + 1;
    if (needed > *size) return ERROR_MORE_DATA;
    memcpy(data, path, needed);
    *size = needed;
    return ERROR_SUCCESS;
}
#define RegGetValueA test_registry
'''


def build_tool(work, *, mock_registry=True):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("gcc")
    if not compiler:
        pytest.skip("a C compiler is needed")
    native = (SRC / "xbox_files.c").read_text()
    translation = r'''
#include "posix.h"
typedef int BOOL;
#define TRUE 1
#define FALSE 0
static char const *platform_data_root(void) {return getenv("TEST_DATA_ROOT");}
static char const *platform_save_root(void) {return platform_data_root();}
static char const *platform_custom_edition_root(void) {return "";}
static void make_directories(char const *path) {(void)path;}
'''
    for name in ("trim_separators", "names_match", "translate_workshop", "translate_elsewhere", "platform_translate_path"):
        translation += function(native, name) + "\n"
    (work / "driver.c").write_text(DRIVER.replace("int main(", translation + "\nint main("))
    exe = work / ("driver.exe" if sys.platform == "win32" else "driver")
    windows = sys.platform == "win32"
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if windows else []
    registry = ROOT / "port/windows/src/win32_steam_workshop.c"
    if windows and mock_registry:
        # Mock only the registry API: production discovery and filesystem code
        # still run, without scanning the developer's installed Steam packages.
        wrapper = work / "registry.c"
        wrapper.write_text(WINDOWS_REGISTRY + '\n#include "%s"\n' % registry.as_posix())
        registry = wrapper
    files = ROOT / "port/windows/src/win32_files.c" if windows else SRC / "posix_files.c"
    result = subprocess.run([compiler, *flags, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-I", str(SRC), str(work / "driver.c"),
                             str(SRC / "steam_workshop.c"), str(files), str(SRC / "server_json.c"),
                             str(SRC / "mcc_manifest.c"),
                             *([str(registry)] if windows else []),
                             *(["-ladvapi32"] if windows and not mock_registry else []),
                             "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


@pytest.fixture(scope="module")
def tool(tmp_path_factory):
    return build_tool(tmp_path_factory.mktemp("steam-workshop"))


def run(tool, *args, home=None, setting="", enabled=True, data_root=None):
    env = dict(os.environ, HOME=str(home or "/nonexistent"), TEST_STEAM_SETTING=setting,
               TEST_WORKSHOP_ENABLED=str(int(enabled)), TEST_DATA_ROOT=str(data_root or "/nonexistent"))
    result = subprocess.run([str(tool), *map(str, args)], capture_output=True, text=True, timeout=20, env=env)
    assert result.returncode == 0, result.stderr
    fields, lists = {}, {}
    for line in result.stdout.splitlines():
        key, value = line.split("=", 1)
        lists.setdefault(key, []).append(value)
        fields[key] = value
    return fields, lists


def item_json(**changes):
    document = {"Engine": "Halo1", "Title": {"Neutral": "Pro Edition"},
                "GameModContents": {"MultiplayerMaps": ["maps\\beaver.json", "maps\\derelict.json"]}}
    document.update(changes)
    return json.dumps(document)


def test_item_text(tool, tmp_path):
    path = tmp_path / "ModInfo.json"
    path.write_text(item_json())
    fields, lists = run(tool, "item", path)
    assert fields["ok"] == "1" and fields["title"] == "Pro Edition" and fields["halo1"] == "1"
    assert lists["json"] == ["maps/beaver.json", "maps/derelict.json"]
    # another engine reads, flagged: the scan skips it
    path.write_text(item_json(Engine="Halo2"))
    assert run(tool, "item", path)[0]["halo1"] == "0"
    # no maps (a campaign-only item) is fine and empty
    path.write_text(json.dumps({"Engine": "Halo1", "Title": {"Neutral": "Campaign"}}))
    fields, _ = run(tool, "item", path)
    assert fields["ok"] == "1" and fields["maps"] == "0"


@pytest.mark.parametrize("bad", ["..\\\\x.json", "../x.json", "/etc/passwd", "C:\\\\x.json", "a\\u0001.json", ""])
def test_item_refuses_paths_that_leave_the_item(tool, tmp_path, bad):
    path = tmp_path / "ModInfo.json"
    path.write_text(item_json(GameModContents={"MultiplayerMaps": [json.loads('"' + bad + '"')]}))
    assert run(tool, "item", path)[0]["ok"] == "0"


@pytest.mark.parametrize("text", ["", "[]", "not json", '{"Engine":', "{" * 40 + "}" * 40, '"text"'])
def test_item_refuses_what_is_not_an_object(tool, tmp_path, text):
    path = tmp_path / "ModInfo.json"
    path.write_text(text)
    assert run(tool, "item", path)[0]["ok"] == "0"


def test_item_too_large_or_too_many_maps(tool, tmp_path):
    path = tmp_path / "ModInfo.json"
    path.write_text(item_json(Padding="x" * (300 * 1024)))
    assert run(tool, "item", path)[0]["ok"] == "0"
    path.write_text(item_json(GameModContents={"MultiplayerMaps": ["maps\\m%d.json" % n for n in range(65)]}))
    assert run(tool, "item", path)[0]["ok"] == "0"


def test_map_text(tool, tmp_path):
    path = tmp_path / "map.json"
    path.write_text(json.dumps({"ScenarioFile": "maps\\pat_beavercreek.map", "Title": {"Neutral": "Better Creek"},
                                "Images": {"Thumbnail": "images\\creek.png"}}))
    fields, _ = run(tool, "map", path)
    assert (fields["ok"], fields["name"], fields["title"], fields["thumbnail"]) == (
        "1", "pat_beavercreek", "Better Creek", "images/creek.png")
    # the preview must stay inside the item
    path.write_text(json.dumps({"ScenarioFile": "x", "Images": {"Thumbnail": "..\\..\\secret.png"}}))
    fields, _ = run(tool, "map", path)
    assert fields["ok"] == "1" and fields["thumbnail"] == "" and fields["title"] == ""
    for scenario in ("", "..", "a:b", "tab\tname", "n" * 80):
        path.write_text(json.dumps({"ScenarioFile": scenario}))
        assert run(tool, "map", path)[0]["ok"] == "0", scenario
    path.write_text(json.dumps({"Title": {"Neutral": "no scenario"}}))
    assert run(tool, "map", path)[0]["ok"] == "0"


def test_library_list(tool, tmp_path):
    path = tmp_path / "libraryfolders.vdf"
    path.write_text('"libraryfolders"\n{\n\t"0"\n\t{\n\t\t"path"\t\t"/opt/user/.local/share/Steam"\n'
                    '\t\t"label"\t\t""\n\t}\n\t"1"\n\t{\n\t\t"path"\t\t"D:\\\\SteamLibrary"\n\t}\n'
                    '\t"2"\n\t{\n\t\t"path"\t\t"/with \\"quote\\""\n\t\t"path"\t\t"\n\t\t"path"\t\t""\n\t}\n}\n')
    fields, lists = run(tool, "vdf", path)
    assert lists["path"] == ["/opt/user/.local/share/Steam", "D:\\SteamLibrary", '/with "quote"']


def make_map(folder, name):
    folder.mkdir(parents=True, exist_ok=True)
    (folder / (name + ".map")).write_bytes(b"head" + bytes(100))


def make_item(library, item_id, title="Item", engine="Halo1", maps=(("beaver", "Better Creek"),), manifest="ModInfo.json",
              preview=True):
    folder = library / "steamapps/workshop/content/976730" / item_id
    folder.mkdir(parents=True)
    names = []
    for name, map_title in maps:
        # (the folder's spelling differs from the manifest's: names are found case-blind)
        (folder / "Maps").mkdir(exist_ok=True)
        (folder / "Maps" / (name + ".json")).write_text(json.dumps({
            "ScenarioFile": "maps\\" + name, "Title": {"Neutral": map_title},
            "Images": {"Thumbnail": "Images\\" + name + ".PNG"}}))
        make_map(folder / "Maps", name.upper() if name == "shouting" else name)
        if preview:
            (folder / "images").mkdir(exist_ok=True)
            (folder / "images" / (name + ".png")).write_bytes(b"png")
        names.append("maps\\" + name + ".json")
    (folder / manifest).write_text(item_json(Engine=engine, Title={"Neutral": title},
                                              GameModContents={"MultiplayerMaps": names}))
    return folder


def test_scan(tool, tmp_path):
    home = tmp_path / "home"
    steam = home / ".local/share/Steam"
    other = tmp_path / "library two"
    # both usual links lead to the one Steam folder: each item once
    (home / ".steam").mkdir(parents=True)
    if sys.platform != "win32":
        (home / ".steam/steam").symlink_to(steam)
        (home / ".steam/root").symlink_to(steam)
    (steam / "steamapps").mkdir(parents=True)
    (steam / "steamapps/libraryfolders.vdf").write_text(
        '"libraryfolders"\n{\n"0"\n{\n"path"\t"%s"\n}\n"1"\n{\n"path"\t"%s"\n}\n}\n' % (steam.as_posix(), other.as_posix()))
    make_item(steam, "111", "Pro Edition", maps=(("beaver", "Better Creek"), ("shouting", "Loud")))
    folder = make_item(other, "222", "Social Pack", manifest="modinfo.JSON")
    make_item(other, "333", engine="Halo2")
    make_item(other, "444", maps=())                       # a campaign-only item
    make_item(other, "555")                                # this one is also in the first library:
    make_item(steam, "555", "Seen First")                  # the first wins
    make_item(other, "notanid")
    make_item(other, "666", maps=(("gone", "No File"),))
    for leftover in (other / "steamapps/workshop/content/976730/666/Maps").glob("gone.map"):
        leftover.unlink()
    # a manifest naming a json outside the item refuses the item
    outside = make_item(other, "777")
    (outside / "ModInfo.json").write_text(item_json(GameModContents={"MultiplayerMaps": ["..\\\\111\\\\maps\\\\beaver.json"]}))
    fields, lists = run(tool, "scan", home=home, setting=str(steam) if sys.platform == "win32" else "")
    rows = [row.split("|") for row in lists["map"]]
    assert sorted((r[0], r[2]) for r in rows) == [("111", "SHOUTING"), ("111", "beaver"), ("222", "beaver"),
                                                  ("555", "beaver")]
    assert fields["count"] == "4" and fields["again"] == "4"
    by_id = {(r[0], r[2]): r for r in rows}
    # the file as it is spelled on disk (SHOUTING.map for a json that says shouting), real paths, a found preview
    # (found through the first link to Steam's folder)
    assert by_id[("111", "SHOUTING")][4].endswith("/steamapps/workshop/content/976730/111/Maps/SHOUTING.map")
    assert by_id[("111", "beaver")][5].endswith("/steamapps/workshop/content/976730/111/images/beaver.png")
    assert by_id[("555", "beaver")][1] == "Seen First"
    assert fields["folder"].startswith("1 ") and fields["folder"].endswith("/976730/111/Maps")
    assert fields["none"] == "0" and fields["small"] == "0"
    assert fields["title"] == "1 Social Pack"
    assert folder.exists()


def test_setting_names_a_steam_folder(tool, tmp_path):
    steam = tmp_path / "Elsewhere"
    make_item(steam, "888", "From the setting", preview=False)
    fields, lists = run(tool, "scan", setting=str(steam))
    assert fields["count"] == "1"
    row = lists["map"][0].split("|")
    assert row[:4] == ["888", "From the setting", "beaver", "Better Creek"] and row[5] == ""
    # no Steam at all: an empty catalog, no error
    assert run(tool, "scan", home=tmp_path / "nothing")[0]["count"] == "0"


@pytest.mark.skipif(sys.platform != "win32", reason="Windows registry discovery")
@pytest.mark.parametrize("key", ["USER", "32", "64"])
def test_windows_registry_installation_and_library(tool, tmp_path, monkeypatch, key):
    steam, library = tmp_path / "Custom Steam", tmp_path / "Other Drive Library"
    make_item(steam, "111", "Registered installation")
    make_item(library, "222", "Registered library")
    (steam / "steamapps/libraryfolders.vdf").write_text(
        '"libraryfolders" { "0" { "path" "%s" } }' % library.as_posix())
    monkeypatch.setenv("TEST_STEAM_REG_" + key, str(steam))
    fields, rows = run(tool, "scan")
    assert fields["count"] == "2"
    assert [row.split("|")[1] for row in rows["map"]] == ["Registered installation", "Registered library"]
    # A disabled feature must not even query the registry.
    monkeypatch.setenv("TEST_REGISTRY_MUST_NOT_READ", "1")
    assert run(tool, "scan", enabled=False)[0]["count"] == "0"


@pytest.mark.skipif(sys.platform != "win32", reason="Windows registry discovery")
def test_windows_manual_root_precedes_registry_and_registry_precedes_defaults(tool, tmp_path, monkeypatch):
    manual, registered, defaults = (tmp_path / name for name in ("manual", "registered", "defaults"))
    make_item(manual, "111", "Manual wins")
    make_item(registered, "111", "Registry wins")
    make_item(defaults / "Steam", "111", "Default loses")
    for key in ("USER", "32", "64"):
        monkeypatch.setenv("TEST_STEAM_REG_" + key, str(registered))
    monkeypatch.setenv("ProgramFiles(x86)", str(defaults))
    for setting, title in ((str(manual), "Manual wins"), ("", "Registry wins")):
        fields, rows = run(tool, "scan", setting=setting)
        assert fields["count"] == "1"
        assert rows["map"][0].split("|")[1] == title


@pytest.mark.skipif(sys.platform != "win32", reason="Windows registry discovery")
@pytest.mark.parametrize("bad", ["", "!denied", "!type", "x" * 512, "Z:/missing-steam-installation"])
def test_windows_bad_registry_value_keeps_fallbacks(tool, tmp_path, monkeypatch, bad):
    make_item(tmp_path / "Steam", "111", "Fallback")
    monkeypatch.setenv("ProgramFiles(x86)", str(tmp_path))
    for key in ("USER", "32", "64"):
        monkeypatch.setenv("TEST_STEAM_REG_" + key, bad)
    fields, rows = run(tool, "scan")
    assert fields["count"] == "1"
    assert rows["map"][0].split("|")[1] == "Fallback"


def test_scan_keeps_short_map_names(tool, tmp_path):
    make_item(tmp_path, "111", maps=(("a10", "One"), ("test", "Two"), ("longer", "Three")))
    _, rows = run(tool, "scan", setting=str(tmp_path))
    assert {row.split("|")[2] for row in rows["map"]} == {"a10", "test", "longer"}
    assert run(tool, "scan", setting=str(tmp_path), enabled=False)[0]["count"] == "0"


def test_translation_keeps_local_override_and_legacy_roots(tool, tmp_path):
    library, data = tmp_path / "steam", tmp_path / "data"
    make_item(library, "111")
    data.mkdir()
    fields, _ = run(tool, "translate", "d:\\mcc_maps\\111\\beaver.map", setting=str(library), data_root=data)
    assert fields["path"].replace("\\", "/").endswith("/111/Maps/beaver.map")
    override = data / "MCC_MAPS/111"
    override.mkdir(parents=True)
    (override / "beaver.map").write_bytes(b"manual")
    for prefix in ("d:\\", ""):
        fields, _ = run(tool, "translate", prefix + "mcc_maps\\111\\beaver.map", setting=str(library), data_root=data)
        assert Path(fields["path"]) == override / "beaver.map"
    for folder in ("maps", "custom_maps"):
        fields, _ = run(tool, "translate", "d:\\" + folder + "\\a10.map", setting=str(library), data_root=data)
        assert Path(fields["path"]) == data / folder / "a10.map"


def make_campaign(folder, names=("z10", "a10")):
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "campaign").mkdir(exist_ok=True)
    (folder / "ModInfo.json").write_text(item_json(GameModContents={"HasCampaign": True}))
    (folder / "CampaignInfo.json").write_text(json.dumps({"Title": {"Neutral": "Example campaign"},
        "CampaignMaps": [f"campaign/{name}.json" for name in names]}))
    for name in names:
        (folder / f"campaign/{name}.json").write_text(json.dumps({"ScenarioFile": f"levels/example/{name}",
            "Title": {"Neutral": "Mission " + name}, "CampaignMapKind": "Default"}))
        make_map(folder / "maps", name)


def test_campaign_discovery_and_explicit_order(tool, tmp_path, monkeypatch):
    folder = tmp_path / "steamapps/workshop/content/976730/111"
    make_campaign(folder)
    assert run(tool, "scan", setting=str(tmp_path))[0]["count"] == "0"
    monkeypatch.setenv("TEST_MCC_CAMPAIGNS", "1")
    fields, rows = run(tool, "scan", setting=str(tmp_path))
    assert fields["count"] == "2"
    assert [row.split("|")[2] for row in rows["map"]] == ["z10", "a10"]
    fields, rows = run(tool, "manifest", folder)
    assert fields["result"] == "1" and fields["title"] == "Example campaign"
    assert rows["mission"] == ["z10|Mission z10", "a10|Mission a10"]
    # Missing binaries do not change the authored sequence.
    (folder / "maps/a10.map").unlink()
    assert run(tool, "manifest", folder)[0]["count"] == "2"


def test_local_campaign_override_never_borrows_steam_metadata(tool, tmp_path, monkeypatch):
    library, data = tmp_path / "steam", tmp_path / "data"
    make_campaign(library / "steamapps/workshop/content/976730/111")
    data.mkdir()
    monkeypatch.setenv("TEST_MCC_CAMPAIGNS", "1")
    args = dict(setting=str(library), data_root=data)
    assert run(tool, "manifest-level", "mcc_maps\\111\\z10", **args)[0]["count"] == "2"
    local = data / "MCC_MAPS/111"
    local.mkdir(parents=True)
    assert run(tool, "manifest-level", "mcc_maps\\111\\z10", **args)[0]["result"] == "0"
    make_campaign(local, ("c10", "b10"))
    fields, rows = run(tool, "manifest-level", "mcc_maps\\111\\c10", **args)
    assert fields["result"] == "1" and rows["mission"][0] == "c10|Mission c10"


@pytest.mark.parametrize("fault", ["missing", "traversal", "duplicate_path", "duplicate_name", "kind", "engine", "disabled", "oversize", "too_many"])
def test_campaign_manifest_refuses_partial_or_ambiguous_metadata(tool, tmp_path, fault):
    make_campaign(tmp_path)
    campaign = tmp_path / "CampaignInfo.json"
    mission = tmp_path / "campaign/a10.json"
    if fault == "missing": mission.unlink()
    elif fault == "traversal": campaign.write_text(json.dumps({"CampaignMaps": ["../a10.json"]}))
    elif fault == "duplicate_path": campaign.write_text(json.dumps({"CampaignMaps": ["campaign/a10.json", "CAMPAIGN/A10.JSON"]}))
    elif fault == "duplicate_name": mission.write_text(json.dumps({"ScenarioFile": "z10"}))
    elif fault == "kind": mission.write_text(json.dumps({"ScenarioFile": "a10", "CampaignMapKind": "Hub"}))
    elif fault in ("engine", "disabled"):
        (tmp_path / "ModInfo.json").write_text(item_json(Engine="Halo2" if fault == "engine" else "Halo1",
            GameModContents={"HasCampaign": fault != "disabled"}))
    elif fault == "oversize": mission.write_text(" " * (256 * 1024 + 1))
    elif fault == "too_many": campaign.write_text(json.dumps({"CampaignMaps": [f"campaign/{i}.json" for i in range(65)]}))
    fields, _ = run(tool, "manifest", tmp_path)
    assert fields["result"] == "-1" and fields["count"] == "0" and fields["reason"]
