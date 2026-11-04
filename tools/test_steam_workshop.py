"""The Steam Workshop catalog (port/linux/src/steam_workshop.c): the item and map json, the library list, and a scan
of a made-up Steam folder through the real directory code."""
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "port/linux/src"

DRIVER = r'''
#include "steam_workshop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
		char folder[512], title[64];

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
	return 0;
}
'''


@pytest.fixture(scope="module")
def tool(tmp_path_factory):
    if sys.platform == "win32":
        pytest.skip("the directory code under test is the POSIX one")
    compiler = os.environ.get("CC") or shutil.which("gcc") or shutil.which("clang")
    if not compiler:
        pytest.skip("a C compiler is needed")
    work = tmp_path_factory.mktemp("steam-workshop")
    (work / "driver.c").write_text(DRIVER)
    exe = work / "driver"
    result = subprocess.run([compiler, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-I", str(SRC), str(work / "driver.c"),
                             str(SRC / "steam_workshop.c"), str(SRC / "posix_files.c"), str(SRC / "server_json.c"),
                             "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


def run(tool, *args, home=None, setting=""):
    env = {"PATH": os.environ["PATH"], "HOME": str(home or "/nonexistent"), "TEST_STEAM_SETTING": setting}
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
    (home / ".steam/steam").symlink_to(steam)
    (home / ".steam/root").symlink_to(steam)
    (steam / "steamapps").mkdir(parents=True)
    (steam / "steamapps/libraryfolders.vdf").write_text(
        '"libraryfolders"\n{\n"0"\n{\n"path"\t"%s"\n}\n"1"\n{\n"path"\t"%s"\n}\n}\n' % (steam, other))
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
    fields, lists = run(tool, "scan", home=home)
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
