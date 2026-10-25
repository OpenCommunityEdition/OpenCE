"""The Metal renderer's post passes (port/macos/guest/d3d8_metal_post.c):
FXAA and SMAA's three passes written as MSL, run in the macOS host's VM as
the guest build compiles them (port/macos/tests/vm_post.c), each library
compiled by Metal (tools/msl_check.swift), and SMAA's lookup textures
inflated to their sizes.

Needs `ninja macos` in `nix develop`.
"""

import platform
import shutil
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import macos_guest_image  # noqa: E402
from tools.test_nv2a_msl import metal_compile  # noqa: E402

PASSES = ("fxaa", "edges", "weights", "blend")
ZLIB = ["zutil", "inftrees", "uncompr", "inffast", "adler32", "inflate", "crc32"]
OBJECTS = [
    macos_guest_image.OBJECTS / "port/linux/src/xgpu_text.o",
    macos_guest_image.OBJECTS / "guest/gen/hud_hires_assets.o",
] + [macos_guest_image.OBJECTS / f"port/third_party/zlib/{name}.o" for name in ZLIB]

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(
        not macos_guest_image.available() or shutil.which("ninja") is None, reason="needs `ninja macos` in nix develop"
    ),
]


@pytest.fixture(scope="module")
def vm_post(tmp_path_factory):
    work = tmp_path_factory.mktemp("vm_post")
    return macos_guest_image.build_guest_image(
        [
            ROOT / "port/macos/tests/vm_post.c",
            ROOT / "port/macos/tests/vm_nv2a_stubs.c",
            ROOT / "port/macos/guest/d3d8_metal_post.c",
        ],
        work,
        "vm_post",
        ["-DHALO_METAL", f"-I{ROOT / 'port/third_party/zlib'}"],
        like="port/linux/src/xgpu_text.c",
        objects=OBJECTS,
    )


def run(image, folder, *arguments):
    result = macos_guest_image.run_guest(image, [str(argument) for argument in arguments], folder)
    assert result.returncode == 0, f"{arguments}: {result.stdout[-2000:]} {result.stderr[-2000:]}"
    return result.stdout


@pytest.fixture(scope="module")
def sources(vm_post, tmp_path_factory):
    """{pass: msl} at 640x480"""
    folder = tmp_path_factory.mktemp("vm_post_run")
    return {name: run(vm_post, folder, "source", name, 640, 480) for name in PASSES}


def test_each_pass_is_metal(sources):
    for name, msl in sources.items():
        assert msl.startswith("#include <metal_stdlib>"), name
        assert "vertex post_out vertex_main(" in msl, name
        assert "fragment " in msl and " fragment_main(" in msl, name
        # (SMAA's text is ASCII once written: Metal refuses its comments'
        # other bytes in some places)
        assert all(ord(character) < 0x80 for character in msl), name
    for name in PASSES[1:]:
        assert "#define SMAA_RT_METRICS float4(1.0 / 640.0, 1.0 / 480.0, 640.0, 480.0)" in sources[name], name
        assert "#define SMAA_PRESET_HIGH" in sources[name], name


def test_the_size_is_the_libraries_own(vm_post, tmp_path):
    msl = run(vm_post, tmp_path, "source", "edges", 3840, 2160)
    assert "float4(1.0 / 3840.0, 1.0 / 2160.0, 3840.0, 2160.0)" in msl


def test_each_pass_compiles(sources, tmp_path):
    paths = []
    for name, msl in sources.items():
        for stage in ("vertex", "fragment"):
            path = tmp_path / f"{name}.{stage}.metal"
            path.write_text(msl)
            paths.append(path)
    code, results = metal_compile(paths)
    failures = {name: message for name, message in results.items() if message.strip() != "ok"}
    assert failures == {} and code == 0, "\n".join(f"{name}: {message}" for name, message in failures.items())


def test_the_lookup_textures_inflate(vm_post, tmp_path):
    # SMAA's area texture, RG8 at 160x560, and its search texture, R8 at 64x16
    width, height, size, _ = run(vm_post, tmp_path, "lookup", "area").split()
    assert (int(width), int(height), int(size)) == (160, 560, 2)
    width, height, size, _ = run(vm_post, tmp_path, "lookup", "search").split()
    assert (int(width), int(height), int(size)) == (64, 16, 1)
