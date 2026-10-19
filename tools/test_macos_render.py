"""The render commands' doorbell in the macOS host's VM (docs/macos_host.md,
"The renderer").

port/macos/tests/vm_render.c is built as a guest image with the guest's
batch writer (port/macos/guest/opence_render_writer.c) and run in the host,
which decodes each batch it is given (port/macos/runtime/src/render.rs) and,
with HALO_HOST_RENDER_LOG, says what it found: the batches must arrive as
the guest wrote them, and each must cost one exit. Needs `ninja macos` in
`nix develop`.
"""

import platform
import re
import shutil
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import macos_guest_image  # noqa: E402

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(not macos_guest_image.available() or shutil.which("ninja") is None,
                       reason="needs `ninja macos` in nix develop"),
]

LOGGING = {"HALO_HOST_RENDER_LOG": "1", "HALO_HOST_IMPORT_COUNTS": "1"}


@pytest.fixture(scope="module")
def image(tmp_path_factory):
    work = tmp_path_factory.mktemp("vm_render")
    return macos_guest_image.build_guest_image(
        [ROOT / "port/macos/tests/vm_render.c", ROOT / "port/macos/guest/opence_render_writer.c"], work,
        "vm_render", [f"-I{ROOT / 'port/macos/include'}"])


def run_case(image, tmp_path, case):
    result = macos_guest_image.run_guest(image, [case], tmp_path / "run", LOGGING)
    assert result.returncode == 0 and result.stdout.strip().endswith("ok"), result.stdout + result.stderr
    return result.stdout, result.stderr


def decoded(stderr):
    """(commands, refused, fnv) of each batch the host decoded"""
    return [(int(commands), int(refused), int(fnv, 16)) for commands, refused, fnv in
            re.findall(r"halo_host: render batch: (\d+) commands, (\d+) refused, fnv (0x[0-9a-f]+)", stderr)]


def import_counts(stderr):
    return {name: int(count) for name, count in re.findall(r"halo_host: import (\w+): (\d+)", stderr)}


def test_batches_arrive_as_the_guest_wrote_them(image, tmp_path):
    stdout, stderr = run_case(image, tmp_path, "batches")
    written = [int(fnv, 16) for fnv in re.findall(r"batch \d+ fnv (0x[0-9a-f]+)", stdout)]
    assert decoded(stderr) == [(3, 0, written[0]), (2, 1, written[1]), (0, 1, written[2])], stderr
    # each refusal is logged with its place and reason, and so is each batch
    # refused whole
    assert len(re.findall(r"halo_host: render command refused at byte \d+ \(op 2\)", stderr)) == 2, stderr
    assert len(re.findall(r"halo_host: render batch refused: ", stderr)) == 3, stderr


def test_each_batch_is_one_exit(image, tmp_path):
    stdout, stderr = run_case(image, tmp_path, "flush")
    assert "batches 5" in stdout
    batches = decoded(stderr)
    assert [commands for commands, _, _ in batches] == [20] * 5, stderr
    assert import_counts(stderr).get("host_render_submit") == 5, stderr


def test_the_scenes_objects_are_valid(image, tmp_path):
    # the frame tools/test_macos_frames.py captures in the app: here, with no
    # window, the host's own check takes its library, pipelines and buffer,
    # and refuses only the window's pass and what is drawn in it
    result = macos_guest_image.run_guest(image, ["scene"], tmp_path / "run", LOGGING)
    refused = re.findall(r"halo_host: render command refused at byte \d+ \(op (\d+)\): (.*)", result.stderr)
    assert refused[0] == ("10", "the window has no size yet"), result.stderr
    assert all(op in {"10", "11", "12", "18", "19", "22"} for op, _ in refused), result.stderr
    assert decoded(result.stderr)[0][:2] == (6, 8), result.stderr


def test_the_host_is_quiet_about_batches_unless_asked(image, tmp_path):
    result = macos_guest_image.run_guest(image, ["flush"], tmp_path / "run")
    assert result.returncode == 0 and "halo_host" not in result.stdout + result.stderr, result.stdout + result.stderr
