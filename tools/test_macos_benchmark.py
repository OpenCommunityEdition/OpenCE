"""The 4K benchmark: OpenCE.app plays bloodgulch for 60 s at 3840x2160 off
the screen, logging each frame it presents (HALO_HOST_FRAME_LOG,
port/macos/host/host_frames.c), and writes the frame times and the VM's exits
per frame to build/macos/benchmark.txt (docs/macos_host.md, "The
benchmark"). The game runs on the real clock, not the frame clock, with the
debug camera of tools/test_macos_frames.py.

    python -m pytest -q tools/test_macos_benchmark.py -s

Needs `ninja macos macos_app` in `nix develop`, and the maps in assets/maps.
"""

import csv
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import macos_guest_image  # noqa: E402
from tools.test_macos_frames import APP, BLOODGULCH_CAMERA, MAPS  # noqa: E402

SIZE = (3840, 2160)
SECONDS = 60
REPORT = ROOT / "build/macos/benchmark.txt"
# the target: 120 frames a second
TARGET_MILLISECONDS = 1000 / 120

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(not macos_guest_image.available() or shutil.which("ninja") is None,
                       reason="needs `ninja macos` in nix develop"),
    pytest.mark.skipif(not APP.is_file(), reason="needs `ninja macos_app`"),
    pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game's maps in assets/maps"),
]


def percentile(values, fraction):
    """the value `fraction` of the way up the sorted `values` (nearest rank)"""
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, int(len(ordered) * fraction))]


def read_frames(path):
    """the log's rows, each checked: (frame, milliseconds, gpu_milliseconds, exits)"""
    with open(path, newline="") as log:
        reader = csv.reader(log)
        assert next(reader) == ["frame", "milliseconds", "gpu_milliseconds", "exits"]
        rows = [(int(frame), float(milliseconds), float(gpu), int(exits))
                for frame, milliseconds, gpu, exits in reader]
    assert [row[0] for row in rows] == list(range(1, len(rows) + 1)), "the frames are not numbered in order"
    assert all(row[1] >= 0 and row[2] >= 0 and row[3] >= 0 for row in rows)
    return rows


def in_game(rows):
    """the frames after the map has loaded: those after the longest wait
    between two frames, which is the load's"""
    longest = max(range(len(rows)), key=lambda index: rows[index][1])
    return rows[longest + 1:]


def summary(rows):
    milliseconds = [row[1] for row in rows]
    gpu = [row[2] for row in rows]
    exits = [row[3] for row in rows]
    return {
        "frames": len(rows),
        "seconds": sum(milliseconds) / 1000,
        "p50": percentile(milliseconds, 0.50),
        "p99": percentile(milliseconds, 0.99),
        "max": max(milliseconds),
        "gpu_p50": percentile(gpu, 0.50),
        "gpu_p99": percentile(gpu, 0.99),
        "exits_per_frame": sum(exits) / len(exits),
    }


def test_the_frame_log_is_summarised(tmp_path):
    # (the summary's arithmetic, without the game)
    log = tmp_path / "frames.csv"
    log.write_text("frame,milliseconds,gpu_milliseconds,exits\n"
                   "1,5.000,1.000,10\n2,500.000,2.000,900\n3,8.000,3.000,100\n4,9.000,4.000,120\n")
    rows = in_game(read_frames(log))
    assert [row[0] for row in rows] == [3, 4]
    result = summary(rows)
    assert result["frames"] == 2 and result["p50"] == 9.0 and result["max"] == 9.0
    assert result["exits_per_frame"] == 110


def test_bloodgulch_at_4k_is_measured(tmp_path):
    (tmp_path / "home").mkdir()
    root = tmp_path / "root"
    root.mkdir()
    (root / "maps").symlink_to(MAPS)
    (root / "camera.txt").write_text(BLOODGULCH_CAMERA)
    log = tmp_path / "frames.csv"
    # (drawn off the screen at 4K: the capture names a frame never reached)
    environment = dict(os.environ, HOME=str(tmp_path / "home"), HALO_DATA_ROOT=str(root),
                       HALO_HOST_CAPTURE=str(tmp_path / "never.png"), HALO_HOST_CAPTURE_FRAME="1000000000",
                       HALO_HOST_CAPTURE_SIZE="%dx%d" % SIZE, HALO_EXIT_AFTER=str(SECONDS),
                       HALO_HOST_FRAME_LOG=str(log), HALO_NETWORK_TEST="host:bloodgulch",
                       HALO_NETWORK_TEST_CAMERA="1", HALO_NET_ONLINE="false", HALO_NET_JOIN_FROM_CLIPBOARD="false")
    environment.pop("HALO_HOST_GUEST_IMAGE", None)
    environment.pop("HALO_FRAME_CLOCK", None)
    run = subprocess.run([str(APP)], cwd=tmp_path, env=environment, capture_output=True, text=True,
                         timeout=SECONDS + 60)
    assert run.returncode == 0, run.stdout[-2000:] + run.stderr[-2000:]
    rows = in_game(read_frames(log))
    result = summary(rows)
    # (the map loaded early in the run, and the game kept drawing)
    assert result["seconds"] > SECONDS / 2, result
    report = ("bloodgulch at %dx%d, %d frames in %.1f s after the map loaded\n"
              "frame time: p50 %.2f ms, p99 %.2f ms, max %.2f ms (target p99 <= %.2f ms)\n"
              "GPU time: p50 %.2f ms, p99 %.2f ms\n"
              "VM exits: %.0f a frame\n") % (
        SIZE[0], SIZE[1], result["frames"], result["seconds"], result["p50"], result["p99"], result["max"],
        TARGET_MILLISECONDS, result["gpu_p50"], result["gpu_p99"], result["exits_per_frame"])
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(report)
    print("\n" + report)
