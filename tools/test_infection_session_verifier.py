"""Bounds for the remote weapon-ready transition in the drop verifier."""
import pytest

from tools.test_infection_sessions import assert_drop_controls


def log_for(readied, ticks=None, weapon='4e7'):
    ticks = ticks or [180 + 30*i for i in range(len(readied))]
    log = ['infection state: phase=2 humans=2 zombies=1',
           'infection drop test: ground_skulls=0 ball=4e7']
    for tick, ready in zip(ticks, readied):
        log.append(f'network test: tick {tick} player 0: (0 0 0) g0/0 w '
                   f'ready{0 if ready else -1} controls1800 held{"abcdef" if ready else "ffffffff"} '
                   f'st{0 if ready else 33} {weapon}:0 s0 k0 d0 f0 t1 m0 | items 1')
    return '\n'.join(log)


def test_remote_initial_draw_recovers_next_sample():
    assert_drop_controls(log_for([False, True, True, True]), 1, require_local=False)


@pytest.mark.parametrize('ready,ticks,peer,weapon', [
    ([False, False, True, True, True], None, 1, '4e7'),
    ([False, True, True, True], [180, 240, 270, 300], 1, '4e7'),
    ([True, True, True, False], None, 1, '4e7'),
    ([False, True, True, True], None, 0, '4e7'),
    ([False, True, True, True], None, 1, '123'),
    ([False], None, 1, '4e7'),
])
def test_rejects_missing_or_lost_skull(ready, ticks, peer, weapon):
    with pytest.raises(AssertionError):
        assert_drop_controls(log_for(ready, ticks, weapon), peer, require_local=False)
