"""The new engine's --headless mode: the app runs with no window, its
control socket made for the user alone, answers the world's state and a
shell command, draws a frame offscreen into a PNG when asked, and quits
when told to. Run inside nix develop with assets/maps/bloodgulch.map."""

from __future__ import annotations

import json
import os
import shutil
import socket
import stat
import subprocess
import tempfile
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
MAP = ROOT / "assets" / "maps" / "bloodgulch.map"
OUT = ROOT / "build" / "engine"


@pytest.fixture(scope="module")
def executable() -> Path:
    if shutil.which("cargo") is None or not Path("/usr/bin/xcrun").exists():
        pytest.skip("needs nix develop's cargo and Xcode's Swift")
    if not MAP.exists():
        pytest.skip("no assets/maps/bloodgulch.map")
    subprocess.run(["python3", str(ROOT / "tools" / "engine_build.py"), "--out", str(OUT)], check=True)
    return OUT / "OpenCE Engine.app" / "Contents" / "MacOS" / "OpenCEEngine"


class Connection:
    def __init__(self, path: str) -> None:
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.settimeout(60)
        self.socket.connect(path)
        self.lines = self.socket.makefile("r", encoding="utf-8")

    def send(self, request: object) -> dict:
        self.socket.sendall((json.dumps(request) + "\n").encode())
        return json.loads(self.lines.readline())


def test_headless_answers_and_draws(executable: Path) -> None:
    folder = Path(tempfile.mkdtemp(prefix="oce"))
    path = folder / "run" / "c.sock"
    cache = OUT / "cache"
    cache.mkdir(parents=True, exist_ok=True)
    process = subprocess.Popen(
        [
            str(executable),
            "--headless",
            "--socket",
            str(path),
            "--map",
            str(MAP),
            "--cache",
            str(cache),
            "--tick-rate",
            "60",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    try:
        assert process.stdout is not None
        line = process.stdout.readline()
        assert line.startswith("control socket: "), line
        assert stat.S_IMODE(os.stat(path).st_mode) == 0o600
        assert stat.S_IMODE(os.stat(path.parent).st_mode) == 0o700

        connection = Connection(str(path))
        state = connection.send({"request": "state"})
        assert state["ok"] and state["playing"], state
        assert state["level"], state
        assert connection.send("time 2")["ok"]
        png = folder / "frame.png"
        drawn = connection.send({"request": "frame", "path": str(png), "width": 320, "height": 180})
        assert drawn["ok"], drawn
        header = png.read_bytes()[:24]
        assert header[:8] == b"\x89PNG\r\n\x1a\n"
        assert (int.from_bytes(header[16:20], "big"), int.from_bytes(header[20:24], "big")) == (320, 180)
        assert connection.send("quit")["action"] == "quit"
        assert process.wait(timeout=30) == 0
        assert not path.exists()
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
        shutil.rmtree(folder, ignore_errors=True)
