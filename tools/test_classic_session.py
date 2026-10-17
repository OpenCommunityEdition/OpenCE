"""Two full engines: native movement phase, then staged real-input combat."""
import argparse
import json
import math
from pathlib import Path

from tools import test_dual_combat as combat


def verify(root):
    result = combat.verify(root)
    manifest = json.loads((root / 'manifest.json').read_text())
    variants = manifest['variants'].split(',')
    details = []
    for peer in ('host', 'client'):
        rows = combat.samples(root / peer / 'session.log')
        for game, variant in enumerate(variants):
            infection = variant.startswith('infection-classic-')
            if infection:
                _, _, speed, shields, alpha, respawn = variant.split('-')
                speed, shields, alpha, respawn = int(speed) or 180, int(shields), int(alpha), int(respawn) or 5
            for machine in (0, 1):
                alive = [r for r in rows if r['game']==game and r['machine']==machine and r['alive']
                         and r['tick']>240 and (not infection or r['phase']>=2)]
                assert alive, f'{peer} game {game}: no live samples'
                for row in alive:
                    zombie = infection and row['team']==1
                    if infection: assert abs(row['speed']-(speed/100 if zombie else 1))<.001, 'wrong speed'
                    if zombie and not shields:
                        assert row['max_shield']==row['shield']==0, 'zombie shield returned'
                    else:
                        assert row['max_shield']>0, 'normal shield capacity missing'
                before = [r for r in alive if 300<=r['tick']<1050 and not r['over']]
                assert len(before)>10, 'movement phase missing'
                travel = max(math.dist(before[0]['position'],r['position']) for r in before)
                assert travel>2, 'no displacement from ordinary controller movement'
                assert all(not (r['biped_flags'] & (1<<3)) for r in before), 'collision disabled'
                airborne = next((r for r in before if r['biped_flags']&1),None)
                assert airborne and any(r['tick']>airborne['tick'] and not r['biped_flags']&1 for r in before), 'jump/landing missing'
                if infection:
                    life = [r for r in rows if r['game']==game and r['machine']==machine and r['phase']>=2]
                    start, dead, after = combat.lifecycle(life, f'{peer} classic lifecycle')
                    delay = (after['tick']-dead['tick'])/30
                    assert respawn-1.1 <= delay <= respawn+1.6, f'respawn {delay}s, expected {respawn}'
                details.append({'peer':peer,'game':game,'machine':machine,'speed':speed if infection and machine==alpha else 100,
                                'travel':round(travel,3),'jump_landing':True,'respawn_seconds':delay if infection else None})
    result['classic'] = details
    result['scope'] = 'Normal input movement/jumps before tick 1050; existing gather fixture positions combat after that tick. No artificial damage.'
    return result


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--root',type=Path,required=True)
    parser.add_argument('--maps',type=Path,default=Path('assets/maps'))
    parser.add_argument('--variants',required=True)
    parser.add_argument('--address',type=int,default=210)
    parser.add_argument('--verify-only',action='store_true')
    args=parser.parse_args()
    result={}
    try:
        if not args.verify_only:
            manifest=combat.prepare(Path('build/windows/halo.exe').resolve(),args.maps,
                                    args.root,'bloodgulch',args.variants)
            for i,instance in enumerate(manifest['instances']):
                instance['overrides'].update(HALO_TEST_INPUT='look:classic',HALO_NET_ADDRESS=f'127.0.0.{args.address+i}',
                    HALO_NET_BROADCAST=f'127.0.0.{args.address+1-i}')
            (args.root/'manifest.json').write_text(json.dumps(manifest,indent=2))
            combat.run(manifest,args.root,480)
        result=verify(args.root)
    except Exception as error:
        result={'result':'FAIL','error':str(error)}
    (args.root/('classic-recheck.json' if args.verify_only else 'classic-result.json')).write_text(json.dumps(result,indent=2))
    print(json.dumps(result))
    raise SystemExit(0 if result['result']=='PASS' else 1)


if __name__=='__main__': main()
