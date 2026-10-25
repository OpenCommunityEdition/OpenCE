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
import re
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
    pytest.mark.skipif(
        not macos_guest_image.available() or shutil.which("ninja") is None, reason="needs `ninja macos` in nix develop"
    ),
    pytest.mark.skipif(not APP.is_file(), reason="needs `ninja macos_app`"),
]


@pytest.fixture(scope="module")
def vm_render(tmp_path_factory):
    work = tmp_path_factory.mktemp("vm_render")
    return macos_guest_image.build_guest_image(
        [ROOT / "port/macos/tests/vm_render.c", ROOT / "port/macos/guest/opence_render_writer.c"],
        work,
        "vm_render",
        [f"-I{ROOT / 'port/macos/include'}"],
    )


def capture(image, case, tmp_path, frame=1):
    """the frame the app drew of the guest's `case`"""
    (tmp_path / "home").mkdir()
    path = tmp_path / f"{case}.png"
    environment = dict(
        os.environ,
        HOME=str(tmp_path / "home"),
        HALO_DATA_ROOT=str(tmp_path),
        HALO_HOST_GUEST_IMAGE=str(image),
        HALO_HOST_CAPTURE=str(path),
        HALO_HOST_CAPTURE_FRAME=str(frame),
        HALO_HOST_CAPTURE_SIZE="%dx%d" % SIZE,
    )
    run = subprocess.run([str(APP), case], cwd=tmp_path, env=environment, capture_output=True, text=True, timeout=60)
    assert run.returncode == 0 and run.stdout.strip().endswith("ok"), run.stdout[-2000:] + run.stderr[-2000:]
    assert path.is_file(), "no frame was captured: " + run.stderr[-2000:]
    return frame_compare.read_png(path)


def run_game(tmp_path, frame, settings=None, seconds=30, data_root=None, size=GAME_SIZE):
    """runs the app offscreen for `seconds`, capturing its `frame`th PRESENT
    at `size` to game.png; returns the finished process"""
    (tmp_path / "home").mkdir()
    environment = dict(
        os.environ,
        HOME=str(tmp_path / "home"),
        HALO_DATA_ROOT=str(data_root or ROOT / "assets"),
        HALO_FRAME_CLOCK="1",
        HALO_HOST_CAPTURE=str(tmp_path / "game.png"),
        HALO_HOST_CAPTURE_FRAME=str(frame),
        HALO_HOST_CAPTURE_SIZE="%dx%d" % size,
        HALO_EXIT_AFTER=str(seconds),
        **(settings or {}),
    )
    environment.pop("HALO_HOST_GUEST_IMAGE", None)
    return subprocess.run(
        [str(APP)], cwd=tmp_path, env=environment, capture_output=True, text=True, timeout=seconds + 60
    )


def capture_game(tmp_path, frame, settings=None, seconds=30, data_root=None, size=GAME_SIZE):
    """the frame the app drew of the game at its `frame`th PRESENT, at `size`"""
    run = run_game(tmp_path, frame, settings, seconds, data_root, size)
    path = tmp_path / "game.png"
    assert path.is_file(), "no frame was captured: " + run.stdout[-2000:] + run.stderr[-2000:]
    return frame_compare.read_png(path)


def check(frame, name, tolerance=frame_compare.Tolerance(), other=None):
    """compares `frame` with its reference, or approves it; a scene that ends
    one of two ways has the other's reference as `other`, which matches as
    well (approved by hand: copied from a failed run's frame)"""
    reference = REFERENCES / f"{name}.png"
    if other and (REFERENCES / f"{other}.png").is_file() and os.environ.get("HALO_APPROVE_FRAMES") != "1":
        if frame_compare.compare(frame, frame_compare.read_png(REFERENCES / f"{other}.png"), tolerance).matches:
            return
    if os.environ.get("HALO_APPROVE_FRAMES") == "1":
        REFERENCES.mkdir(parents=True, exist_ok=True)
        frame_compare.write_png(reference, frame)
        pytest.skip(f"approved {reference.relative_to(ROOT)}")
    FAILED.mkdir(parents=True, exist_ok=True)
    assert reference.is_file(), (
        f"{name} has no reference: look at the frame, then approve it with "
        f"HALO_APPROVE_FRAMES=1 (this file's docstring)"
    )
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


@pytest.mark.skipif(not (MAPS / "ui.map").is_file(), reason="needs the game's maps in assets/maps")
def test_the_frame_time_draws_when_on(tmp_path):
    # display.frame_time: its text at the top right of the title-safe area;
    # the frame clock steps a 60th of a second a frame, so it reads
    # "16.67 ms  60 fps" every run
    frame = capture_game(tmp_path, 600, {"HALO_FRAME_TIME": "true"})
    reference = frame_compare.read_png(REFERENCES / "main_menu.png")
    corner = [(x, y) for y in range(30, 60) for x in range(440, 600)]
    text = [point for point in corner if min(frame.pixel(*point)[:3]) > 200 and min(reference.pixel(*point)[:3]) < 150]
    assert len(text) > 50, "no white text in the corner"
    # (the background's ship moves by the real time when the machine is
    # busy: under 2% of the frame; the text does not)
    check(frame, "main_menu_frame_time", frame_compare.Tolerance(fraction=0.02, largest=255))


# the high-res HUD and text (hud_hires.c, text_hires.c) at the window's
# own size: twice the game's 640x480
NATIVE_SIZE = (1280, 960)


@pytest.mark.skipif(not (MAPS / "ui.map").is_file(), reason="needs the game's maps in assets/maps")
def test_the_main_menu_draws_at_native_resolution(tmp_path):
    # the menu's text, the version string of the high-res text's atlas among
    # it, drawn at 1280x960
    frame = capture_game(tmp_path, 600, size=NATIVE_SIZE)
    assert (frame.width, frame.height) == NATIVE_SIZE
    assert max(frame.pixel(x, 524)[0] for x in range(500, 800)) > 200
    # (the ring behind the logo turns by the real time, which a busy machine
    # moves on by a little at the frame captured: its edges, about 1% of the
    # frame; the text does not)
    check(frame, "main_menu_native", frame_compare.Tolerance(fraction=0.02, largest=255))


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game's maps in assets/maps")
def test_the_hud_draws_at_native_resolution(tmp_path):
    # the player's own view of a network test game on bloodgulch at
    # 1280x960: the shield and health meters, the motion sensor (which draws
    # into the target it samples) and the text, high-res
    root = tmp_path / "root"
    root.mkdir()
    (root / "maps").symlink_to(MAPS)
    frame = capture_game(
        tmp_path,
        1500,
        {"HALO_NETWORK_TEST": "host:bloodgulch", "HALO_NET_ONLINE": "false", "HALO_NET_JOIN_FROM_CLIPBOARD": "false"},
        seconds=75,
        data_root=root,
        size=NATIVE_SIZE,
    )
    assert (frame.width, frame.height) == NATIVE_SIZE
    # (the motion sensor is not the blank white of a target sampled while
    # drawn: its middle is darker than white)
    assert sum(frame.pixel(180, 800)[:3]) < 3 * 230
    # (the frame clock's fixed seed spawns the player in one place; the
    # plasma pistol's glow animates by the real time: 0.1% of the frame)
    check(frame, "hud_native", frame_compare.Tolerance(fraction=0.01, largest=255))


# the debug camera of camera.txt (debug.network_test_camera): over the
# grass from the red base towards the blue, the cliffs on the right
BLOODGULCH_CAMERA = "95.0 -150.0 4.0\n-0.6 0.8 -0.15\n0 0 1\n1.22\n"


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game's maps in assets/maps")
def test_the_game_answers_its_hottest_imports_itself(tmp_path):
    # Bloodgulch made 500,000 exits a second before the guest kept its own
    # clock (SDL_GetTicks from CNTVCT_EL0) and errno (with each hostposix_*
    # import's result): now it asks the host for neither but the clock's
    # calibration, and makes fewer than a tenth of the exits (about 24,000 a
    # second, nearly all host_syscall: futexes, sleeps and the like). Memory's
    # write tracking (port/macos/host/host_watch.c) is read from the table it
    # shares, never asked for, and pages are protected again only after the
    # game writes them
    run = run_game(
        tmp_path,
        1500,
        {
            "HALO_NETWORK_TEST": "host:bloodgulch",
            "HALO_NET_ONLINE": "false",
            "HALO_NET_JOIN_FROM_CLIPBOARD": "false",
            "HALO_HOST_IMPORT_COUNTS": "1",
        },
        seconds=20,
    )
    assert run.returncode == 0, run.stderr[-2000:]
    rate = re.search(r"halo_host: exits: \d+ in [\d.]+ s, (\d+)/s", run.stderr)
    assert rate, run.stderr[-2000:]
    counts = {name: int(count) for name, count in re.findall(r"halo_host: import (\w+): (\d+)", run.stderr)}
    assert int(rate.group(1)) < 500000 // 10, (rate.group(0), counts)
    assert counts.get("host_sdl_ticks", 0) == 1 and counts.get("host_errno", 0) == 0, counts
    assert counts.get("host_memory_watch_share", 0) == 1, counts
    assert not counts.get("host_memory_watch_generation") and not counts.get("host_memory_watch_serial"), counts
    # (contiguous memory freed or made again forgets its pages, about once a
    # frame of the 1500)
    assert counts.get("host_memory_watch_forget", 0) < 2 * 1500, counts
    assert counts.get("host_memory_watch_protect", 0) + counts.get("host_memory_watch_prepare_write", 0) < 1500, counts


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game's maps in assets/maps")
def test_bloodgulch_draws_as_approved(tmp_path):
    # a network test game hosted on bloodgulch with no other machine (none
    # found or asked for: online play off), seen from a fixed camera; a data
    # root of its own holds the maps and the camera (and no one's settings)
    root = tmp_path / "root"
    root.mkdir()
    (root / "maps").symlink_to(MAPS)
    (root / "camera.txt").write_text(BLOODGULCH_CAMERA)
    frame = capture_game(
        tmp_path,
        1500,
        {
            "HALO_NETWORK_TEST": "host:bloodgulch",
            "HALO_NETWORK_TEST_CAMERA": "1",
            "HALO_NET_ONLINE": "false",
            "HALO_NET_JOIN_FROM_CLIPBOARD": "false",
        },
        seconds=75,
        data_root=root,
    )
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


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game's maps in assets/maps")
@pytest.mark.parametrize("anti_aliasing", ["fxaa", "smaa"])
def test_bloodgulch_draws_anti_aliased(tmp_path, anti_aliasing):
    # the same view with display.anti_aliasing's passes (d3d8_metal_post.c),
    # each drawn over the frame before the menus and the HUD; the stock frame
    # above has the setting at its default, off
    root = tmp_path / "root"
    root.mkdir()
    (root / "maps").symlink_to(MAPS)
    (root / "camera.txt").write_text(BLOODGULCH_CAMERA)
    frame = capture_game(
        tmp_path,
        1500,
        {
            "HALO_NETWORK_TEST": "host:bloodgulch",
            "HALO_NETWORK_TEST_CAMERA": "1",
            "HALO_NET_ONLINE": "false",
            "HALO_NET_JOIN_FROM_CLIPBOARD": "false",
            "HALO_ANTI_ALIASING": anti_aliasing,
        },
        seconds=75,
        data_root=root,
    )
    assert (frame.width, frame.height) == GAME_SIZE
    red, green, blue, _ = frame.pixel(20, 10)
    assert blue > red and blue > 150
    red, green, blue, _ = frame.pixel(150, 320)
    assert green > red and green > blue
    check(frame, f"bloodgulch_{anti_aliasing}", frame_compare.Tolerance(fraction=0.02, largest=255))


# the ragdoll scene (debug.network_test_ragdoll): at 40 seconds of frames a
# frag grenade kills the biped six world units before the camera (a cyborg:
# bloodgulch has no Grunt), and eight seconds later a Warthog rolls down the
# slope into its body; the first camera watches the fall, the second, nearer
# and narrower, the shove
RAGDOLL_CAMERA = "95.0 -150.0 4.0\n-0.6 0.78 -0.36\n0 0 1\n0.95\n"
RAGDOLL_SHOVE_CAMERA = "95.0 -150.0 4.0\n-0.6 0.78 -0.233\n0 0 1\n0.5\n"


# The frames taken in the grenade's smoke. The smoke is drawn from the game's
# time, which steps with the real time while the map loads (debug.frame_clock's
# dark steps), so from run to run it lies a little thicker or thinner: up to a
# quarter of the pixels differ by more than 8, few by more than 64. These
# frames are checked only for differences above 64, at most 1536 of them (a
# half per cent); a body lying in another pose made more (1856 in
# ragdoll_shoved, 2418 in ragdoll_falling).
RAGDOLL_SMOKE_FRAMES = {"ragdoll_falling", "ragdoll_shoved", "dismemberment_flying"}

# The Warthog that shoves the body is the game's, rolling by the game's ticks,
# and comes to rest at one of two angles by how many ticks ran while the map
# loaded; the body lies against it. Each ending is approved: the two differ in
# 10381 pixels, and a run's frame matches one of them exactly.
RAGDOLL_OTHER_ENDINGS = {"ragdoll_shoved_at_rest": "ragdoll_shoved_at_rest_other"}


def ragdoll_tolerance(name):
    if name in RAGDOLL_SMOKE_FRAMES:
        return frame_compare.Tolerance(threshold=64, fraction=0.005, largest=255)
    return frame_compare.Tolerance(fraction=0.02, largest=255)


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game's maps in assets/maps")
@pytest.mark.parametrize(
    "name, frame, camera",
    [
        # (thrown from the grenade's smoke; lying limp on the slope; pushed before
        # the Warthog's bumper; at rest between its front wheels)
        ("ragdoll_falling", 2430, RAGDOLL_CAMERA),
        ("ragdoll_at_rest", 2850, RAGDOLL_CAMERA),
        ("ragdoll_shoved", 2930, RAGDOLL_SHOVE_CAMERA),
        ("ragdoll_shoved_at_rest", 3100, RAGDOLL_SHOVE_CAMERA),
    ],
)
def test_ragdolls_draw_as_approved(tmp_path, name, frame, camera):
    # game.ragdolls on; the frames above it, with the setting at its default,
    # are unchanged
    root = tmp_path / "root"
    root.mkdir()
    (root / "maps").symlink_to(MAPS)
    (root / "camera.txt").write_text(camera)
    frame = capture_game(
        tmp_path,
        frame,
        {
            "HALO_NETWORK_TEST": "host:bloodgulch",
            "HALO_NETWORK_TEST_CAMERA": "1",
            "HALO_NET_ONLINE": "false",
            "HALO_NET_JOIN_FROM_CLIPBOARD": "false",
            "HALO_RAGDOLLS": "true",
            "HALO_NETWORK_TEST_RAGDOLL": "40",
        },
        seconds=75,
        data_root=root,
    )
    assert (frame.width, frame.height) == GAME_SIZE
    # (the scene is keyed to frames shown, the physics stepped with the
    # game's ticks; the smoke and the trees far off move by the game's time)
    check(frame, name, ragdoll_tolerance(name), RAGDOLL_OTHER_ENDINGS.get(name))


# the same scene with game.dismemberment on: nearer, the grenade tears a
# forearm and a calf from the cyborg (port/assets/ragdolls/ragdolls.txt), the
# torso flung up the slope and the limbs left by the blast, each piece drawn
# with its own bones and its stump closed; and a bullet's kill, which tears
# nothing, the body whole where it fell
DISMEMBERMENT_CAMERA = "95.0 -150.0 4.0\n-0.6 0.78 -0.30\n0 0 1\n0.35\n"


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game's maps in assets/maps")
@pytest.mark.parametrize(
    "name, frame, camera, bullet",
    [
        # (the torso in the air above the limbs; at rest, apart, the blood on the
        # ground; shot, lying whole)
        ("dismemberment_flying", 2445, DISMEMBERMENT_CAMERA, False),
        ("dismemberment_at_rest", 2850, DISMEMBERMENT_CAMERA, False),
        ("dismemberment_bullet_whole", 2850, RAGDOLL_CAMERA, True),
    ],
)
def test_dismemberment_draws_as_approved(tmp_path, name, frame, camera, bullet):
    root = tmp_path / "root"
    root.mkdir()
    (root / "maps").symlink_to(MAPS)
    (root / "camera.txt").write_text(camera)
    frame = capture_game(
        tmp_path,
        frame,
        {
            "HALO_NETWORK_TEST": "host:bloodgulch",
            "HALO_NETWORK_TEST_CAMERA": "1",
            "HALO_NET_ONLINE": "false",
            "HALO_NET_JOIN_FROM_CLIPBOARD": "false",
            "HALO_RAGDOLLS": "true",
            "HALO_DISMEMBERMENT": "true",
            "HALO_NETWORK_TEST_RAGDOLL": "40",
            "HALO_NETWORK_TEST_RAGDOLL_BULLET": "true" if bullet else "false",
        },
        seconds=75,
        data_root=root,
    )
    assert (frame.width, frame.height) == GAME_SIZE
    check(frame, name, ragdoll_tolerance(name), RAGDOLL_OTHER_ENDINGS.get(name))


# the debris scene (debug.network_test_debris): at 40 seconds of frames an
# assault rifle held a metre above the slope three world units before the
# camera fires twenty rounds, its own firing effect ejecting the casings
# (effects\particles\solid\casing h ar bullet, 0.015 units wide, of
# effects\point physics\stones: density 2.4, friction 0.1, elasticity 0.35);
# with game.physics_debris on each casing that strikes the ground is a Jolt
# capsule, tumbling and coming to rest lying along its body, and with it off
# the game's point physics have them. The log counts them: those left after
# the burst and how many are bodies, and nearly ten seconds later those at
# rest (how many casings are left, and which, is the game's random lifetimes')
DEBRIS_CAMERA = "95.0 -150.0 4.0\n-0.645 0.681 -0.349\n0 0 1\n0.5\n"


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game's maps in assets/maps")
@pytest.mark.parametrize(
    "name, frame, debris",
    [
        # (the casings in the air and on the ground mid-burst; all at rest, Jolt's
        # and the game's)
        ("debris_flying", 2430, True),
        ("debris_at_rest", 2700, True),
        ("debris_off_at_rest", 2700, False),
    ],
)
def test_debris_draws_as_approved(tmp_path, name, frame, debris):
    root = tmp_path / "root"
    root.mkdir()
    (root / "maps").symlink_to(MAPS)
    (root / "camera.txt").write_text(DEBRIS_CAMERA)
    run = run_game(
        tmp_path,
        frame,
        {
            "HALO_NETWORK_TEST": "host:bloodgulch",
            "HALO_NETWORK_TEST_CAMERA": "1",
            "HALO_NET_ONLINE": "false",
            "HALO_NET_JOIN_FROM_CLIPBOARD": "false",
            "HALO_PHYSICS_DEBRIS": "true" if debris else "false",
            "HALO_NETWORK_TEST_DEBRIS": "40",
        },
        seconds=frame // 60 + 6,
        data_root=root,
        size=NATIVE_SIZE,
    )
    log = run.stdout + run.stderr
    burst = re.search(r"fired twenty rounds, (\d+) casings left, (\d+) of them Jolt's bodies", log)
    later = re.search(r"network test: (\d+) casings left, (\d+) of them at rest, (\d+) Jolt's bodies", log)
    assert burst and later, log[-3000:]
    casings, at_rest, bodies = (int(count) for count in later.groups())
    if debris:
        # (casings struck the ground as bodies, and those left are at rest or
        # bodies still settling, none lost through the ground)
        assert int(burst[2]) > 0, burst[0]
        assert at_rest > 0 and at_rest + bodies >= casings, later[0]
    else:
        assert int(burst[2]) == 0 and bodies == 0 and at_rest > 0, burst[0] + later[0]
    path = tmp_path / "game.png"
    assert path.is_file(), "no frame was captured: " + log[-2000:]
    frame = frame_compare.read_png(path)
    assert (frame.width, frame.height) == NATIVE_SIZE
    check(frame, name, ragdoll_tolerance(name))
