"""Records the native engine's reference movement traces: the reference
game (build/macos/halo_host, `ninja macos`) hosts bloodgulch headless with
no menus while debug.movement_trace plays each script of inputs, and its
log's "movement trace:" lines (port/linux/game/movement_trace.c) are written
to engine/tests/traces/<script>.csv, one row a 30 Hz tick. The home folder
is never touched: HOME and XDG_DATA_HOME are in a temporary folder.

    python3 tools/engine_movement_traces.py [script ...]
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HOST = ROOT / "build" / "macos" / "halo_host"
IMAGE = ROOT / "build" / "macos" / "halo_guest.elf"
MAPS = ROOT / "assets" / "maps"
TRACES = ROOT / "engine" / "tests" / "traces"
SCRIPTS = ("run", "strafe", "jump", "runjump", "crouch", "fall", "ramp")
COLUMNS = ("tick", "x", "y", "z", "vx", "vy", "vz", "yaw", "airborne", "airborne_ticks", "crouch", "landing",
           "body", "shield")
LINE = re.compile(r"movement trace: (\w+) (-?\d+(?: -?[\d.]+(?:e[-+]?\d+)?){13})$")


def record(script: str, exit_after: int = 45, timeout: int = 240) -> list[list[str]]:
    """The script's rows, from one headless run of the reference game."""
    with tempfile.TemporaryDirectory() as folder:
        folder_path = Path(folder)
        data_root = folder_path / "data"
        data_root.mkdir()
        (data_root / "maps").symlink_to(MAPS)
        home = folder_path / "home"
        environment = dict(os.environ)
        environment.update({
            "HOME": str(home), "XDG_DATA_HOME": str(home / "saves"), "HALO_DATA_ROOT": str(data_root),
            "HALO_EXIT_AFTER": str(exit_after), "HALO_NULL_RENDERER": "1", "HALO_HIDDEN_WINDOW": "1",
            "SDL_VIDEO_DRIVER": "dummy", "SDL_AUDIO_DRIVER": "dummy",
            "HALO_NETWORK_TEST": "host:bloodgulch", "HALO_NET_ADDRESS": "127.0.0.1",
            "HALO_NET_ONLINE": "false", "HALO_MOVEMENT_TRACE": script,
        })
        result = subprocess.run([str(HOST), str(IMAGE)], cwd=data_root, env=environment, capture_output=True,
                                text=True, timeout=timeout)
        output = result.stdout + result.stderr
        log = data_root / "debug.txt"
        if log.exists():
            output += log.read_text(errors="replace")
    if f"movement trace: {script} done" not in output:
        traced = "\n".join(line for line in output.splitlines() if "movement trace" in line or "crash" in line)
        raise RuntimeError(f"{script}: the trace did not finish\n{traced[-4000:]}")
    rows: dict[int, list[str]] = {}
    for line in output.splitlines():
        match = LINE.search(line.strip())
        if match and match.group(1) == script:
            values = match.group(2).split()
            rows[int(values[0])] = values
    return [rows[tick] for tick in sorted(rows)]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("scripts", nargs="*", default=list(SCRIPTS))
    arguments = parser.parse_args()
    if not (HOST.is_file() and IMAGE.is_file()):
        print("needs `ninja macos`", file=sys.stderr)
        return 1
    TRACES.mkdir(parents=True, exist_ok=True)
    for script in arguments.scripts:
        rows = record(script)
        path = TRACES / f"{script}.csv"
        path.write_text(",".join(COLUMNS) + "\n" + "".join(",".join(row) + "\n" for row in rows))
        print(f"{path.relative_to(ROOT)}: {len(rows)} ticks")
    return 0


if __name__ == "__main__":
    sys.exit(main())
