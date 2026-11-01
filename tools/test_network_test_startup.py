"""The real automated launch gate must respect the menu's profile initialization."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("omit_readiness", [False, True])
def test_menu_readiness(tmp_path, omit_readiness):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is needed")
    update = function((ROOT / "port/linux/game/network_test.c").read_text(), "network_test_update")
    # Extract the existing menu/intro gate, without the unrelated gameplay
    # scripting above it or the host/join implementation below it.
    start = update.index("\tif (!main_menu_loaded")
    gate = update[start:update.index("\tswitch (network_test.mode)", start)]
    if omit_readiness:
        assert " || filesystem_check_thread_is_active()" in gate
        gate = gate.replace(" || filesystem_check_thread_is_active()", "")
    source = tmp_path / "startup.c"
    source.write_text(r'''
static struct {float menu_seconds;} network_test;
static int initializing, movie=1, movie_checks, stops, starts;
static int filesystem_check_thread_is_active(void) {return initializing;}
static int bink_playback_in_progress(void) {movie_checks++;return movie;}
static void bink_playback_stop(void) {stops++;movie=0;}
static void platform_log(char const *text) {(void)text;}
static void frame(int main_menu_loaded,float seconds) {
''' + gate + r'''
    starts++;
}
int main(void) {
    int i;
    frame(0,10.0f);
    if(starts||movie_checks)return 1;
    initializing=1;
    for(i=0;i<100;i++)frame(1,0.5f);
    if(starts||movie_checks||network_test.menu_seconds)return 1;
    initializing=0;frame(1,10.0f);
    if(starts||stops!=1||network_test.menu_seconds)return 1;
    frame(1,1.0f);if(starts)return 1;
    frame(1,1.0f);return starts==1 ? 0 : 1;
}
''')
    executable = tmp_path / ("startup.exe" if os.name == "nt" else "startup")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if os.name == "nt" else []
    result = subprocess.run([compiler, *flags, "-std=c99", str(source), "-o", str(executable)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    assert subprocess.run([str(executable)]).returncode == int(omit_readiness)
