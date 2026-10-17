"""Observe native grenade policy with two engines and optional protocol stand-ins."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import threading
import time

import test_dual_combat as dual


def verify(root):
    m=json.loads((root/'manifest.json').read_text())
    result=[]
    peers={}
    for peer in ('host','client'):
        log=(root/peer/'session.log').read_text(errors='replace')
        assert not re.search(r'EXCEPTION|ASSERTION FAILED',log),peer
        peers[peer]=dual.samples(root/peer/'session.log')
    for game,name in enumerate(m['variants'].split(',')):
        selected={peer:[r for r in rows if r['game']==game and r['tick']==450 and r['alive']]
                  for peer,rows in peers.items()}
        assert all(len(rows)==m['participants'] for rows in selected.values()),(name,'active players', {p:len(r) for p,r in selected.items()})
        host={r['player']:r for r in selected['host']}
        for r in selected['client']:
            assert r['grenades']==host[r['player']]['grenades'],(name,'inventory desync',r)
        generic='generic' in name
        spawn_log=(root/'host/session.log').read_text(errors='replace')
        spawn=next(line for line in spawn_log.splitlines() if 'grenade spawn: stage=after' in line and f'game={game} ' in line)
        policy={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',spawn)}
        assert policy['weapon_set']==0, 'not Weapon Set NORMAL'
        for r in host.values():
            zombie=name.startswith('infection') and r['team']==1
            quota=1 if m['participants']>=9 else 2 if m['participants']>=5 else 4
            no_grenades=not generic and policy['equipment_flags']&1
            plasma=not generic and policy['equipment_flags']&2
            expected=[0,0] if zombie or no_grenades else [0,quota] if plasma else [quota,0]
            assert r['grenades']==expected,(name,r['player'],r['grenades'],expected)
        if generic:
            # At least one full human engine throws through ordinary controller input.
            humans=[r for r in host.values() if r['machine']<2 and r['team']==0]
            assert humans,(name,'no full human engine')
            assert any(r['player']==h['player'] and r['game']==game and 570<=r['tick']<=720
                       and r['grenades'][0]<h['grenades'][0] for h in humans for r in peers['host']), 'no real grenade consumption'
        result.append({'variant':name,'participants':len(host),'grenades':{str(p):r['grenades'] for p,r in host.items()},'result':'PASS'})
    return {'result':'PASS','full_engines':2,'protocol_standins':m['participants']-2,'rounds':result}


def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True)
    p.add_argument('--count',type=int,default=2);p.add_argument('--map',default='bloodgulch')
    p.add_argument('--variants',default='slayer,infection-stock,infection-stock-generic')
    p.add_argument('--seconds',type=int,default=280)
    p.add_argument('--verify-only',action='store_true');a=p.parse_args()
    if not a.verify_only:
        m=dual.prepare(Path('build/windows/halo.exe').resolve(),Path('assets/maps'),a.root,a.map,
                       a.variants)
        m['participants']=a.count
        for n,i in enumerate(m['instances']):
            i['overrides'].update(HALO_TEST_INPUT='look:inventory-grenades',HALO_NET_ADDRESS=f'127.0.0.{240+n}',
                                 HALO_NET_BROADCAST=f'127.0.0.{241-n}',HALO_NETWORK_TEST_START='25')
        bots=[];cancel=threading.Event()
        with (a.root/'bots.log').open('wb') as log:
            def join_bots():
                deadline=time.monotonic()+45
                while not cancel.is_set() and time.monotonic()<deadline:
                    debug=a.root/'host/debug.txt'
                    if debug.exists() and 'server added player from machine #1 at controller index #0' in debug.read_text(errors='replace'):
                        bots.append(subprocess.Popen([sys.executable,'tools/system_link_bots.py','--host','127.0.0.240',
                            '--first-address','127.0.0.20','--machines',str(a.count-2),'--seconds',str(a.seconds+30),'--status-every','20'],
                            stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0))
                        return
                    cancel.wait(.25)
            worker=threading.Thread(target=join_bots)
            try:
                if a.count>2: worker.start()
                dual.run(m,a.root,a.seconds)
            finally:
                cancel.set()
                if a.count>2: worker.join(timeout=5)
                for bot in bots:
                    if bot.poll() is None: bot.terminate();bot.wait(timeout=10)
    try:result=verify(a.root)
    except (AssertionError,StopIteration) as e:result={'result':'FAIL','error':str(e)}
    (a.root/'grenade-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
    return int(result['result']!='PASS')


if __name__=='__main__':raise SystemExit(main())
