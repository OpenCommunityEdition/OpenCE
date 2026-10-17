"""Opt-in complete-engine load/lifecycle test, not protocol stand-ins or AI.

Players stand still; two rotate through labelled fixture world deaths. The
last peer joins after play begins, disconnects and reconnects. Observe spawn
occupancy without changing spawn rules. Logs and process memory are retained.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time
import test_dual_combat as dual
from test_population_session import memory_usage


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--count', type=int, choices=(6, 10, 16), required=True)
    p.add_argument('--seconds', type=int, default=360)
    p.add_argument('--variant', default='team_slayer')
    args = p.parse_args()
    m = dual.prepare(args.exe, Path('assets/maps'), args.root, 'bloodgulch', args.variant)
    for n in range(2, args.count):
        work = args.root/f'client{n}'
        shutil.copytree(m['instances'][1]['work'], work)
        m['instances'].append({'name': f'client{n}', 'work': str(work), 'overrides': dict(m['instances'][1]['overrides'])})
    for n, peer in enumerate(m['instances']):
        work = Path(peer['work'])
        peer['overrides'].update(HALO_DATA_ROOT=str(work), HALO_SAVE_ROOT=str(work/'saves'),
            HALO_TEST_INPUT='look:population-death', HALO_NETWORK_TEST_START='55', HALO_NETWORK_TEST_SCORE='32767',
            HALO_NET_ADDRESS=f'127.0.0.{180+n}', HALO_NET_BROADCAST=','.join(f'127.0.0.{180+j}' for j in range(1,args.count)) if not n else '127.0.0.180')
    m.update(full_engines=args.count, protocol_bots=0, seconds=args.seconds)
    processes, handles, events, resources = {}, [], [], []
    start = time.monotonic()
    def launch(n, reconnect=False):
        peer = m['instances'][n]; work = Path(peer['work'])
        log = (work/('rejoin.log' if reconnect else 'session.log')).open('wb'); handles.append(log)
        env = {k:v for k,v in os.environ.items() if not k.upper().startswith('HALO_')}; env.update(peer['overrides'])
        proc = subprocess.Popen([str(work/'halo.exe')], cwd=work, env=env, stdout=log, stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
        processes[n] = proc; peer['pid'] = proc.pid
        events.append({'event':'rejoin' if reconnect else 'launch', 'peer':n, 'pid':proc.pid, 'seconds':time.monotonic()-start})
        (args.root/'manifest.json').write_text(json.dumps(m,indent=2))
    error = None
    try:
        for n in range(args.count-1):
            launch(n); time.sleep(1)
        late = left = rejoined = False; next_sample = 0
        while time.monotonic()-start < args.seconds:
            elapsed = time.monotonic()-start
            if elapsed >= 85 and not late: launch(args.count-1); late=True
            if elapsed >= 135 and not left:
                proc=processes.pop(args.count-1); proc.terminate(); proc.wait(timeout=15)
                events.append({'event':'disconnect', 'peer':args.count-1, 'seconds':elapsed}); left=True
            if elapsed >= 155 and not rejoined: launch(args.count-1,True); rejoined=True
            assert all(proc.poll() is None for proc in processes.values()), 'unexpected process exit'
            if elapsed >= next_sample:
                for n,proc in processes.items():
                    work=Path(m['instances'][n]['work']); debug=work/'debug.txt'
                    assert not debug.exists() or 'EXCEPTION' not in debug.read_text(errors='replace'), (n,'exception')
                    resources.append({'peer':n,'elapsed':round(elapsed,1),'memory':memory_usage(proc.pid)})
                next_sample=elapsed+10
            time.sleep(.5)
    except (AssertionError, OSError) as exc:
        error=str(exc)
    finally:
        for proc in processes.values():
            if proc.poll() is None: proc.terminate()
            proc.wait(timeout=15)
        for h in handles: h.close()
        (args.root/'events.json').write_text(json.dumps(events,indent=2))
        (args.root/'resources.json').write_text(json.dumps(resources,indent=2))
    checks=[]
    try:
        assert error is None, error
        for n,peer in enumerate(m['instances']):
            work=Path(peer['work']); rows=dual.samples(work/'session.log')
            again=dual.samples(work/'rejoin.log') if (work/'rejoin.log').exists() else []
            local=[r for r in rows if r['local'] and r['alive'] and not r['over']]
            assert local, (n,'never alive in active round')
            assert all(r['camera_cluster']>=0 and r['attached'] for r in local), (n,'invalid camera/ownership')
            if n==args.count-1:
                assert any(r['local'] and r['alive'] and not r['over'] for r in again), 'rejoin did not spawn'
                if args.variant.startswith('infection'): assert all(r['team']==1 for r in local), 'late player not zombie'
            if n in (0,1):
                assert any(r['local'] and not r['alive'] and r['deaths']>0 for r in rows), (n,'no fixture death')
                assert any(r['local'] and r['alive'] and r['deaths']>0 for r in rows), (n,'no respawn')
            mem=[r['memory']['private_bytes'] for r in resources if r['peer']==n]
            checks.append({'peer':n,'live_local_samples':len(local),'last_tick':max(r['tick'] for r in rows+again),
                           'max_private_bytes':max(mem),'last_minus_first_private_bytes':mem[-1]-mem[0]})
        host=dual.samples(args.root/'host/session.log')
        ticks={}
        for r in host:
            if not r['quit']: ticks.setdefault((r['game'],r['tick']),[]).append(r)
        peak_registered=max(len(rs) for rs in ticks.values()); peak_active=max(sum(r['alive'] for r in rs) for rs in ticks.values())
        assert peak_registered>=args.count, ('registration',peak_registered,args.count)
        result={'result':'PASS','full_engines':args.count,'protocol_bots':0,'seconds':round(time.monotonic()-start,1),
                'peak_registered':peak_registered,'peak_active_units':peak_active,'checks':checks,
                'scope':'Stationary full engines, sampled memory/camera, labelled world deaths/respawns and late/rejoin. Not AI/combat or a proof of no memory leaks.'}
    except AssertionError as exc:
        result={'result':'FAIL','error':str(exc),'full_engines':args.count,'protocol_bots':0,'checks':checks}
    (args.root/'soak-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
    return int(result['result']!='PASS')


if __name__=='__main__': raise SystemExit(main())
