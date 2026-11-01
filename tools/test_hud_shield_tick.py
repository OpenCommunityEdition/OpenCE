"""Shield decay on an unowned object has no player HUD to update."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def test_shield_tick_player_ownership(tmp_path):
    compiler = shutil.which("clang")
    if compiler is None:
        pytest.skip("clang is required")
    source = (ROOT / "source/interface/hud_unit.c").read_text()
    (tmp_path / "under_test.inc").write_text(function(source, "hud_tick_shield"))
    (tmp_path / "shield.c").write_text(r'''
#include <assert.h>
#define NONE (-1)
typedef float real;
static struct player { short local_player_index; } players[2]={{0},{NONE}};
static struct hud { real last_shield_vitality; } hud={2.0f};
static int player_lookups, hud_lookups;
static struct player *player_get(long index)
{
    assert(index>=0 && index<2);
    player_lookups++;
    return &players[index];
}
static struct hud *get_hud_state(short index)
{
    assert(index==0);
    hud_lookups++;
    return &hud;
}
#include "under_test.inc"
int main(void)
{
    hud_tick_shield(NONE,0.25f);
    assert(!player_lookups && !hud_lookups && hud.last_shield_vitality==2.0f);
    hud_tick_shield(0,0.25f);
    assert(player_lookups==1 && hud_lookups==1 && hud.last_shield_vitality==1.75f);
    hud_tick_shield(1,0.25f);
    assert(player_lookups==2 && hud_lookups==1 && hud.last_shield_vitality==1.75f);
    hud_tick_shield(NONE,0.25f);
    assert(player_lookups==2 && hud_lookups==1 && hud.last_shield_vitality==1.75f);
    return 0;
}
''')
    binary = tmp_path / ("shield.exe" if os.name == "nt" else "shield")
    command = [compiler, "-m32", "-std=gnu89", "-O1", "-Wall", "-Werror",
               str(tmp_path / "shield.c"), "-o", str(binary)]
    if os.name == "nt":
        command.append("-fuse-ld=lld")
    subprocess.run(command, check=True, capture_output=True, text=True)
    subprocess.run([str(binary)], check=True, capture_output=True, timeout=10)
