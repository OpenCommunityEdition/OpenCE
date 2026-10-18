"""Exercise the real changelog layout module with the generated main menu."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import xml.etree.ElementTree as ET

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="session")
def layout_tool(tmp_path_factory):
    cc = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not cc:
        pytest.skip("a C compiler is needed for the changelog layout test")
    tree = ET.parse(ROOT / "port/assets/menus/ce/main_menu.xml")
    rows = tree.find("widget[@name='main_menu/main_menu_select_list']").findall("child")
    assert len(rows) == 7
    setup = "\n".join(
        f"children[{i}].widget = {json.dumps(row.get('widget'))}; "
        f"children[{i}].x = {row.get('x')}; children[{i}].y = {row.get('y')};"
        for i, row in enumerate(rows)
    )
    folder = tmp_path_factory.mktemp("changelog-layout")
    source = folder / "layout.c"
    binary = folder / ("layout.exe" if os.name == "nt" else "layout")
    source.write_text(r'''
#include "halo_menus.h"
#include "changelog_menu.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    struct halo_menus menus = {0};
    struct halo_menu_widget widget = {0};
    struct halo_menu_child children[7] = {0}, before[7];
    long i;
    int enabled;
    if (argc != 2) return 2;
    enabled = strcmp(argv[1], "off") != 0;
    menus.widgets = &widget; menus.widget_count = 1;
    menus.children = children; menus.child_count = 7;
    widget.name = "main_menu/main_menu_select_list"; widget.first_child = 0;
    for (i = 0; i < 7; i++) {
        children[i].nested = HALO_MENU_NONE;
        children[i].next = i == 6 ? HALO_MENU_NONE : i + 1;
    }
''' + setup + r'''
    if (!strcmp(argv[1], "custom")) children[0].y = 100;
    if (!strcmp(argv[1], "absent")) children[4].next = 6;
    if (!strcmp(argv[1], "other")) widget.name = "main_menu/mods/mods_list";
    memcpy(before, children, sizeof(children));
    changelog_menu_configure(&menus, enabled);
    if (!strcmp(argv[1], "repeat")) changelog_menu_configure(&menus, enabled);
    if (!strcmp(argv[1], "custom") || !strcmp(argv[1], "absent") || !strcmp(argv[1], "other")) {
        if (memcmp(before, children, sizeof(children))) return 3;
    }
    for (i = widget.first_child; i != HALO_MENU_NONE; i = children[i].next)
        printf("%s %ld %ld\n", children[i].widget, children[i].x, children[i].y);
    return 0;
}
''')
    command = [cc, "-std=c89", "-Wall", "-Wextra", "-Werror",
               "-iquote", str(ROOT / "port/linux/include"),
               "-iquote", str(ROOT / "port/linux/src"), str(source),
               str(ROOT / "port/linux/src/changelog_menu.c"), "-o", str(binary)]
    if os.name == "nt":
        command.insert(1, "--target=i686-pc-windows-msvc")
    built = subprocess.run(command, capture_output=True, text=True)
    assert built.returncode == 0, built.stdout + built.stderr
    return binary


def run_layout(tool, mode):
    output = subprocess.run([str(tool), mode], check=True, capture_output=True, text=True).stdout
    return [line.rsplit(" ", 2) for line in output.splitlines()]


@pytest.mark.parametrize("mode", ["on", "repeat"])
def test_enabled_entry_fits_and_is_between_mods_and_quit(layout_tool, mode):
    rows = run_layout(layout_tool, mode)
    assert [int(row[2]) for row in rows] == [211, 247, 283, 319, 355, 391, 427]
    assert [row[0].rsplit("_", 1)[-1] for row in rows[4:6]] == ["mods", "changelog"]
    assert rows[-1][0].endswith("quit_game")
    assert all(int(row[2]) + 33 <= 480 for row in rows)


def test_disabled_restores_original_six_rows(layout_tool):
    rows = run_layout(layout_tool, "off")
    assert len(rows) == 6
    assert all("changelog" not in row[0] for row in rows)
    assert [int(row[2]) for row in rows] == [247, 283, 319, 355, 391, 427]


@pytest.mark.parametrize("mode", ["custom", "absent", "other"])
def test_overrides_and_unrelated_menus_are_unchanged(layout_tool, mode):
    run_layout(layout_tool, mode)
