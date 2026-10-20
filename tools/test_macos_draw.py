"""A draw through the macOS device (port/macos/guest/d3d8_metal_device.c) as
the game makes one, run in the app off the screen: the device made at
64x64, a blue clear, the left half drawn red by an NV2A vertex program and
a pixel shader passing the diffuse colour through, in immediate mode, as a
triangle strip and as a quad list; the frame captured and every pixel
checked (port/macos/tests/vm_draw.c).

Needs `ninja macos macos_app` in `nix develop`; no window is shown.
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

from tools import macos_guest_image  # noqa: E402
from tools.frame_compare import read_png  # noqa: E402

APP = ROOT / "build/macos/OpenCE.app/Contents/MacOS/OpenCE"
TRANSLATORS = [macos_guest_image.OBJECTS / f"port/linux/src/{name}.o" for name in ("nv2a_vsh", "nv2a_psh", "xgpu_text")]
# the post passes' (d3d8_metal_post.c): SMAA's text and lookup textures, and
# zlib to inflate them
POST = ([macos_guest_image.OBJECTS / "guest/gen/hud_hires_assets.o"]
        + [macos_guest_image.OBJECTS / f"port/third_party/zlib/{name}.o"
           for name in ("zutil", "inftrees", "uncompr", "inffast", "adler32", "inflate", "crc32")])

RED = (255, 0, 0, 255)
BLUE = (0, 0, 255, 255)

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(not macos_guest_image.available() or shutil.which("ninja") is None,
                       reason="needs `ninja macos` in nix develop"),
    pytest.mark.skipif(not APP.is_file(), reason="needs `ninja macos_app`"),
]


@pytest.fixture(scope="module")
def vm_draw(tmp_path_factory):
    work = tmp_path_factory.mktemp("vm_draw")
    return macos_guest_image.build_guest_image(
        [ROOT / "port/macos/tests/vm_draw.c", ROOT / "port/macos/tests/vm_draw_stubs.c",
         ROOT / "port/macos/tests/vm_hires_stubs.c",
         ROOT / "port/macos/guest/d3d8_metal_device.c", ROOT / "port/macos/guest/d3d8_metal.c",
         ROOT / "port/macos/guest/d3d8_metal_post.c",
         ROOT / "port/linux/src/d3d8_resources.c", ROOT / "port/linux/src/xbox_textures.c",
         ROOT / "port/macos/guest/opence_render_writer.c"],
        work, "vm_draw", ["-DHALO_METAL", f"-I{ROOT / 'port/macos/include'}", f"-I{ROOT / 'port/third_party/zlib'}"],
        like="port/linux/src/xgpu_text.c", objects=TRANSLATORS + POST)


def frame(image, tmp_path, case):
    """the captured frame of a case, run in the app's place off the screen"""
    (tmp_path / "home").mkdir(exist_ok=True)
    capture = tmp_path / "frame.png"
    environment = dict(os.environ, HOME=str(tmp_path / "home"), HALO_DATA_ROOT=str(tmp_path),
                       HALO_HOST_GUEST_IMAGE=str(image), HALO_HOST_CAPTURE=str(capture),
                       HALO_HOST_CAPTURE_FRAME="1", HALO_HOST_CAPTURE_SIZE="64x64")
    result = subprocess.run([str(APP), case], cwd=tmp_path, env=environment, capture_output=True, text=True,
                            timeout=300)
    assert result.returncode == 0 and result.stdout.strip().endswith("ok"), \
        result.stdout[-3000:] + result.stderr[-3000:]
    assert "log:" not in result.stdout, result.stdout
    picture = read_png(capture)
    assert (picture.width, picture.height) == (64, 64)
    return picture


def wrong_pixels(picture, expected):
    return [(x, y, picture.pixel(x, y)) for y in range(64) for x in range(64) if picture.pixel(x, y) != expected(x, y)]


def test_the_clear_alone_fills_the_frame(vm_draw, tmp_path):
    assert wrong_pixels(frame(vm_draw, tmp_path, "clear"), lambda x, y: BLUE)[:8] == []


@pytest.mark.parametrize("case", ["strip", "quads"])
def test_the_draw_covers_the_left_half(vm_draw, tmp_path, case):
    picture = frame(vm_draw, tmp_path, case)
    assert wrong_pixels(picture, lambda x, y: RED if x < 32 else BLUE)[:8] == []
