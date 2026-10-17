"""Two full processes, persisted Item Options, finite ammo and real combat."""
import argparse
import json
from pathlib import Path
import shutil
import test_dual_combat as dual

VARIANTS = 'slayer-stock-ammo-normal,slayer-stock-ammo-rifles-normal,infection-stock-ammo,infection-stock-ammo-rifles-off-remote'


def verify(root):
    manifest = json.loads((root/'manifest.json').read_text())
    variants = manifest['variants'].split(',')
    result = {'result':'PASS', 'engines':2, 'bots':0, 'rounds':[]}
    for peer in ('host','client'):
        log = (root/peer/'session.log').read_text(errors='replace')
        assert 'ASSERTION FAILED' not in log and 'EXCEPTION' not in log
        ammo = [json.loads(l.split('ammo audit: ',1)[1]) for l in log.splitlines() if 'ammo audit: {' in l]
        samples = dual.samples(root/peer/'session.log')
        for game, name in enumerate(variants):
            machines = (0,1) if not name.startswith('infection') else ((0,) if 'remote' in name else (1,))
            for machine in machines:
                for slot in (0,1):
                    rows = [r for r in ammo if r['game']==game and r['machine']==machine and r['slot']==slot]
                    assert rows, (peer,name,machine,slot,'missing ammo')
                    first = rows[0]
                    expected = max(0, first['initial']-first['clip']) if 'normal' in name else (
                        100 if 'rifles' in name else (200 if slot==0 else 120))
                    assert first['reserve']==expected, (peer,name,slot,first,expected)
                    assert first['loaded']==min(first['initial'],first['clip'])
                    same = [r for r in rows if r['item']==first['item'] and r['tick']<720]
                    assert all(0<=r['loaded']<=r['clip'] and 0<=r['reserve']<=max(r['maximum'],expected) for r in same)
                    assert any(r['reserve']+r['loaded']<expected+first['loaded'] for r in same), (peer,name,slot,'no consumption')
                    assert any(b['loaded']>a['loaded'] and b['reserve']<a['reserve'] for a,b in zip(same,same[1:])), (peer,name,slot,'no reload')
                life = [r for r in samples if r['game']==game and r['machine']==machine]
                before, dead, after = dual.lifecycle(life,peer+' '+name)
                if name.startswith('infection'):
                    assert after['skull'] and after['team']==1 and after['weapons']==1 and after['grenades']==[0,0]
                    assert after['speed']==1.8 and (after['max_shield']==0)==('off' in name)
                    assert 4<=(after['tick']-dead['tick'])/30<=7
                    assert any(r['winner']==1 and r['over'] for r in life)
            if peer=='host':
                assert f'inventory profile: PASS game={game}' in log
            result['rounds'].append({'peer':peer,'variant':name,'initial_reload_death_respawn':'PASS'})
    return result


def main():
    p=argparse.ArgumentParser(); p.add_argument('--root',type=Path,required=True)
    p.add_argument('--verify-only',action='store_true'); p.add_argument('--seconds',type=int,default=330)
    p.add_argument('--variants',default=VARIANTS)
    a=p.parse_args()
    if not a.verify_only:
        maps=Path('assets/maps')
        m=dual.prepare(Path('build/windows/halo.exe').resolve(),maps,a.root,'ratrace',a.variants)
        for n,i in enumerate(m['instances']):
            i['overrides'].update(HALO_TEST_INPUT='look:inventory-ammo',HALO_NET_ADDRESS=f'127.0.0.{238+n}',HALO_NET_BROADCAST=f'127.0.0.{239-n}')
            shutil.copy2(maps/'bloodgulch.map',Path(i['work'])/'maps/bloodgulch.map')
        dual.run(m,a.root,a.seconds)
    try: result=verify(a.root)
    except AssertionError as e: result={'result':'FAIL','error':str(e)}
    (a.root/'ammo-result.json').write_text(json.dumps(result,indent=2)); print(json.dumps(result))
    return int(result['result']!='PASS')

if __name__=='__main__': raise SystemExit(main())
