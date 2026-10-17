"""Negative controls on captured full-engine evidence; never launch Halo."""
from pathlib import Path
import os
import pytest
from tools import test_party_session as party


@pytest.fixture
def evidence():
    path=os.environ.get('OPENCE_PARTY_EVIDENCE')
    if not path: pytest.skip('set OPENCE_PARTY_EVIDENCE to a completed full-client party session')
    root=Path(path)
    if not (root/'result.json').exists(): pytest.skip('captured session unavailable')
    return root


def test_complete_full_engine_evidence(evidence):
    result=party.verify(evidence,write=False)
    assert result['result']=='PASS',result['errors']


@pytest.mark.parametrize('fault', ['ownership','damage','movement','conversion','respawn','postgame'])
def test_incomplete_evidence_is_rejected(evidence,monkeypatch,fault):
    original=party.samples
    changed=[]
    def altered(path):
        rows=original(path)
        peer=path.parent.name
        first=next((r['unit'] for r in rows if r['game']==0 and r['local'] and r['alive']),None)
        for r in rows:
            if fault=='ownership' and peer=='client2' and r['local']:
                r['machine']=1
            elif fault=='damage' and peer=='client' and r['game']==1:
                r['hits_sent']=0
            elif fault=='movement' and peer=='client' and r['local']:
                r['position']=[0,0,0]
            elif fault=='conversion' and peer=='client2' and r['game']==1 and r['team']==1:
                r['team']=0
            elif fault=='respawn' and peer=='client2' and r['game']==0 and r['local'] and r['alive']:
                r['unit']=first
            elif fault=='postgame' and peer=='client2' and r['over']:
                r['score']+=1
            else: continue
            changed.append(True)
        return rows
    monkeypatch.setattr(party,'samples',altered)
    result=party.verify(evidence,write=False)
    assert changed
    assert result['result']=='FAIL',fault
