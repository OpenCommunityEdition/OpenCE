"""Exercise the actual shared mixer entry with neutral and active MCC gain."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def test_effects_gain_preserves_music_and_legacy_mix(tmp_path):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    mixer = function((ROOT / "source/sound/sound_manager.c").read_text(),
                     "sound_manager_master_gain")
    source = tmp_path / "mixer.c"
    source.write_text("""
typedef float real;
enum {_sound_class_music, _sound_class_scripted_dialog_to_player,
      _sound_class_scripted_dialog_to_other, _sound_class_scripted_dialog_force_unspatialized,
      effects};
static struct {real nondialog_gain;} sound_manager_globals={.5f};
static real script_gain=1;
static int muted;
real mcc_campaign_mix_gain(int music) {return muted ? 0 : music ? 1 : script_gain;}
static real sound_class_get_gain(short c) {(void)c;return .5f;}
static real sound_manager_port_volume(short c) {return c==_sound_class_music ? .75f : .25f;}
""" + mixer + """
int main(void) {
    int c;
    real baseline[5];
    for(c=0;c<5;c++) baseline[c]=sound_manager_master_gain(c);
    if(baseline[effects]!=.0625f || baseline[1]!=.125f || baseline[0]!=.1875f) return 1;
    script_gain=.5f;
    if(sound_manager_master_gain(0)!=baseline[0]) return 2;
    for(c=1;c<5;c++) if(sound_manager_master_gain(c)!=baseline[c]*.5f) return 3;
    muted=1;
    for(c=0;c<5;c++) if(sound_manager_master_gain(c)!=0) return 5;
    muted=0;script_gain=1;
    for(c=0;c<5;c++) if(sound_manager_master_gain(c)!=baseline[c]) return 4;
    return 0;
}
""")
    binary = tmp_path / ("mixer.exe" if sys.platform == "win32" else "mixer")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else []
    result = subprocess.run([compiler] + target + [str(source), "-o", str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    assert subprocess.run([str(binary)], timeout=10).returncode == 0
