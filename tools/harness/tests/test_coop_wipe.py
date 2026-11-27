"""Host visibility policy and checkpoint membership through actual save/death code."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, run
from harness.coop_wipe import generated


@pytest.mark.parametrize('case', ['policy', 'authority', 'roster', 'save', 'new-map', 'delay', 'failure'])
def test_team_wipe(case):
    status, output = run(build('coop_wipe', generated()), case)
    assert status == 0, output
