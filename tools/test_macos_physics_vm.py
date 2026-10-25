"""The physics bridge's import in the macOS host's VM (docs/macos_host.md,
"Physics").

port/macos/tests/vm_physics.c is built as a guest image and run in the
host, which carries out its batches on Jolt (port/macos/host/host_physics.c)
and writes the poses back: a box dropped on a level mesh must come to rest
on it, each step must cost one exit, and bad batches must be refused and
logged. Needs `ninja macos` in `nix develop`.
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
    pytest.mark.skipif(
        not macos_guest_image.available() or shutil.which("ninja") is None, reason="needs `ninja macos` in nix develop"
    ),
]

LOGGING = {"HALO_HOST_PHYSICS_LOG": "1", "HALO_HOST_IMPORT_COUNTS": "1"}


@pytest.fixture(scope="module")
def image(tmp_path_factory):
    work = tmp_path_factory.mktemp("vm_physics")
    return macos_guest_image.build_guest_image(
        [ROOT / "port/macos/tests/vm_physics.c"], work, "vm_physics", [f"-I{ROOT / 'port/macos/include'}"]
    )


def run_case(image, tmp_path, case):
    result = macos_guest_image.run_guest(image, [case], tmp_path / "run", LOGGING)
    assert result.returncode == 0 and result.stdout.strip().endswith("ok"), result.stdout + result.stderr
    return result.stdout, result.stderr


def import_counts(stderr):
    return {name: int(count) for name, count in re.findall(r"halo_host: import (\w+): (\d+)", stderr)}


def test_a_box_comes_to_rest_on_the_level(image, tmp_path):
    stdout, stderr = run_case(image, tmp_path, "drop")
    steps = int(re.search(r"steps (\d+)", stdout).group(1))
    # the first batch, then one exit a step
    assert import_counts(stderr).get("host_physics_step") == steps + 1, stderr
    lines = re.findall(r"halo_host: physics step: (\d+) refused, (\d+) ragdolls, (\d+) bodies, level (\d)", stderr)
    assert len(lines) == steps + 1 and all(line == ("0", "0", "1", "1") for line in lines), stderr
    # resting, and asleep by the end
    assert re.search(r"rest z 0\.2\d\d awake 0", stdout), stdout


def test_bad_batches_are_refused(image, tmp_path):
    _, stderr = run_case(image, tmp_path, "refusals")
    assert len(re.findall(r"halo_host: physics batch refused: ", stderr)) == 2, stderr
    assert len(re.findall(r"halo_host: physics command refused at byte \d+ \(op 2\)", stderr)) == 1, stderr
    assert "1 refused, 0 ragdolls, 0 bodies, level 0" in stderr, stderr
