import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CLIPS = ROOT / "port/assets/sound/talking_timer"


def generated(tmp_path):
    output = tmp_path / "embedded.c"
    subprocess.run([sys.executable, str(ROOT / "tools/embed_assets.py"), str(output)], check=True, cwd=ROOT)
    return output.read_text()


def test_clips_table_has_each_clip_in_order(tmp_path):
    text = generated(tmp_path)
    table = re.search(r"const struct talking_timer_clip talking_timer_clips\[NUMBER_OF_TALKING_TIMER_LINES\] =\n\{\n(.*?)\n\};",
                      text, re.S)
    assert table, "talking_timer_clips is not generated"
    entries = re.findall(r"\{ (\w+), (\d+) \},", table.group(1))
    assert len(entries) == 42
    for number, (symbol, size) in enumerate(entries):
        assert int(size) == (CLIPS / f"{number:02d}.opus.bin").stat().st_size
        assert re.search(rf"static const unsigned int {symbol}\[\] = \{{", text)
