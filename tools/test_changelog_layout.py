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
    rows = tree.find("widget[@name='main_menu/main_menu']").findall("child")
    assert any(row.get("widget") == "main_menu/changelog_corner" for row in rows)
    setup = "\n".join(
        f"children[{i}].widget = {json.dumps(row.get('widget'))}; "
        f"children[{i}].x = {row.get('x', 0)}; children[{i}].y = {row.get('y', 0)};"
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
    struct halo_menu_child children[32] = {0}, before[32];
    long i;
    int enabled;
    if (argc != 2) return 2;
    enabled = !strcmp(argv[1], "on") || !strcmp(argv[1], "repeat");
    menus.widgets = &widget; menus.widget_count = 1;
    menus.children = children; menus.child_count = ROWCOUNT;
    widget.name = "main_menu/main_menu"; widget.first_child = 0;
    for (i = 0; i < ROWCOUNT; i++) {
        children[i].nested = HALO_MENU_NONE;
        children[i].next = i == ROWCOUNT - 1 ? HALO_MENU_NONE : i + 1;
    }
'''.replace('ROWCOUNT', str(len(rows))) + setup + r'''
    if (!strcmp(argv[1], "other")) widget.name = "main_menu/mods/mods_list";
    memcpy(before, children, sizeof(children));
    changelog_menu_configure(&menus, enabled);
    changelog_menu_configure(&menus, enabled);
    if (enabled || !strcmp(argv[1], "other")) {
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


def test_corner_is_separate_from_original_six_rows(layout_tool):
    rows = run_layout(layout_tool, "on")
    assert [row for row in rows if row[0].endswith("changelog_corner")] == [
        ["main_menu/changelog_corner", "456", "444"]]
    tree = ET.parse(ROOT / "port/assets/menus/ce/main_menu.xml")
    central = tree.find("widget[@name='main_menu/main_menu_select_list']").findall("child")
    assert len(central) == 6
    assert [int(row.get("y")) for row in central] == [247, 283, 319, 355, 391, 427]


def test_disabled_removes_only_corner_and_preserves_build_label(layout_tool):
    on, off = run_layout(layout_tool, "on"), run_layout(layout_tool, "off")
    assert off == [row for row in on if row[0] != "main_menu/changelog_corner"]
    assert any(row[0] == "main_menu/gulch_build_number" for row in off)


def test_unrelated_menu_is_unchanged(layout_tool):
    assert run_layout(layout_tool, "other") == run_layout(layout_tool, "on")
