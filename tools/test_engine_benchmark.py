"""The new engine's benchmark: bloodgulch flown offscreen along the
benchmark's path at 3840x2160 for 60 seconds, with temporal anti-aliasing
at full scale, its frame times' p50 and p99 reported, and p99 held to
8.33 ms (120 frames a second). No window is opened. Run inside nix develop
with assets/maps/bloodgulch.map; OPENCE_BENCHMARK_SECONDS shortens it."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
MAP = ROOT / "assets" / "maps" / "bloodgulch.map"
OUT = ROOT / "build" / "engine"
P99_MS = 8.33


@pytest.fixture(scope="module")
def executable() -> Path:
    if shutil.which("cargo") is None or not Path("/usr/bin/xcrun").exists():
        pytest.skip("needs nix develop's cargo and Xcode's Swift")
    if not MAP.exists():
        pytest.skip("no assets/maps/bloodgulch.map")
    subprocess.run(["python3", str(ROOT / "tools" / "engine_build.py"), "--out", str(OUT)], check=True)
    return OUT / "OpenCE Engine.app" / "Contents" / "MacOS" / "OpenCEEngine"


def test_bloodgulch_at_4k_holds_120_frames_a_second(executable: Path) -> None:
    seconds = float(os.environ.get("OPENCE_BENCHMARK_SECONDS", "60"))
    cache = OUT / "cache"
    cache.mkdir(parents=True, exist_ok=True)
    result = subprocess.run(
        [
            str(executable), "--benchmark", str(seconds), "--map", str(MAP), "--size", "3840x2160",
            "--anti-aliasing", "temporal", "--render-scale", "1", "--cache", str(cache),
        ],
        capture_output=True, text=True, check=True, timeout=seconds + 300,
    )
    report = json.loads(result.stdout.strip().splitlines()[-1])
    print(f"\nbenchmark: {report}")
    (OUT / "benchmark.json").write_text(json.dumps(report, indent=2) + "\n")
    assert report["frames"] > seconds * 60
    assert report["p99_ms"] <= P99_MS, f"p99 {report['p99_ms']} ms is above {P99_MS} ms"
