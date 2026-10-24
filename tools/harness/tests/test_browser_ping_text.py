"""The server browser's ping cell (menu_functions.c's browser_ping_text)."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure


def generated(fault=None):
    header = read('port/linux/src/p2p.h')
    code = function(read('port/linux/game/menu_functions.c'), 'browser_ping_text')
    if fault:
        code = mutated(code, *fault)
    return (('listing.inc', enum_with(header, 'P2P_LISTING_NAME_SIZE') + structure(header, 'p2p_listing')),
            ('under_test.inc', code))


@pytest.mark.parametrize('case', ['number', 'dots', 'status', 'dash'])
def test_case(case):
    status, output = run(build('browser_ping_text', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    (('now / 300', 'now / 600'), 'dots'),
    (('if (game->probing)', 'if (game->probing && game->open && !game->in_progress)'), 'dots'),
    (('if (game->failed)', 'if (0)'), 'status'),
    (('game->ping >= 0', 'game->ping >= 0 && !game->probing'), 'number'),
])
def test_negative_control(fault, case):
    status, output = run(build('browser_ping_text', generated(fault)), case)
    assert status == CHECK_FAILED, output
