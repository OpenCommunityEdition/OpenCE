"""Exact source matching, independent controls and map-scoped tag identities."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, read, run


@pytest.mark.parametrize('fault', [False, True])
def test_authored_emitters(fault):
    source = re.sub(r'^#include.*$', '', read('port/linux/game/authored_emitter_lighting.c'), flags=re.M)
    if fault:
        source = source.replace('definition == identities[i].light &&', 'TRUE &&')
    status, output = run(build('authored_emitter_lighting', (('under_test.inc', source),)), 'profiles')
    assert status == (CHECK_FAILED if fault else 0), output
