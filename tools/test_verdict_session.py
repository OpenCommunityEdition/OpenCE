"""Real engines and optional protocol stand-in: record why a round ends."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import test_dual_combat as dual


def verify(root):
    manifest = json.loads((root/'manifest.json').read_text())
    modes = manifest['variants'].split(',')
    results = []
    for peer in manifest['instances']:
        log = (Path(peer['work'])/'session.log').read_text(errors='replace')
        assert not re.search(r'EXCEPTION|ASSERTION FAILED|!!! ASSERT', log), peer['name']
        rows = dual.samples(Path(peer['work'])/'session.log')
        for game, mode in enumerate(modes):
            match = [r for r in rows if r['game'] == game]
            assert match and any(r['over'] for r in match), f'{peer["name"]}: unfinished {mode}'
            if mode.startswith('infection'):
                if manifest['idle']:
                    assert any(r['winner'] == 0 and r['phase'] == 3 for r in match)
                    if peer['name'] == 'host':
                        assert re.search(r'infection: result=0 humans=2 zombies=1 elapsed=9000 limit=9000', log)
                else:
                    alpha = int('remote' in mode)
                    protected = [r for r in match if 25*30 <= r['tick'] <= 34*30 and r['machine'] == 2]
                    if '-ff-on' in mode:
                        assert protected and all(r['deaths'] == 1 and not r['over'] and r['team'] == 0 and r['alive'] for r in protected), 'betrayal must respawn human without conversion'
                        assert re.search(r'verdict audit: reason=death-before game='+str(game)+r' .*infection=1 .*friendly=1 ',log), 'no authoritative betrayal'
                        penalized = [r for r in match if r['machine']==1-alpha and r['deaths']==1 and not r['alive'] and not r['over']]
                        assert penalized and 420 <= max(r['respawn'] for r in penalized) <= 450, 'missing 5+10 second betrayal penalty'
                        assert any(r['machine']==1-alpha and r['deaths']==1 and r['alive'] and r['team']==1 and not r['over'] for r in match), 'no penalized zombie respawn'
                    else:
                        assert protected and all(r['deaths'] == 0 and not r['over'] and r['team'] == 0 for r in protected), 'friendly fire ended round'
                    zombie_dead = [r for r in match if r['machine'] == alpha and r['deaths'] == 1 and not r['alive']]
                    assert zombie_dead and not zombie_dead[0]['over'], 'zombie death ended round'
                    # A later postgame teardown also detaches the surviving unit.
                    assert any(r['machine']==alpha and r['deaths']==1 and r['alive'] and not r['over'] and
                               r['tick']>zombie_dead[0]['tick'] for r in match), 'no live zombie respawn before victory'
                    assert any(r['winner'] == 1 and r['phase'] == 3 for r in match)
                    assert all(any(r['machine'] == m and r['team'] == 1 and r['over'] for r in match) for m in range(3))
            elif peer['name'] == 'host':
                assert re.search(r'verdict audit: reason=slayer-score-limit game='+str(game)+r' .*infection=0 .*limit=1 ', log)
                if mode=='team_slayer':
                    assert re.search(r'verdict audit: reason=death-before game='+str(game)+r' .*infection=0 .*friendly=1 ',log), 'no betrayal'
                    assert any(r['score']<0 and not r['over'] for r in match), 'betrayal penalty or continuation missing'
            results.append({'peer':peer['name'], 'game':game, 'mode':mode, 'result':'PASS'})
    return {'result':'PASS','full_engines':len(manifest['instances']), 'protocol_bots':int(manifest['bots']), 'rounds':results}


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--exe', type=Path, default=Path('build/windows/halo.exe'))
    p.add_argument('--maps', type=Path, default=Path('assets/maps'))
    p.add_argument('--address-base', type=int, default=220, choices=range(1, 252))
    p.add_argument('--variants', default='infection-verdict,infection-verdict-remote,slayer')
    p.add_argument('--bots', action='store_true'); p.add_argument('--idle', action='store_true')
    p.add_argument('--seconds', type=int, default=300); p.add_argument('--verify-only', action='store_true')
    a = p.parse_args()
    if not a.verify_only:
        m = dual.prepare(a.exe.resolve(), a.maps, a.root, 'bloodgulch', a.variants)
        if not a.bots:
            work = a.root/'client2'; shutil.copytree(m['instances'][1]['work'], work)
            m['instances'].append({'name':'client2','work':str(work),'overrides':dict(m['instances'][1]['overrides'])})
        m.update(bots=a.bots, idle=a.idle)
        for n, i in enumerate(m['instances']):
            work = Path(i['work'])
            i['overrides'].update(HALO_DATA_ROOT=str(work),HALO_SAVE_ROOT=str(work/'saves'),HALO_NET_ADDRESS=f'127.0.0.{a.address_base+n}',
                HALO_NET_BROADCAST=f'127.0.0.{a.address_base+1},127.0.0.{a.address_base+2}' if not n else f'127.0.0.{a.address_base}',HALO_NETWORK_TEST_START='25',
                HALO_TEST_INPUT='look:party-verdict'+('-idle' if a.idle else ''),HALO_NETWORK_TEST_SCORE='1')
        procs, handles = [], []
        flags = subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
        try:
            for n, i in enumerate(m['instances']):
                work = Path(i['work']); env = {k:v for k,v in os.environ.items() if not k.upper().startswith('HALO_')}; env.update(i['overrides'])
                log = (work/'session.log').open('wb'); handles.append(log)
                proc = subprocess.Popen([str(work/m.get('executable', 'halo.exe'))],cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=flags)
                procs.append(proc); i['pid'] = proc.pid
                deadline = time.monotonic()+20
                while time.monotonic()<deadline:
                    debug = a.root/'host/debug.txt'
                    if debug.exists() and f'server added player from machine #{n} at controller index #0' in debug.read_text(errors='replace'): break
                    assert proc.poll() is None, f'{i["name"]}: exit {proc.returncode}'
                    time.sleep(.25)
                else: raise AssertionError('player registration timeout')
            if a.bots:
                log = (a.root/'bots.log').open('wb'); handles.append(log)
                procs.append(subprocess.Popen([sys.executable,'tools/system_link_bots.py','--host',f'127.0.0.{a.address_base}','--first-address',f'127.0.0.{a.address_base+3}',
                    '--machines','1','--seconds',str(a.seconds+30),'--status-every','10'],stdout=log,stderr=subprocess.STDOUT,creationflags=flags))
            (a.root/'manifest.json').write_text(json.dumps(m,indent=2))
            deadline = time.monotonic()+a.seconds; complete = None
            while time.monotonic()<deadline:
                assert all(p.poll() is None for p in procs), str([p.poll() for p in procs])
                finished = all(any(r['game']==len(a.variants.split(','))-1 and r['over'] for r in dual.samples(Path(i['work'])/'session.log')) for i in m['instances'])
                if finished: complete = complete or time.monotonic()
                if complete and time.monotonic()-complete>5: break
                time.sleep(1)
        finally:
            for proc in procs:
                dual.stop_process(proc)
            for h in handles: h.close()
    try: result = verify(a.root)
    except AssertionError as e: result = {'result':'FAIL','error':str(e)}
    (a.root/'verdict-result.json').write_text(json.dumps(result,indent=2)); print(json.dumps(result))
    return int(result['result'] != 'PASS')


if __name__ == '__main__': raise SystemExit(main())
