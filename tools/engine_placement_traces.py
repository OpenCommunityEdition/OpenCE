"""Records the reference game's placements, for the native engine's
placement checks: the reference game (build/macos/halo_host, `ninja macos`)
hosts each stock map headless with no menus (a campaign map as co-op)
while debug.placement_trace logs every object its scenario places
(port/linux/game/placement_trace.c), and the "placement trace:" lines are
written to build/placement_traces/<map>.csv, a row an object, with the
structure BSP the game was in at the end (a mission's scripts may have
switched it, placing that BSP's scenery) in <map>.bsp. The traces are
made from the game's data, so they are never committed. The home folder is
never touched: HOME and XDG_DATA_HOME are in a temporary folder.

    python3 tools/engine_placement_traces.py [map ...]
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
TRACES = ROOT / "build" / "placement_traces"
MULTIPLAYER = ("beavercreek", "bloodgulch", "boardingaction", "carousel", "chillout", "damnation", "hangemhigh",
               "longest", "prisoner", "putput", "ratrace", "sidewinder", "wizard")
CAMPAIGN = ("a10", "a30", "a50", "b30", "b40", "c10", "c20", "c40", "d20", "d40")
COLUMNS = ("type", "block_index", "x", "y", "z", "forward_i", "forward_j", "forward_k", "up_i", "up_j", "up_k",
           "regions", "parent", "tag")
NUMBER = r"-?[\d.]+(?:e[-+]?\d+)?"
LINE = re.compile(r"placement trace: (\d+) (-?\d+)((?: " + NUMBER + r"){9}) ([\d.]+) (-?\d+) (.+)$")
BEGIN = re.compile(r"placement trace: begin (.+)$")
DONE = re.compile(r"placement trace: done (-?\d+) (-?\d+) (\d+)")


def scenario(name: str) -> str:
    """The map's scenario tag."""
    return f"levels\\{name}\\{name}" if name in CAMPAIGN else f"levels\\test\\{name}\\{name}"


def network_test(name: str) -> str:
    """debug.network_test's game for the map: co-op for a campaign map."""
    return f"host:levels\\{name}\\{name}:coop" if name in CAMPAIGN else f"host:{name}"


def parse(output: str, scenario: str) -> tuple[list[list[str]], dict[str, int]]:
    """The rows of `scenario`'s placements in a log, and the end's BSP,
    difficulty and count; the rows are each object's columns, in COLUMNS'
    order. Another level's (the menus') are left out."""
    rows: list[list[str]] = []
    level = None
    for line in output.splitlines():
        line = line.strip()
        begin = BEGIN.search(line)
        if begin:
            level = begin.group(1)
            rows = []
            continue
        if level != scenario:
            continue
        done = DONE.search(line)
        if done:
            bsp, difficulty, count = (int(value) for value in done.groups())
            if count != len(rows):
                raise ValueError(f"{count} objects placed, {len(rows)} read")
            return rows, {"bsp": bsp, "difficulty": difficulty, "count": count}
        match = LINE.search(line)
        if match:
            rows.append([match.group(1), match.group(2), *match.group(3).split(), match.group(4), match.group(5),
                         match.group(6)])
    raise ValueError("the trace did not finish")


def write(path: Path, rows: list[list[str]], bsp: int = 0) -> None:
    """A trace's rows as a CSV file (the tags quoted: some have spaces), and
    the structure BSP it ended in beside it, in a .bsp file."""
    def cell(value: str) -> str:
        return '"' + value.replace('"', '""') + '"' if any(c in value for c in ',"') else value
    path.write_text(",".join(COLUMNS) + "\n" + "".join(",".join(cell(value) for value in row) + "\n" for row in rows))
    path.with_suffix(".bsp").write_text(f"{bsp}\n")


def record(name: str, exit_after: int = 60, timeout: int = 300) -> tuple[list[list[str]], dict[str, int]]:
    """The map's placements, from one headless run of the reference game."""
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
            "HALO_NETWORK_TEST": network_test(name), "HALO_NET_ADDRESS": "127.0.0.1",
            "HALO_NET_ONLINE": "false", "HALO_PLACEMENT_TRACE": "true",
        })
        process = subprocess.Popen([str(HOST), str(IMAGE)], cwd=data_root, env=environment,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
        output = ""
        try:
            assert process.stdout is not None
            for line in process.stdout:
                output += line
                if "placement trace: done" in line and f"placement trace: begin {scenario(name)}" in output:
                    break
        finally:
            process.kill()
            process.wait(timeout=timeout)
        log = data_root / "debug.txt"
        if log.exists() and f"placement trace: begin {scenario(name)}" not in output:
            output += log.read_text(errors="replace")
    try:
        return parse(output, scenario(name))
    except ValueError as error:
        traced = "\n".join(line for line in output.splitlines() if "trace" in line or "crash" in line or "error" in line)
        raise RuntimeError(f"{name}: {error}\n{traced[-4000:]}") from None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("maps", nargs="*", default=list(MULTIPLAYER + CAMPAIGN))
    arguments = parser.parse_args()
    if not (HOST.is_file() and IMAGE.is_file()):
        print("needs `ninja macos`", file=sys.stderr)
        return 1
    TRACES.mkdir(parents=True, exist_ok=True)
    for name in arguments.maps:
        if not (MAPS / f"{name}.map").is_file():
            print(f"{name}: no assets/maps/{name}.map", file=sys.stderr)
            continue
        rows, end = record(name)
        path = TRACES / f"{name}.csv"
        write(path, rows, end["bsp"])
        print(f"{path.relative_to(ROOT)}: {len(rows)} objects (bsp {end['bsp']}, difficulty {end['difficulty']})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
