"""Run the real debug observation/action gates against controlled frame ticks."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest
from harness import function
from net_compare import pair_ticks

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def sampler(tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed")
    work = tmp_path_factory.mktemp("network-test-sampling")
    update = function((ROOT / "port/linux/game/network_test.c").read_text(), "network_test_update")
    # Keep the production reset, cue, observation and scripted-action gates.
    # The unrelated action bodies are represented by an action-time recorder.
    start = update.index("\t/* the game running:")
    stop = update.index("\t\tif (network_test.shoot_interval", start)
    gate = update[start:stop]
    source = work / "sampling.c"
    source.write_text(r'''
#include <stdio.h>
#include <stdlib.h>
#define TICKS_PER_SECOND 30
#define NONE -1
static struct {long logged_time, sample_time, cues_start, next_cue;} network_test;
static long tick;
static int active=1;
static long game_time_get(void) {return tick;}
static int game_in_progress(void) {return active;}
static void network_test_log_players(void) {printf("S %ld\n",tick);}
static void network_test_run_cues(void) {}
static void frame(int main_menu_loaded) {
''' + gate + r'''
        printf("A %ld\n",tick);
    }
}
int main(int argc,char **argv) {
    int i,menu;
    for(i=1;i<argc;i++) {
        menu=0;active=1;
        if(sscanf(argv[i],"%ld,%d,%d",&tick,&menu,&active)<1)return 2;
        frame(menu);
    }
    return 0;
}
''')
    binary = work / ("sampling.exe" if os.name == "nt" else "sampling")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else []
    result = subprocess.run([compiler, *flags, "-std=c89", str(source), "-o", str(binary)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return binary


def observe(sampler, ticks):
    result = subprocess.run([str(sampler), *map(str, ticks)], capture_output=True, text=True, check=True)
    return {kind: [int(line[2:]) for line in result.stdout.splitlines() if line.startswith(kind + " ")]
            for kind in ("S", "A")}


def test_late_first_frame_aligns_observations_without_retiming_actions(sampler):
    host = observe(sampler, range(182))
    client = observe(sampler, range(39, 182))
    assert host["S"] == [30, 60, 90, 120, 150, 180]
    assert client["S"] == [39, 60, 90, 120, 150, 180]
    assert client["A"] == [39, 69, 99, 129, 159]
    assert pair_ticks(host["S"], client["S"]) == [(tick, tick) for tick in range(60, 181, 30)]


def test_skipped_boundary_logs_actual_tick_without_backfill_or_duplicates(sampler):
    result = observe(sampler, [0, 29, 34, 34, 59, 62, 62, 151, 151, 179, 180])
    assert result["S"] == [34, 62, 151, 180]
    assert result["A"] == [34, 151]
    # A genuinely late observation still fails the established two-tick gate.
    assert pair_ticks([30, 60, 90, 120, 150, 180], result["S"]) == [(60, 62), (150, 151), (180, 180)]


def test_revert_restarts_observations_even_before_next_action(sampler):
    result = observe(sampler, [39, 60, 58, 59, 60, 69, 90])
    assert result["S"] == [39, 60, 58, 60, 90]
    assert result["A"] == [39, 69]


def test_new_game_menu_and_inactive_frames(sampler):
    result = observe(sampler, [30, 60, "90,1,1", "120,0,0", 0, 29, 30, 30, 60])
    assert result["S"] == [30, 60, 30, 60]
    assert result["A"] == [30, 60, 30, 60]
