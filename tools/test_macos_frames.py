"""Golden frames: what OpenCE.app's Metal renderer draws, compared with
approved references (docs/macos_host.md, "Frame captures").

Each case runs a guest program in the app in the game's place
(HALO_HOST_GUEST_IMAGE), or the game itself with the maps of assets/maps
and its clock stepped a frame at a time (HALO_FRAME_CLOCK), with its window
drawn off the screen and one frame written to a PNG (HALO_HOST_CAPTURE);
tools/frame_compare.py compares it with port/macos/tests/frames/<case>.png. No window is shown. A frame that does
not match leaves its capture and its difference image in build/macos/frames.

To approve a new or changed frame, look at it, then run

    HALO_APPROVE_FRAMES=1 python -m pytest -q tools/test_macos_frames.py

which writes the references (the cases are then skipped), and commit them.
Needs `ninja macos macos_app` in `nix develop`.
"""

import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import frame_compare, macos_guest_image  # noqa: E402

APP = ROOT / "build/macos/OpenCE.app/Contents/MacOS/OpenCE"
REFERENCES = ROOT / "port/macos/tests/frames"
FAILED = ROOT / "build/macos/frames"
SIZE = (320, 240)
MAPS = ROOT / "assets/maps"
GAME_SIZE = (640, 480)

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(not macos_guest_image.available() or shutil.which("ninja") is None,
                       reason="needs `ninja macos` in nix develop"),
    pytest.mark.skipif(not APP.is_file(), reason="needs `ninja macos_app`"),
]


@pytest.fixture(scope="module")
def vm_render(tmp_path_factory):
    work = tmp_path_factory.mktemp("vm_render")
    return macos_guest_image.build_guest_image(
        [ROOT / "port/macos/tests/vm_render.c", ROOT / "port/macos/guest/opence_render_writer.c"], work,
        "vm_render", [f"-I{ROOT / 'port/macos/include'}"])


def capture(image, case, tmp_path, frame=1):
    """the frame the app drew of the guest's `case`"""
    (tmp_path / "home").mkdir()
    path = tmp_path / f"{case}.png"
    environment = dict(os.environ, HOME=str(tmp_path / "home"), HALO_DATA_ROOT=str(tmp_path),
                       HALO_HOST_GUEST_IMAGE=str(image), HALO_HOST_CAPTURE=str(path),
                       HALO_HOST_CAPTURE_FRAME=str(frame), HALO_HOST_CAPTURE_SIZE="%dx%d" % SIZE)
    run = subprocess.run([str(APP), case], cwd=tmp_path, env=environment, capture_output=True, text=True,
                         timeout=60)
    assert run.returncode == 0 and run.stdout.strip().endswith("ok"), run.stdout[-2000:] + run.stderr[-2000:]
    assert path.is_file(), "no frame was captured: " + run.stderr[-2000:]
    return frame_compare.read_png(path)


def capture_game(tmp_path, frame, settings=None, seconds=30, data_root=None):
    """the frame the app drew of the game at its `frame`th PRESENT"""
    (tmp_path / "home").mkdir()
    path = tmp_path / "game.png"
    environment = dict(os.environ, HOME=str(tmp_path / "home"), HALO_DATA_ROOT=str(data_root or ROOT / "assets"),
                       HALO_FRAME_CLOCK="1", HALO_HOST_CAPTURE=str(path), HALO_HOST_CAPTURE_FRAME=str(frame),
                       HALO_HOST_CAPTURE_SIZE="%dx%d" % GAME_SIZE, HALO_EXIT_AFTER=str(seconds),
                       **(settings or {}))
    environment.pop("HALO_HOST_GUEST_IMAGE", None)
    run = subprocess.run([str(APP)], cwd=tmp_path, env=environment, capture_output=True, text=True,
                         timeout=seconds + 60)
    assert path.is_file(), "no frame was captured: " + run.stdout[-2000:] + run.stderr[-2000:]
    return frame_compare.read_png(path)


def check(frame, name, tolerance=frame_compare.Tolerance()):
    """compares `frame` with its reference, or approves it"""
    reference = REFERENCES / f"{name}.png"
    if os.environ.get("HALO_APPROVE_FRAMES") == "1":
        REFERENCES.mkdir(parents=True, exist_ok=True)
        frame_compare.write_png(reference, frame)
        pytest.skip(f"approved {reference.relative_to(ROOT)}")
    FAILED.mkdir(parents=True, exist_ok=True)
    assert reference.is_file(), (f"{name} has no reference: look at the frame, then approve it with "
                                 f"HALO_APPROVE_FRAMES=1 (this file's docstring)")
    result = frame_compare.compare(frame, frame_compare.read_png(reference), tolerance)
    if not result.matches:
        frame_compare.write_png(FAILED / f"{name}.png", frame)
        if result.same_size:
            frame_compare.write_png(FAILED / f"{name}-difference.png", result.difference())
    assert result.matches, f"{name}: {result.describe()} (the frame and its difference are in {FAILED})"


def test_the_scene_draws_as_approved(vm_render, tmp_path):
    # a clear, a triangle of three colours and a quad blended over it
    frame = capture(vm_render, "scene", tmp_path)
    assert (frame.width, frame.height) == SIZE
    # (the capture is the window's, not empty: the clear's dark blue in a
    # corner, and the triangle's three colours near its corners, before the
    # reference is consulted at all)
    assert frame.pixel(2, 2) == pytest.approx((26, 26, 51, 255), abs=1)
    width, height = SIZE
    red, green, blue, _ = frame.pixel(int(width * 0.15), int(height * 0.85))
    assert red > 200 and green < 60 and blue < 60
    red, green, blue, _ = frame.pixel(width // 2, int(height * 0.15))
    assert blue > 200 and red < 60 and green < 60
    check(frame, "scene")


@pytest.mark.skipif(not (MAPS / "ui.map").is_file(), reason="needs the game's maps in assets/maps")
def test_the_main_menu_draws_as_approved(tmp_path):
    # ten seconds in: the menu's background, the logo and its items
    frame = capture_game(tmp_path, 600)
    assert (frame.width, frame.height) == GAME_SIZE
    # (not black: the selected item, CAMPAIGN, is drawn light)
    assert max(frame.pixel(x, 262)[0] for x in range(250, 400)) > 200
    check(frame, "main_menu")


# the debug camera of camera.txt (debug.network_test_camera): over the
# grass from the red base towards the blue, the cliffs on the right
BLOODGULCH_CAMERA = "95.0 -150.0 4.0\n-0.6 0.8 -0.15\n0 0 1\n1.22\n"


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game's maps in assets/maps")
def test_bloodgulch_draws_as_approved(tmp_path):
    # a network test game hosted on bloodgulch with no other machine (none
    # found or asked for: online play off), seen from a fixed camera; a data
    # root of its own holds the maps and the camera (and no one's settings)
    root = tmp_path / "root"
    root.mkdir()
    (root / "maps").symlink_to(MAPS)
    (root / "camera.txt").write_text(BLOODGULCH_CAMERA)
    frame = capture_game(tmp_path, 1500, {"HALO_NETWORK_TEST": "host:bloodgulch", "HALO_NETWORK_TEST_CAMERA": "1",
                                          "HALO_NET_ONLINE": "false", "HALO_NET_JOIN_FROM_CLIPBOARD": "false"},
                         seconds=75, data_root=root)
    assert (frame.width, frame.height) == GAME_SIZE
    # (the sky blue at the top left, the grass green below the middle)
    red, green, blue, _ = frame.pixel(20, 10)
    assert blue > red and blue > 150
    red, green, blue, _ = frame.pixel(150, 320)
    assert green > red and green > blue
    # (how long the map takes to load moves the game's time at the frame
    # captured, so the clouds and the swaying trees far off differ run to
    # run: under 1% of the frame; the ground and the cliffs do not)
    check(frame, "bloodgulch", frame_compare.Tolerance(fraction=0.02, largest=255))
