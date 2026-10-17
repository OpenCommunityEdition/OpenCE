"""Native modes with stale custom reserves: two engines, no inventory writes."""
import argparse
import json
from pathlib import Path
import re
import test_dual_combat as dual


def verify(root):
    m=json.loads((root/'manifest.json').read_text());checks=[]
    for peer in ('host','client'):
        log=(root/peer/'session.log').read_text(errors='replace')
        assert not re.search(r'EXCEPTION|ASSERTION FAILED',log)
        rows=[{k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',line)} for line in log.splitlines() if 'ammo off:' in line]
        players=dual.samples(root/peer/'session.log')
        for game,name in enumerate(m['variants'].split(',')):
            weapons=[r for r in rows if r['game']==game]
            assert weapons,(peer,name,'no native weapon samples')
            assert all(not r['infection'] and r['option']==100 and r['cap']==r['native_cap'] for r in weapons),(peer,name,'non-native limits')
            assert all(r['reserve']+r['loaded']==r['initial'] for r in weapons),(peer,name,'inventory changed',weapons)
            alive=[r for r in players if r['game']==game and r['tick']==300 and r['alive']]
            assert len(alive)==2 and all(r['speed']==1 and r['phase']==-1 for r in alive),(peer,name,'player state')
            checks.append({'peer':peer,'mode':name,'weapons':len(weapons),'result':'PASS'})
    return {'result':'PASS','full_engines':2,'protocol_bots':0,'checks':checks,
            'scope':'Native initial/map inventories and caps, ordinary spawns and consecutive mode changes; not full objective matches.'}


def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--verify-only',action='store_true');a=p.parse_args()
    if not a.verify_only:
        m=dual.prepare(Path('build/windows/halo.exe').resolve(),Path('assets/maps'),a.root,'bloodgulch',
                       'slayer,team_slayer,ctf,oddball,king,race')
        # Keep the same map for comparisons; no battle controller is used.
        for n,i in enumerate(m['instances']):
            i['overrides'].update(HALO_TEST_INPUT='look:inventory-off',HALO_NET_ADDRESS=f'127.0.0.{230+n}',HALO_NET_BROADCAST=f'127.0.0.{231-n}')
        dual.run(m,a.root,320)
    try: result=verify(a.root)
    except (AssertionError,StopIteration) as e:result={'result':'FAIL','error':str(e)}
    (a.root/'off-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
    return int(result['result']!='PASS')


if __name__=='__main__':raise SystemExit(main())
