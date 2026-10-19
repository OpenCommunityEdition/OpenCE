"""Actual co-op surface producer/consumer over separate fake host and client state."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, run
from harness.surface_sync import generated


@pytest.mark.parametrize('case', ['rewind', 'ordering', 'repair', 'full', 'bounds', 'authority'])
def test_surface_state(case):
    status, output = run(build('coop_surface_sync', generated()), case)
    assert status == 0, output
