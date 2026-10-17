"""Two full engines: native map pickup, finite use, drop and pickup again."""
import argparse
import json
from pathlib import Path
import re
import test_dual_combat as dual


def fields(log, marker):
    return [{k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',line)} for line in log.splitlines() if marker in line]


def verify(root):
    manifest=json.loads((root/'manifest.json').read_text()); rounds=[]
    for peer in ('host','client'):
        log=(root/peer/'session.log').read_text(errors='replace')
        assert not re.search(r'EXCEPTION|ASSERTION FAILED',log)
        for game, name in enumerate(manifest['variants'].split(',')):
            kind,value=map(int,name.split('-map-')[1].split('-',1))
            original=[r for r in fields(log,'provenance map:') if r['game']==game]
            assert original, (peer,name,'map weapon missing')
            initial=original[0]; active=name.startswith('infection')
            expected=(48,12,180,8)[kind] if value<0 or not active else min(value,999)
            if peer=='host': assert initial['reserve']==expected, ('map reserve',initial,expected)
            rows=[r for r in fields(log,'provenance sample:') if r['game']==game]
            held=[r for r in rows if r['parent']==r['human']]
            first=next((r for r in held if 12<=r['second']<=16),None)
            assert first and first['reserve']==expected, (peer,name,'pickup reserve',first)
            assert first['loaded']==(12,12,60,4)[kind], 'changed clip'
            if value!=0:
                used=next((r for r in held if r['second']==27),None)
                assert used and used['reserve']+used['loaded']<first['reserve']+first['loaded'], 'no consumption'
                assert any(b['loaded']>a['loaded'] and b['reserve']<a['reserve'] for a,b in zip(held,held[1:]) if 20<=b['second']<=26), 'no reload'
            before=next(r for r in held if r['second']==29)
            after=next((r for r in held if 36<=r['second']<=39),None)
            assert after and (after['reserve'],after['loaded'],after['item'])==(before['reserve'],before['loaded'],before['item']), (peer,'drop refilled/lost',before,after)
            if peer=='host':
                assert any(r['parent']==-1 and r['dropped']==r['item'] for r in rows if 30<=r['second']<=33), 'no manual drop'
                spawned=[r for r in fields(log,'provenance respawn:') if r['game']==game]
                assert spawned and spawned[0]['reserve']==expected, 'normal respawn reserve'
                dup=[r for r in fields(log,'provenance duplicate:') if r['game']==game]
                contact=125 if kind==3 else 45
                before_dup=next(r for r in dup if r['second']==contact-1)
                after_dup=next(r for r in dup if r['second']==contact+5)
                repeated=next(r for r in dup if r['second']==contact+9)
                assert before_dup['source_reserve']+before_dup['target_reserve']==after_dup['source_reserve']+after_dup['target_reserve'], 'duplicate pickup created ammo'
                cap=min(value,999) if active and value>=0 else (120,60,600,24)[kind]
                assert after_dup['target_reserve']==min(cap,before_dup['target_reserve']+before_dup['source_reserve']), ('wrong reserve cap',after_dup,cap)
                assert (after_dup['source_reserve'],after_dup['target_reserve'])==(repeated['source_reserve'],repeated['target_reserve']), 'repeated contact refilled'
                assert f'inventory profile: PASS game={game}' in log
            if name.startswith('infection'):
                assert all(r['parent']==-1 for r in rows if 7<=r['second']<=9), 'zombie picked map weapon'
            assert 'provenance complete:' in log if peer=='host' else True
            rounds.append({'peer':peer,'game':game,'variant':name,'result':'PASS'})
    return {'result':'PASS','full_engines':2,'protocol_bots':0,'rounds':rounds}


def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True)
    p.add_argument('--variants',default='infection-stock-ammo-map-2-500');p.add_argument('--seconds',type=int,default=105)
    p.add_argument('--verify-only',action='store_true');a=p.parse_args()
    if not a.verify_only:
        m=dual.prepare(Path('build/windows/halo.exe').resolve(),Path('assets/maps'),a.root,'bloodgulch',a.variants)
        for n,i in enumerate(m['instances']):
            i['overrides'].update(HALO_TEST_INPUT='look:inventory-provenance',HALO_NET_ADDRESS=f'127.0.0.{230+n}',HALO_NET_BROADCAST=f'127.0.0.{231-n}',
                HALO_NETWORK_TEST_PICKUP_WEAPON=','.join(v.split('-map-')[1].split('-')[0] for v in a.variants.split(',')))
        dual.run(m,a.root,a.seconds)
    try:result=verify(a.root)
    except (AssertionError,StopIteration) as e:result={'result':'FAIL','error':str(e)}
    (a.root/'map-ammo-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
    return int(result['result']!='PASS')


if __name__=='__main__':raise SystemExit(main())
