"""32-bit event ABI and real batching preserve every surface-mask bit."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, run
from harness.surface_wire import generated


def test_surface_mask_wire_roundtrip():
    status, output = run(build('coop_surface_wire', generated()), 'wire32')
    assert status == 0, output
