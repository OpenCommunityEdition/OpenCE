#!/usr/bin/env python3
"""Record the profile of a profile-guided native build (configure.py --pgo=train).

Runs an instrumented build (built with -fprofile-generate), Linux or
Windows, through the main menu and the opening minute of every campaign
level in the game data (level loading, cutscenes, the first gameplay with
AI), each a separate run that quits by itself (HALO_EXIT_AFTER), and merges
what they recorded into the profile the optimised build uses, one of the
profiles kept in pgo/.

Nobody plays, so what the profile learns is the engine's steady work
(rendering, AI, physics, sound), which is what matters for frame rates. The
multiplayer maps are left out: loaded this way they start no game, so nobody
spawns and the camera looks at the sky.

The runs use the game data in assets/ (read only: a scratch data root in the
work directory links to it and has its own init.txt, and saves go there
too), need a display, play silently and take about fifteen minutes. On
Windows, started from a session without a desktop (over ssh), each run is a
scheduled task in the session of the logged-in user.

Usage: pgo_train.py --binary build/linux/pgo-generate/halo --work build/pgo/linux
                    --output pgo/halo_linux.profdata
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict

# seconds in the main menu, and in each level
MENU_SECONDS = 20
LEVEL_SECONDS = 60
# the campaign's levels: a10 to d40
CAMPAIGN_LEVEL = re.compile(r"[a-d][0-9]{2}")

# seconds a run may take beyond its own time to load and quit
MARGIN = 90

WINDOWS = os.name == "nt"
TASK_NAME = "HaloProfileTraining"


def remove_data_root(root: Path) -> None:
    """removes the scratch data root without following its links into the
    game data"""
    if not root.exists():
        return
    for entry in root.iterdir():
        if entry.is_symlink() or (WINDOWS and entry.is_dir() and os.path.isjunction(entry)):
            if WINDOWS and entry.is_dir():
                os.rmdir(entry)
            else:
                entry.unlink()
    shutil.rmtree(root)


def data_root(assets: Path, root: Path) -> None:
    """a data root linking to the game data in assets/, but not to the text
    files there (init.txt, and the logs the game writes: debug.txt ...).
    Windows links directories as junctions, which need no privileges, and
    copies files."""
    remove_data_root(root)
    root.mkdir(parents=True)
    for entry in assets.iterdir():
        if entry.suffix.lower() == ".txt":
            continue
        target = root / entry.name
        if not WINDOWS:
            target.symlink_to(entry.resolve())
        elif entry.is_dir():
            subprocess.run(["cmd", "/c", "mklink", "/J", str(target), str(entry.resolve())],
                           check=True, stdout=subprocess.DEVNULL)
        else:
            shutil.copy2(entry, target)


def without_desktop() -> bool:
    """whether this process runs in Windows' session 0, which has no desktop
    and no OpenGL driver (a service, or ssh)"""
    if not WINDOWS:
        return False
    import ctypes

    session = ctypes.c_ulong()
    if not ctypes.windll.kernel32.ProcessIdToSessionId(os.getpid(), ctypes.byref(session)):
        return False
    return session.value == 0


def run_directly(binary: Path, settings: Dict[str, str], timeout: int) -> int:
    try:
        return subprocess.run([str(binary)], env=dict(os.environ, **settings), stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL, timeout=timeout).returncode
    except subprocess.TimeoutExpired:
        return -1


def run_on_desktop(binary: Path, settings: Dict[str, str], timeout: int, work: Path) -> int:
    """runs the game in the session of the user logged in at the computer,
    through a scheduled task, and waits for it to finish"""
    script = work / "run.cmd"
    done = work / "run.done"
    lines = ["@echo off", f'cd /d "{Path.cwd()}"']
    # batch files expand %...%: the profile file name's %p must stay
    lines += [f'set "{name}={value.replace("%", "%%")}"' for name, value in settings.items()]
    # (the redirection goes first: "echo 0>file" would redirect handle 0)
    lines += [f'"{binary.resolve()}" > nul 2>&1', f'>"{done.resolve()}" echo %ERRORLEVEL%']
    script.write_text("\r\n".join(lines) + "\r\n", encoding="ascii")
    if done.exists():
        done.unlink()
    subprocess.run(["schtasks", "/create", "/tn", TASK_NAME, "/tr", str(script.resolve()), "/sc", "once",
                    "/st", "23:59", "/it", "/f"], check=True, stdout=subprocess.DEVNULL)
    try:
        subprocess.run(["schtasks", "/run", "/tn", TASK_NAME], check=True, stdout=subprocess.DEVNULL)
        deadline = time.monotonic() + timeout
        while not done.exists():
            if time.monotonic() > deadline:
                subprocess.run(["schtasks", "/end", "/tn", TASK_NAME], stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
                return -1
            time.sleep(1)
        time.sleep(1)
        text = done.read_text(encoding="ascii", errors="replace").strip()
        return int(text) if text.lstrip("-").isdigit() else -1
    finally:
        subprocess.run(["schtasks", "/delete", "/tn", TASK_NAME, "/f"], stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True, help="scratch directory for the runs")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--assets", type=Path, default=Path("assets"))
    parser.add_argument("--llvm-profdata", default=shutil.which("llvm-profdata") or "llvm-profdata")
    args = parser.parse_args()

    if not (args.assets / "maps").is_dir():
        sys.exit(f"profile training needs the game data: {args.assets / 'maps'} not found (see README.md)")
    if not WINDOWS and not os.environ.get("WAYLAND_DISPLAY") and not os.environ.get("DISPLAY"):
        sys.exit("profile training runs the game and needs a display")
    use_task = without_desktop()
    if use_task:
        print("pgo: no desktop here: the runs are scheduled tasks on the logged-in user's desktop", flush=True)

    work = args.work
    raw = work / "raw"
    root = work / "data"
    saves = work / "save"
    if raw.exists():
        shutil.rmtree(raw)
    raw.mkdir(parents=True)
    saves.mkdir(parents=True, exist_ok=True)
    data_root(args.assets, root)

    levels = sorted(path.stem for path in (args.assets / "maps").glob("*.map")
                    if CAMPAIGN_LEVEL.fullmatch(path.stem.lower()))
    if not levels:
        sys.exit(f"pgo: no campaign levels in {args.assets / 'maps'}")
    scenes = [("menu", None, MENU_SECONDS)] + [(level, level, LEVEL_SECONDS) for level in levels]
    for name, level, seconds in scenes:
        init = root / "init.txt"
        if level:
            init.write_text(f"map_name levels\\{level}\\{level}\n", encoding="ascii")
        elif init.exists():
            init.unlink()
        settings = {
            "HALO_DATA_ROOT": str(root.resolve()),
            "HALO_SAVE_ROOT": str(saves.resolve()),
            "HALO_EXIT_AFTER": str(seconds),
            "HALO_NO_VSYNC": "1",
            "HALO_FULLSCREEN": "0",
            "HALO_VOLUME": "0",
            # no internet play (nor its registering of invite links)
            "HALO_NET_ONLINE": "0",
            "LLVM_PROFILE_FILE": str((raw / f"{name}-%p.profraw").resolve()),
        }
        print(f"pgo: {name} ({seconds} s)", flush=True)
        if use_task:
            status = run_on_desktop(args.binary, settings, seconds + MARGIN, work)
        else:
            status = run_directly(args.binary, settings, seconds + MARGIN)
        if status != 0:
            sys.exit(f"pgo: {name} " + ("did not quit" if status == -1 else f"exited with status {status}"))

    profiles = sorted(glob.glob(str(raw / "*.profraw")))
    if not profiles:
        sys.exit("pgo: the instrumented build wrote no profile")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([args.llvm_profdata, "merge", "-o", str(args.output), *profiles], check=True)
    remove_data_root(root)
    print(f"pgo: {args.output} from {len(profiles)} runs")


if __name__ == "__main__":
    main()
