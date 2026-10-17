"""Three full engines: native death drops and contact pickups."""
import argparse
import json
from pathlib import Path
import shutil
import re
import test_dual_combat as dual


def verify(root):
    log=(root/'host/session.log').read_text(errors='replace')
    drops=re.search(r'resource drops: count=(\d+) expected=(\d+).*weapons=(-?\d+),(-?\d+) ammo=(\d+),(\d+)',log)
    assert drops and drops[1]==drops[2]=='6', 'native drops missing'
    contact=re.search(r'resource zombie contact: remaining=(\d+) expected=(\d+) grenades=(\d+),(\d+) weapons=(\d+)',log)
    # Native unit_get_weapon_count excludes objective weapons such as skulls.
    assert contact and contact[1]==contact[2]=='6' and contact.groups()[2:]==('0','0','0'), 'zombie consumed resources'
    recovery=re.search(r'resource recovery: remaining=(\d+) grenades=(\d+),(\d+) weapons=(\d+)',log)
    assert recovery and recovery.groups()[1:]==('4','0','2'), 'human recovery incomplete'
    dropped={int(i):(int(r),int(l)) for i,r,l in re.findall(r'resource dropped weapon: item=(-?\d+) reserve=(\d+) loaded=(\d+)',log)}
    received={int(i):(int(r),int(l)) for i,r,l in re.findall(r'resource receiver weapon: item=(-?\d+) reserve=(\d+) loaded=(\d+)',log)}
    if dropped:
        assert sum(dropped[int(drops[3])])==int(drops[5]) and sum(dropped[int(drops[4])])==int(drops[6]), 'death changed ammo'
        assert received[int(drops[4])]==dropped[int(drops[4])], 'picked weapon changed ammo or identity'
        other=[v for i,v in received.items() if i!=int(drops[4])]
        assert len(other)==1 and other[0][0]==dropped[int(drops[3])][0], 'ammo pickup duplicated or lost reserve'
        assert len(set(re.findall(r'resource zombie touches: item=(-?\d+)',log)))==6, 'not all six drops were touched'
    for peer in ('host','client','client2'):
        text=(root/peer/'session.log').read_text(errors='replace')
        assert 'ASSERTION FAILED' not in text and 'EXCEPTION' not in text
        rows=dual.samples(root/peer/'session.log')
        zombies=[r for r in rows if r['machine']==0 and r['alive'] and r['phase']==2 and r['deaths']==1]
        assert zombies and all(r['skull'] and r['weapons']==1 and r['grenades']==[0,0] for r in zombies)
        human=[r for r in rows if r['machine']==2]
        assert any(r['alive'] and r['team']==0 and r['weapons']==2 and r['grenades']==[4,0] and r['over'] for r in human)
        if dropped:
            settled=[r for r in human if r['alive'] and r['over'] and r['weapons']==2]
            assert settled[-1]['ammo']==sum(dropped[int(drops[4])]), 'picked weapon ammo not synchronized'
        life=[r for r in rows if r['machine']==1]
        _,_,after=dual.lifecycle(life,peer)
        assert after['team']==1 and after['skull'] and after['weapons']==1 and after['grenades']==[0,0]
    return {'result':'PASS','full_engines':3,'bots':0,'drops':6,'zombie_contact':'PASS','human_recovery':'PASS','conversion_respawn_sync':'PASS'}


def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--verify-only',action='store_true')
    p.add_argument('--variant',default='infection-stock-custom')
    a=p.parse_args()
    if not a.verify_only:
        m=dual.prepare(Path('build/windows/halo.exe').resolve(),Path('assets/maps'),a.root,'bloodgulch',a.variant)
        source=m['instances'][1]; work=a.root/'client2'; shutil.copytree(source['work'],work)
        m['instances'].append({'name':'client2','work':str(work),'overrides':dict(source['overrides'])})
        for n,i in enumerate(m['instances']):
            work=Path(i['work'])
            i['overrides'].update(HALO_DATA_ROOT=str(work),HALO_SAVE_ROOT=str(work/'saves'),HALO_TEST_INPUT='look:inventory-resources',
                HALO_NET_ADDRESS=f'127.0.0.{240+n}',HALO_NET_BROADCAST='127.0.0.241,127.0.0.242' if n==0 else '127.0.0.240',HALO_NETWORK_TEST_START='18')
        dual.run(m,a.root,110)
    try: result=verify(a.root)
    except AssertionError as e: result={'result':'FAIL','error':str(e)}
    (a.root/'resources-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
    return int(result['result']!='PASS')

if __name__=='__main__':raise SystemExit(main())
