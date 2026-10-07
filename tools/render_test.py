#!/usr/bin/env python3
"""Compare the pictures that two renderers draw of the same scenes.

The OpenGL renderer (port/linux/src/d3d8_gl.c) is the reference for any other
(the Direct3D 12 one): the game above them is the same, so of the same game
state, from the same camera, they should draw the same picture. A scene
(tools/render_scenes.toml) is:

- a map, and a game state saved in it with the game frozen (game_speed 0,
  then core_save_name), which each run loads as the map starts
  (core_load_name_at_startup in init.txt), so that every run draws the same
  tick;
- optionally a camera (debug_camera_save's camera.txt, kept in
  tools/render_cameras/<map>/<view>.txt), which debug_camera_load puts the
  view at; without one, the player's own view, with the first-person weapon
  and the HUD;
- the settings to draw it with (HALO_* environment variables: anti-aliasing,
  per-pixel lighting ...), and console commands (rasterizer_* to leave out a
  pass, show_hud 0 ...).

For each run (a renderer, named to the game by HALO_RENDERER) the game starts
once for each scene, with its telnet console on (debug.telnet_console), and
saves the frame (the console's port_screenshot, port/linux/src/screenshot.h)
as <work>/<run>/<scene>.bmp. The first run's pictures are then compared with
each other run's, pixel by pixel, into <work>/report.html.

The runs share a scratch data root (<work>/data) that links to the game data
and keeps the scenes' game states (core/) and the camera (camera.txt). The
pictures and game states are of the game's maps: they stay in build/, out of
the repository. The cameras are only positions, and are kept.

Usage:
  render_test.py run [--runs gl,d3d12] [--scene NAME ...]
      draws the scenes with each renderer and compares the first run with the
      others. --runs gl,gl draws them twice with the same renderer: what then
      differs is what the game does not draw the same twice (the noise floor).
  render_test.py author MAP
      starts the game on a map with its console here, to make scenes: play to
      a place, then ':scene NAME' freezes the game, saves its state and the
      camera, and prints the scene's lines for render_scenes.toml.
  render_test.py compare DIR_A DIR_B
      compares two folders of pictures again.
"""

import argparse
import html
import os
import re
import shutil
import socket
import subprocess
import sys
import time
import tomllib
from pathlib import Path
from typing import Dict, List, Optional

ROOT = Path(__file__).resolve().parent.parent
WINDOWS = os.name == "nt"
SCENES_FILE = ROOT / "tools" / "render_scenes.toml"
CAMERAS = ROOT / "tools" / "render_cameras"
DEFAULT_BINARY = ROOT / "build" / ("windows/halo.exe" if WINDOWS else "linux/halo")
DEFAULT_WORK = ROOT / "build" / "render_test"
DEFAULT_ASSETS = ROOT / "assets"

# the telnet console's (debug.telnet_console_port), and its banner
TELNET_PORT = 2323
BANNER = "Would you like to play a game?"
# its lines are at most this long (TELNET_CLIENT_BUFFER_SIZE in telnet_console.c)
LINE_LIMIT = 127

# seconds for the game to start and load a map, and to save a frame
LOAD_TIMEOUT = 240
CAPTURE_TIMEOUT = 60
# seconds a run may last at all (debug.exit_after), should the harness not
# stop it
RUN_LIMIT = 900

# the settings of every run: a window of a fixed size, no sound, no
# network, no questions
BASE_SETTINGS = {
    "HALO_DISPLAY_MODE": "windowed",
    "HALO_WINDOW_SIZE": "1280x960",
    "HALO_NO_VSYNC": "1",
    "HALO_NO_AUDIO": "1",
    "HALO_NET_ONLINE": "0",
    "HALO_NET_JOIN_FROM_CLIPBOARD": "0",
    "HALO_UPDATE_AUTO": "0",
    "HALO_CRASH_REPORTS": "no",
    # the view follows the last tick, not the mouse: the game is frozen
    "HALO_DIRECT_CAMERA": "0",
    # the random numbers only this machine draws (the fog screen's layers,
    # effects) the same in every run
    "HALO_RANDOM_SEED": "1",
    # real time held still until the picture is asked for, then every frame a
    # 60th of a second however long it takes: the weather, the lens flares and
    # cinematics are where the settle frames take them, whatever renderer
    # draws (a renderer's first frames take longer, making its programs), and
    # however long the console's commands take to arrive
    "HALO_FRAME_TIME": "-0.0166667",
    # lens flares as bright as their visibility tests of the frame before,
    # not of whichever frame the GPU has got to (renderers differ in that)
    "HALO_VISIBILITY_WAIT": "1",
    "HALO_TELNET_CONSOLE": "1",
}

# a scene's defaults (render_scenes.toml's [defaults] changes them)
SCENE_DEFAULTS = {
    "core": None,
    "camera": None,
    "settings": {},
    "commands": [],
    # frames drawn after the last command before the one saved
    "settle_frames": 8,
    # a pixel matches when no channel differs by more than threshold (of 255);
    # a scene passes when at least this fraction of its pixels match
    "threshold": 2,
    "tolerance": 0.995,
}


# ---------- the game


def map_path(name: str) -> str:
    """levels\\b30\\b30 for b30; a path (levels\\test\\bloodgulch\\bloodgulch)
    as it is"""
    return name if "\\" in name else f"levels\\{name}\\{name}"


def ensure_data_root(assets: Path, root: Path) -> None:
    """a data root linking to the game data in assets/ (as tools/pgo_train.py
    makes one), but kept between runs: what the game writes there (core/,
    camera.txt) stays. Windows links directories as junctions, which need no
    privileges, and copies files."""
    if not (assets / "maps").is_dir():
        sys.exit(f"render_test: the game data was not found: {assets / 'maps'} (--assets)")
    root.mkdir(parents=True, exist_ok=True)
    for entry in assets.iterdir():
        target = root / entry.name
        # (the text files are the game's own: init.txt, debug.txt ...)
        if entry.suffix.lower() == ".txt" or target.exists() or target.is_symlink():
            continue
        if not WINDOWS:
            target.symlink_to(entry.resolve())
        elif entry.is_dir():
            subprocess.run(["cmd", "/c", "mklink", "/J", str(target), str(entry.resolve())],
                           check=True, stdout=subprocess.DEVNULL)
        else:
            shutil.copy2(entry, target)


class Game:
    """the game, started on a map with its telnet console on"""

    def __init__(self, binary: Path, root: Path, saves: Path, init: List[str], settings: Dict[str, str],
                 log: Path, port: int):
        (root / "init.txt").write_text("".join(f"{line}\n" for line in init), encoding="ascii")
        saves.mkdir(parents=True, exist_ok=True)
        log.parent.mkdir(parents=True, exist_ok=True)
        environment = dict(os.environ, **BASE_SETTINGS)
        environment.update({
            "HALO_DATA_ROOT": str(root.resolve()),
            "HALO_SAVE_ROOT": str(saves.resolve()),
            "HALO_TELNET_CONSOLE_PORT": str(port),
            "HALO_EXIT_AFTER": str(RUN_LIMIT),
        })
        environment.update(settings)
        self.log_file = open(log, "wb")
        self.process = subprocess.Popen([str(binary)], env=environment, stdout=self.log_file,
                                        stderr=subprocess.STDOUT)
        try:
            self.console = Console(port, self.process, LOAD_TIMEOUT)
        except BaseException:
            self.stop()
            raise

    def stop(self) -> None:
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        self.log_file.close()


class Console:
    """the game's telnet console (source/networking/telnet_console.c). It
    echoes what is typed, and writes each line the game's console prints
    after "\\r\\n"; one client at a time."""

    def __init__(self, port: int, process: subprocess.Popen, timeout: float):
        self.process = process
        self.text = ""
        self.position = 0
        self.counter = 0
        deadline = time.monotonic() + timeout
        while True:
            self.check_alive()
            try:
                self.socket = socket.create_connection(("127.0.0.1", port), timeout=2)
                break
            except OSError:
                if time.monotonic() > deadline:
                    raise TimeoutError(f"the game's console did not open on port {port}")
                time.sleep(0.5)
        self.wait_for(re.compile(re.escape(BANNER)), timeout)

    def check_alive(self) -> None:
        if self.process.poll() is not None:
            raise RuntimeError(f"the game exited (status {self.process.returncode})")

    def send(self, line: str) -> None:
        if len(line) > LINE_LIMIT or not line.isascii():
            raise ValueError(f"the console takes ASCII lines of at most {LINE_LIMIT} characters: {line!r}")
        self.socket.sendall(line.encode("ascii") + b"\r\n")

    def receive(self, seconds: float) -> bool:
        """adds what the game wrote within seconds; False if nothing"""
        self.socket.settimeout(max(seconds, 0.01))
        try:
            data = self.socket.recv(65536)
        except socket.timeout:
            return False
        if not data:
            self.check_alive()
            raise ConnectionError("the game closed its console")
        self.text += data.decode("ascii", "replace")
        return True

    def wait_for(self, pattern: "re.Pattern[str]", timeout: float) -> "re.Match[str]":
        """the first match after the last one waited for"""
        deadline = time.monotonic() + timeout
        while True:
            match = pattern.search(self.text, self.position)
            if match:
                self.position = match.end()
                return match
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError(f"the game's console did not print {pattern.pattern!r}")
            self.check_alive()
            self.receive(min(remaining, 1.0))

    def drain(self, seconds: float) -> str:
        """what the game writes within seconds"""
        start = len(self.text)
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            self.receive(deadline - time.monotonic())
        self.position = len(self.text)
        return self.text[start:]

    def ready(self, timeout: float = LOAD_TIMEOUT) -> None:
        """waits until the game has run every line sent before: the console
        runs them in order, between frames"""
        self.counter += 1
        token = f"render_test_ready_{self.counter}"
        self.send(f'(print "{token}")')
        # (the echo of the line has the token in quotes)
        self.wait_for(re.compile(rf'(?<![\w"]){token}(?![\w"])'), timeout)

    def run(self, line: str) -> None:
        self.send(line)
        self.ready()

    def wait_in_game(self, timeout: float = LOAD_TIMEOUT) -> int:
        """waits until a map has loaded and its game is under way (the
        console also runs while a map loads, behind the loading screen); its
        game time"""
        deadline = time.monotonic() + timeout
        while True:
            self.send("port_status")
            match = self.wait_for(re.compile(r"port_status: (in_game (\d+)|loading)"), timeout)
            if match.group(2):
                return int(match.group(2))
            if time.monotonic() > deadline:
                raise TimeoutError("the map did not finish loading")
            time.sleep(0.5)

    def capture(self, name: str, settle_frames: int) -> Path:
        """the frame saved after settle_frames more (port_screenshot), with
        no lines of the console on it"""
        # (nothing printed between the two: ready would print)
        self.send("cls")
        self.send(f"port_screenshot {name} {settle_frames}")
        match = self.wait_for(re.compile(r"captured ([^\r\n]+\.bmp)|capture failed ([^\r\n]+)|"
                                         r"port_screenshot: (?!saving )([^\r\n]+)"), CAPTURE_TIMEOUT)
        if not match.group(1):
            raise RuntimeError(match.group(0))
        return Path(match.group(1))


# ---------- scenes


class Scene:
    def __init__(self, values: dict):
        self.name: str = values["name"]
        self.map: str = values["map"]
        self.core: Optional[str] = values["core"]
        self.camera: Optional[str] = values["camera"]
        self.settings: Dict[str, str] = {key: str(value) for key, value in values["settings"].items()}
        self.commands: List[str] = list(values["commands"])
        self.settle_frames: int = int(values["settle_frames"])
        self.threshold: int = int(values["threshold"])
        self.tolerance: float = float(values["tolerance"])
        if not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.-]*", self.name):
            sys.exit(f"render_test: a scene's name is letters, digits, '-', '_' and '.': {self.name!r}")


def load_scenes(path: Path) -> List[Scene]:
    with open(path, "rb") as file:
        data = tomllib.load(file)
    defaults = dict(SCENE_DEFAULTS, **data.get("defaults", {}))
    scenes = [Scene(dict(defaults, **values)) for values in data.get("scene", [])]
    names = [scene.name for scene in scenes]
    for name in set(names):
        if names.count(name) > 1:
            sys.exit(f"render_test: two scenes are named {name}")
    return scenes


def draw_scene(scene: Scene, renderer: str, binary: Path, args: argparse.Namespace, run_dir: Path) -> Path:
    root = args.work / "data"
    camera = None
    if scene.camera:
        camera = CAMERAS / f"{scene.camera}.txt"
        if not camera.is_file():
            raise FileNotFoundError(f"no camera {camera}")
        # (d:\camera.txt: director.c's debug_camera_load reads only this one)
        shutil.copyfile(camera, root / "camera.txt")
    if scene.core and not (root / "core" / scene.core).is_file():
        raise FileNotFoundError(f"no game state {root / 'core' / scene.core} (render_test.py author {scene.map})")

    init = []
    if scene.core:
        init.append(f'core_load_name_at_startup "{scene.core}"')
    init.append(f"map_name {map_path(scene.map)}")
    settings = dict(scene.settings, HALO_RENDERER=renderer, HALO_SCREENSHOT_DIR=str(run_dir.resolve()))
    settings.update(args.overrides)
    game = Game(binary, root, args.work / "save", init, settings, run_dir / f"{scene.name}.log", args.port)
    try:
        console = game.console
        # (the lines the harness prints are not drawn: console.capture also
        # clears the console)
        console.run("terminal_render 0")
        console.wait_in_game()
        if not scene.core:
            # (not the same tick from run to run: only for a first look)
            console.run("game_speed 0")
        if camera:
            console.run("debug_camera_load")
        for command in scene.commands:
            console.run(command)
        return console.capture(scene.name, scene.settle_frames)
    finally:
        game.stop()


# ---------- comparing


def compare_pictures(a: Path, b: Path, threshold: int, diff_path: Path) -> dict:
    """how much of b matches a; the differences drawn into diff_path"""
    import numpy
    from PIL import Image

    first = numpy.asarray(Image.open(a).convert("RGB"), dtype=numpy.int16)
    second = numpy.asarray(Image.open(b).convert("RGB"), dtype=numpy.int16)
    if first.shape != second.shape:
        return {"error": f"sizes differ: {first.shape[1]}x{first.shape[0]} and {second.shape[1]}x{second.shape[0]}"}
    difference = numpy.abs(first - second).max(axis=2)
    matching = float((difference <= threshold).mean())
    # the first picture, darkened, with the pixels that differ in red, as
    # bright as they differ (a difference of 32 or more at full brightness)
    picture = (first // 4).astype(numpy.uint8)
    differing = difference > threshold
    picture[differing] = 0
    picture[..., 0][differing] = numpy.clip(64 + difference[differing] * 6, 0, 255).astype(numpy.uint8)
    diff_path.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(picture).save(diff_path)
    return {"matching": matching, "largest": int(difference.max()), "mean": float(difference.mean())}


def write_report(path: Path, runs: List[str], rows: List[dict]) -> None:
    def image(file: Optional[Path]) -> str:
        if not file:
            return "<td class=missing>none</td>"
        relative = os.path.relpath(file, path.parent).replace(os.sep, "/")
        return f'<td><a href="{html.escape(relative)}"><img src="{html.escape(relative)}" loading=lazy></a></td>'

    lines = [
        "<!doctype html><meta charset=utf-8><title>Render test</title>",
        "<style>body{font:14px system-ui;margin:16px;background:#fff;color:#111}"
        "table{border-collapse:collapse}td,th{border:1px solid #ccc;padding:4px;vertical-align:top}"
        "img{width:320px;display:block}.fail{background:#fdd}.pass{background:#dfd}.missing{color:#888}"
        "@media(prefers-color-scheme:dark){body{background:#111;color:#eee}td,th{border-color:#444}"
        ".fail{background:#522}.pass{background:#253}}</style>",
        f"<h1>Render test: {html.escape(' against '.join(runs))}</h1>",
        f"<p>{sum(row['passed'] for row in rows)} of {len(rows)} passed.</p>",
        "<table><tr><th>Scene</th><th>Result</th>" + "".join(f"<th>{html.escape(run)}</th>" for run in runs) +
        "<th>Differences</th></tr>",
    ]
    for row in rows:
        lines.append(f"<tr class={'pass' if row['passed'] else 'fail'}><td>{html.escape(row['name'])}</td>"
                     f"<td>{html.escape(row['summary'])}</td>" +
                     "".join(image(file) for file in row["pictures"]) + image(row.get("diff")) + "</tr>")
    lines.append("</table>")
    path.write_text("\n".join(lines), encoding="utf-8")


def compare_runs(run_dirs: List[Path], names: List[str], scenes: Dict[str, Scene], report: Path) -> bool:
    rows = []
    for name in names:
        scene = scenes.get(name)
        threshold = scene.threshold if scene else SCENE_DEFAULTS["threshold"]
        tolerance = scene.tolerance if scene else SCENE_DEFAULTS["tolerance"]
        pictures = [run_dir / f"{name}.bmp" for run_dir in run_dirs]
        present = [picture if picture.is_file() else None for picture in pictures]
        row = {"name": name, "pictures": present, "passed": False}
        if not all(present):
            row["summary"] = "not drawn by " + ", ".join(run_dir.name for run_dir, picture
                                                         in zip(run_dirs, present) if not picture)
        else:
            summaries = []
            passed = True
            for index, other in enumerate(pictures[1:], 1):
                diff = report.parent / "diff" / f"{name}.{run_dirs[index].name}.png"
                result = compare_pictures(pictures[0], other, threshold, diff)
                if "error" in result:
                    summaries.append(result["error"])
                    passed = False
                    continue
                row.setdefault("diff", diff)
                ok = result["matching"] >= tolerance
                passed = passed and ok
                summaries.append(f"{run_dirs[index].name}: {result['matching'] * 100:.3f}% within {threshold}, "
                                 f"largest {result['largest']}, mean {result['mean']:.3f}"
                                 f"{'' if ok else f' (needs {tolerance * 100:.1f}%)'}")
            row["passed"] = passed
            row["summary"] = "; ".join(summaries)
        print(f"{'pass' if row['passed'] else 'FAIL'} {name}: {row['summary']}", flush=True)
        rows.append(row)
    write_report(report, [run_dir.name for run_dir in run_dirs], rows)
    print(f"render_test: {sum(row['passed'] for row in rows)} of {len(rows)} passed; {report}")
    return all(row["passed"] for row in rows)


# ---------- commands


def command_run(args: argparse.Namespace) -> int:
    scenes = load_scenes(args.scenes)
    if args.scene:
        unknown = set(args.scene) - {scene.name for scene in scenes}
        if unknown:
            sys.exit(f"render_test: no such scenes: {', '.join(sorted(unknown))}")
        scenes = [scene for scene in scenes if scene.name in args.scene]
    if not scenes:
        sys.exit(f"render_test: no scenes in {args.scenes}")
    renderers = [renderer.strip() for renderer in args.runs.split(",") if renderer.strip()]
    if len(renderers) < 2:
        sys.exit("render_test: --runs names two renderers or more (gl,gl for the noise floor)")
    # (each run's game: --binaries, else --binary for all of them)
    binaries = [Path(path.strip()) for path in args.binaries.split(",")] if args.binaries else []
    if binaries and len(binaries) != len(renderers):
        sys.exit("render_test: --binaries names one game for each of --runs")
    binaries = binaries or [args.binary] * len(renderers)
    for binary in binaries:
        if not binary.is_file():
            sys.exit(f"render_test: no game at {binary} (--binary, --binaries)")
    ensure_data_root(args.assets, args.work / "data")

    run_dirs = []
    for index, (renderer, binary) in enumerate(zip(renderers, binaries), 1):
        run_dir = args.work / f"{index}-{renderer}"
        if run_dir.exists():
            shutil.rmtree(run_dir)
        run_dir.mkdir(parents=True)
        run_dirs.append(run_dir)
        for scene in scenes:
            print(f"render_test: {renderer}: {scene.name}", flush=True)
            try:
                draw_scene(scene, renderer, binary, args, run_dir)
            except (OSError, RuntimeError, TimeoutError, ValueError) as error:
                print(f"render_test: {renderer}: {scene.name}: {error} (log: {run_dir / (scene.name + '.log')})",
                      flush=True)

    passed = compare_runs(run_dirs, [scene.name for scene in scenes], {scene.name: scene for scene in scenes},
                          args.work / "report.html")
    return 0 if passed else 1


def command_compare(args: argparse.Namespace) -> int:
    if len(args.directories) < 2:
        sys.exit("render_test: compare takes two folders of pictures or more; the first is the reference")
    scenes ={scene.name: scene for scene in load_scenes(args.scenes)} if args.scenes.is_file() else {}
    names = sorted({path.stem for directory in args.directories for path in directory.glob("*.bmp")})
    report = args.report or args.directories[0].parent / "report.html"
    return 0 if compare_runs(args.directories, names, scenes, report) else 1


def command_author(args: argparse.Namespace) -> int:
    """a console to the game, with ':scene NAME' to save a scene"""
    if not args.binary.is_file():
        sys.exit(f"render_test: no game at {args.binary} (--binary)")
    if args.scene and not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.-]*", args.scene):
        sys.exit("render_test: a scene's name is letters, digits, '-', '_' and '.'")
    root = args.work / "data"
    ensure_data_root(args.assets, root)
    # (real time as it goes: the map runs by itself while the scene is made)
    settings = {"HALO_RENDERER": args.renderer, "HALO_SCREENSHOT_DIR": str((args.work / "author").resolve()),
                "HALO_FRAME_TIME": "0"}
    (args.work / "author").mkdir(parents=True, exist_ok=True)
    game = Game(args.binary, root, args.work / "save", [f"map_name {map_path(args.map)}"], settings,
                args.work / "author" / "game.log", args.port)
    print("render_test: the game's console. Play to the place, then ':scene NAME' saves a scene there "
          "(the game frozen, its state, and the view); ':quit' stops the game.", flush=True)
    try:
        console = game.console
        start = console.wait_in_game()
        if args.scene:
            # no one plays: the map runs by itself for a while
            time.sleep(args.after)
            now = console.wait_in_game()
            print(f"render_test: game time {start} to {now}", flush=True)
            return 0 if save_scene(console, root, args.map, args.scene) else 1
        print(console.drain(0.5), end="", flush=True)
        while True:
            try:
                line = input("> ").strip()
            except EOFError:
                break
            if line in (":quit", ":q"):
                break
            if line.startswith(":scene"):
                parts = line.split()
                if len(parts) != 2 or not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.-]*", parts[1]):
                    print("usage: :scene NAME (letters, digits, '-', '_' and '.')")
                    continue
                save_scene(console, root, args.map, parts[1])
                continue
            if line:
                console.send(line)
            print(console.drain(1.0), end="", flush=True)
    finally:
        game.stop()
    return 0


def save_scene(console: Console, root: Path, map_name: str, name: str) -> bool:
    console.run("game_speed 0")
    console.run(f'core_save_name "{name}"')
    camera_file = root / "camera.txt"
    if camera_file.exists():
        camera_file.unlink()
    console.run("debug_camera_save")
    if not (root / "core" / name).is_file():
        print(f"render_test: the game did not save its state to {root / 'core' / name}")
        return False
    if not camera_file.is_file():
        print(f"render_test: the game did not save the camera to {camera_file}")
        return False
    view = f"{map_name.split(chr(92))[-1]}/{name}"
    destination = CAMERAS / f"{view}.txt"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(camera_file, destination)
    print(f"render_test: saved. For tools/render_scenes.toml (leave out camera for the player's own view):\n\n"
          f"[[scene]]\nname = \"{name}\"\nmap = \"{map_name}\"\ncore = \"{name}\"\ncamera = \"{view}\"\n",
          flush=True)
    # (the game stays frozen: game_speed 1 lets it go on)
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY)
    parser.add_argument("--assets", type=Path, default=DEFAULT_ASSETS, help="the game data: the folder with maps/")
    parser.add_argument("--work", type=Path, default=DEFAULT_WORK)
    parser.add_argument("--scenes", type=Path, default=SCENES_FILE)
    parser.add_argument("--port", type=int, default=TELNET_PORT)
    commands = parser.add_subparsers(dest="command", required=True)

    run = commands.add_parser("run", help="draw the scenes with each renderer and compare")
    run.add_argument("--runs", default="gl,gl", help="renderers, comma-separated; the first is the reference")
    run.add_argument("--binaries", help="each run's game, comma-separated (else --binary for all)")
    run.add_argument("--scene", action="append", help="only this scene (again for more)")
    run.add_argument("--set", action="append", default=[], metavar="NAME=VALUE",
                     help="a HALO_* environment variable for every run (again for more)")
    run.set_defaults(handler=command_run)

    author = commands.add_parser("author", help="make scenes on a map")
    author.add_argument("map", help="b30, or a path such as levels\\test\\bloodgulch\\bloodgulch")
    author.add_argument("--renderer", default="gl")
    author.add_argument("--scene", help="no console: save this scene once the map has run for --after seconds")
    author.add_argument("--after", type=float, default=30.0)
    author.set_defaults(handler=command_author)

    compare = commands.add_parser("compare", help="compare folders of pictures again")
    compare.add_argument("directories", type=Path, nargs="+")
    compare.add_argument("--report", type=Path)
    compare.set_defaults(handler=command_compare)

    args = parser.parse_args()
    args.overrides = dict(setting.split("=", 1) for setting in getattr(args, "set", []))
    return args.handler(args)


if __name__ == "__main__":
    sys.exit(main())
