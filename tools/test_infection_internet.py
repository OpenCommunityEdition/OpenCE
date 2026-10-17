"""Private native invite transport smoke test; same PC, not a remote-NAT test."""
import json
import argparse
import os
import re
import shutil
import subprocess
import time
from pathlib import Path

from tools import test_dual_combat as combat


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,required=True)
    parser.add_argument('--maps',type=Path,default=Path('assets/maps'))
    args=parser.parse_args()
    root=args.root
    manifest=combat.prepare(Path('build/windows/halo.exe').resolve(),args.maps,
        root,'bloodgulch','infection-classic-200-0-1-5')
    processes=[]; handles=[]; result={'result':'FAIL','scope':'Two full Windows engines on one PC; public broker rendezvous and invite tunnel. Remote NAT not tested.'}
    try:
        for i,instance in enumerate(manifest['instances']):
            work=Path(instance['work']); shutil.copy2('build/windows/brokers.txt',work/'brokers.txt')
            instance['overrides'].update(HALO_NET_ONLINE='true',HALO_NET_HOST_PUBLIC='false',
                HALO_NET_MAP_SHARE='false',HALO_NET_MAP_SEED='false',HALO_NET_MAP_DOWNLOAD='false',
                HALO_NET_ALLOW_UPNP='false',HALO_NET_JOIN_FROM_CLIPBOARD='false',HALO_TEST_INPUT='look:classic',
                HALO_NET_ADDRESS=f'127.0.0.{220+i}',HALO_NET_BROADCAST='127.0.0.250',HALO_NETWORK_TEST_START='40')
            env={k:v for k,v in os.environ.items() if not k.startswith('HALO_')};env.update(instance['overrides']); (work/'localappdata').mkdir(); env['LOCALAPPDATA']=str(work/'localappdata')
            log=(work/'session.log').open('wb');handles.append(log)
            p=subprocess.Popen([str(work/'halo.exe')] + ([invite.group()] if i else []),cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
            processes.append(p);instance['pid']=p.pid
            if i==0:
                end=time.monotonic()+50
                while time.monotonic()<end:
                    if p.poll() is not None: raise AssertionError('host exited before invite')
                    text=(work/'session.log').read_text(errors='replace')
                    invite=re.search(r'halo://join/[0-9a-fA-F]{64}',text)
                    if invite: break
                    time.sleep(1)
                assert invite,'host generated no invite'
        (root/'manifest.json').write_text(json.dumps(manifest,indent=2))
        deadline=time.monotonic()+180
        while time.monotonic()<deadline:
            assert all(p.poll() is None for p in processes),'engine exited'
            peers=[combat.samples(root/name/'session.log') for name in ('host','client')]
            if all(any(r['winner']==1 for r in rows) for rows in peers): break
            time.sleep(1)
        for name,rows in zip(('host','client'),peers):
            log=(root/name/'session.log').read_text(errors='replace')
            assert 'Internet play: connected to' in log,f'{name}: no invite tunnel'
            assert 'ASSERTION FAILED' not in log and 'crash: exception' not in log
            assert {r['machine'] for r in rows}=={0,1},f'{name}: missing player'
            assert any(r['phase']==3 and r['winner']==1 for r in rows),f'{name}: no infection victory'
            assert any(r['machine']==1 and r['alive'] and r['speed']==2 and r['max_shield']==0 for r in rows)
        assert any(r['hits_dealt']>0 for r in peers[0]),'host accepted no remote hit'
        result.update(result='PASS',full_engines=2,protocol_standins=0,private=True,map_sharing=False,
                      distinct_lan_broadcast=True,invite_tunnel=True,zombie_victory_both=True)
    except Exception as error:
        result['error']=str(error)
    finally:
        for p in processes:
            if p.poll() is None:p.terminate()
            p.wait(timeout=15)
        for f in handles:f.close()
        (root/'result.json').write_text(json.dumps(result,indent=2))
    print(json.dumps(result))
    raise SystemExit(0 if result['result']=='PASS' else 1)


if __name__=='__main__':main()
