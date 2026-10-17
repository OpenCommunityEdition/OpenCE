"""Negative controls against saved full-process combat evidence (opt-in)."""
import os
from pathlib import Path

import pytest

from tools import test_dual_combat as combat


@pytest.fixture
def evidence():
    value = os.environ.get('OPENCE_DUAL_EVIDENCE')
    if not value:
        pytest.skip('set OPENCE_DUAL_EVIDENCE to a completed dual combat session')
    return Path(value)


def test_complete_evidence(evidence):
    assert combat.verify(evidence)['result'] == 'PASS'


@pytest.mark.parametrize('fault,message', [
    ('initial_role', 'wrong initial zombie'),
    ('respawn', 'no live respawn'),
    ('skull', 'zombie not holding only'),
    ('remote_hit', 'host did not accept'),
    ('camera', 'camera outside live unit'),
    ('score', 'peers disagree'),
])
def test_rejects_corrupted_evidence(evidence, monkeypatch, fault, message):
    original = combat.samples
    changes = []

    def corrupted(path):
        rows = original(path)
        alpha = next(r['unit'] for r in rows if r['game'] == 1 and r['machine'] == 0
                     and r['phase'] == 2 and r['deaths'] == 0 and r['alive'])
        for r in rows:
            if fault == 'initial_role' and r['game'] == 1 and r['machine'] == 0 and r['deaths'] == 0:
                r['team'] = 0
            elif fault == 'respawn' and r['game'] == 1 and r['machine'] == 0 and r['deaths'] == 1:
                r['unit'] = alpha
            elif fault == 'skull' and r['game'] == 1 and r['machine'] == 0 and r['alive']:
                r['weapons'] = 2
            elif fault == 'remote_hit':
                r['hits_dealt'] = 0
            elif fault == 'camera' and r['local']:
                r['camera_distance'] = 999
            elif fault == 'score' and path.parent.name == 'client' and r['over']:
                r['score'] += 1
            else:
                continue
            changes.append(True)
        return rows

    monkeypatch.setattr(combat, 'samples', corrupted)
    with pytest.raises(AssertionError, match=message):
        combat.verify(evidence)
    assert changes, 'negative control never modified its input'
