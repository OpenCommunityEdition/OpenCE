"""Three full isolated engines; controller movement/combat, no protocol stand-ins."""
import argparse
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

if __package__:
    from .test_dual_combat import prepare, samples, sockets, lifecycle, sha
    from .test_population_session import memory_usage
    from .analyze_population_session import analyze
else:
    from test_dual_combat import prepare, samples, sockets, lifecycle, sha
    from test_population_session import memory_usage
    from analyze_population_session import analyze


def run(args):
    manifest = prepare(args.exe, args.maps, args.root, args.map, args.variants)
    source = manifest['instances'][1]
    work = args.root / 'client2'
    shutil.copytree(source['work'], work)
    manifest['instances'].append({'name': 'client2', 'work': str(work),
                                  'overrides': dict(source['overrides'])})
    manifest.update(full_engines=3, protocol_standins=0)
    for index, instance in enumerate(manifest['instances']):
        work = Path(instance['work'])
        instance['overrides'].update(HALO_DATA_ROOT=str(work), HALO_SAVE_ROOT=str(work/'saves'),
            HALO_NET_ADDRESS=f'127.0.0.{args.address_base+index}',
            HALO_NET_BROADCAST=','.join(f'127.0.0.{args.address_base+i}' for i in (1, 2))
                if index == 0 else f'127.0.0.{args.address_base}',
            HALO_NETWORK_TEST_START='30', HALO_TEST_INPUT='look:party-soak' if args.soak else 'look:party')
    processes, logs, resources, events = [], [], [], []
    start = time.monotonic()
    completed = None
    try:
        for machine, instance in enumerate(manifest['instances']):
            work = Path(instance['work'])
            env = {k:v for k,v in os.environ.items() if not k.upper().startswith('HALO_')}
            env.update(instance['overrides'])
            log = (work/'session.log').open('wb'); logs.append(log)
            process = subprocess.Popen([str(work/'halo.exe')], cwd=work, env=env,
                stdout=log, stderr=subprocess.STDOUT,
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
            processes.append(process); instance['pid'] = process.pid
            (args.root/'manifest.json').write_text(json.dumps(manifest, indent=2))
            deadline = time.monotonic()+25
            marker = f'server added player from machine #{machine} at controller index #0 to the game'
            while time.monotonic() < deadline:
                path = args.root/'host'/'debug.txt'
                if path.exists() and marker in path.read_text(errors='replace'): break
                if process.poll() is not None: raise RuntimeError(f'{instance["name"]} exited: {process.returncode}')
                time.sleep(.25)
            else: raise RuntimeError(f'{instance["name"]} did not register machine {machine}')
        sockets(processes, args.root)
        next_sample = 0
        last_stage, stage_since = None, time.monotonic()
        last_game = len(args.variants.split(','))-1
        while time.monotonic()-start < args.seconds:
            elapsed = time.monotonic()-start
            ended = [{'peer':i['name'], 'exit_code':p.poll()} for i,p in zip(manifest['instances'],processes)
                     if p.poll() is not None]
            if ended:
                events.append({'event':'unexpected_engine_exit','seconds':elapsed,'processes':ended}); break
            if elapsed >= next_sample:
                complete = True
                for instance, process in zip(manifest['instances'], processes):
                    resources.append({'peer':instance['name'], 'elapsed_seconds':round(elapsed,2),
                                      'memory':memory_usage(process.pid)})
                    rows = samples(Path(instance['work'])/'session.log')
                    complete &= any(r['game']==last_game and r['over'] for r in rows)
                next_sample = elapsed+5
                host_text = (args.root/'host'/'session.log').read_text(errors='replace')
                stages = re.findall(r'party stage: game=\d+ stage=\d+', host_text)
                if stages and stages[-1] != last_stage:
                    last_stage, stage_since = stages[-1], time.monotonic()
                if stages and not complete and time.monotonic()-stage_since > args.stage_timeout:
                    events.append({'event':'no_combat_progress','stage':last_stage,'seconds':elapsed}); break
                if complete and completed is None:
                    completed = elapsed; events.append({'event':'all_rounds_completed','seconds':elapsed})
            if completed is not None and elapsed-completed >= 10: break
            time.sleep(.5)
    finally:
        for process in processes:
            if process.poll() is None: process.terminate()
            process.wait(timeout=15)
        for log in logs: log.close()
        (args.root/'resources.json').write_text(json.dumps(resources,indent=2))
        (args.root/'events.json').write_text(json.dumps(events,indent=2))
    return verify(args.root)


def verify(root, write=True):
    manifest = json.loads((root/'manifest.json').read_text())
    report = analyze(root)
    if write: (root/'analysis.json').write_text(json.dumps(report,indent=2))
    errors, rounds = [], []
    def check(condition, label):
        if not condition: errors.append(label)
    check(len(manifest['instances']) == 3 and manifest['protocol_standins'] == 0, 'not three full engines')
    check(len({i.get('pid') for i in manifest['instances']}) == 3, 'full-engine PIDs are not distinct')
    events = json.loads((root/'events.json').read_text())
    check(any(e['event']=='all_rounds_completed' for e in events), 'round sequence incomplete')
    check(not any(e['event']=='unexpected_engine_exit' for e in events), 'unexpected engine exit')
    variants = manifest['variants'].split(',')
    finals = {}
    for machine_index, instance in enumerate(manifest['instances']):
        name = instance['name']; work = Path(instance['work']); rows = samples(work/'session.log')
        text = (work/'session.log').read_text(errors='replace')
        check({r['machine'] for r in rows if r['local']} == {machine_index}, name+': wrong local ownership')
        check(sha(work/'halo.exe') == manifest['binary_sha256'], name+': binary differs')
        check(not report['peers'][name]['exception'], name+': exception')
        check(report['peers'][name]['local_live_bad_camera_samples'] == 0, name+': bad live camera')
        check('infection test: damage' not in text and 'population test: environmental death' not in text,
              name+': forced damage/death fixture')
        for game, variant in enumerate(variants):
            match = [r for r in rows if r['game']==game]
            label = f'{name} game {game} {variant}'
            check(bool(match), label+': absent'); check(any(r['over'] for r in match), label+': unfinished')
            for machine in range(3):
                player = [r for r in match if r['machine']==machine]
                local = [r for r in player if r['local'] and r['alive'] and not r['over']]
                lives = report['peers'][name]['local_death_respawns']
                check(any(r['alive'] and r['attached'] for r in player), label+f': machine {machine} never alive')
                if local:
                    groups = {}
                    for r in local: groups.setdefault(r['unit'],[]).append(r)
                    span = max((math.dist(rs[0]['position'],r['position']) for rs in groups.values() for r in rs), default=0)
                    rounds.append({'peer':name,'game':game,'machine':machine,'movement_span':round(span,3),
                        'kills':max(r['kills'] for r in player),'deaths':max(r['deaths'] for r in player),
                        'local_respawns':sum(r['game']==game for r in lives)})
                    if not variant.startswith('infection'):
                        check(span > 2, label+': local player did not move two world units')
                if not variant.startswith('infection'):
                    try: lifecycle(player,label+f' machine {machine}')
                    except AssertionError as e: errors.append(str(e))
                    check(any(r['kills']>0 and r['deaths']>0 and r['alive'] for r in player),label+': missing real kills/deaths')
                    check(all(not r['skull'] and r['team'] != -1 for r in player if r['alive']),label+': Slayer role leak')
                    check(any(a['unit']==b['unit'] and a['ammo']>b['ammo'] for a,b in zip(player,player[1:])
                              if a['alive'] and b['alive']),label+f': machine {machine} ammo not consumed')
                else:
                    alpha = 0 if variant.endswith('host') else 1
                    active = [r for r in player if r['phase']==2]
                    check(bool(active) and active[0]['team']==(1 if machine==alpha else 0),label+': wrong initial role')
                    if machine in (alpha,1-alpha):
                        try:
                            _,_,after=lifecycle(player,label+f' machine {machine}')
                            check(after['team']==1 and after['skull'] and after['weapons']==1,label+': invalid zombie respawn')
                        except AssertionError as e: errors.append(str(e))
                    check(any(r['deaths']>0 and r['team']==1 for r in player),label+': missing infection/death')
                    check(all(r['skull'] and r['weapons']==1 for r in player if r['alive'] and r['phase']>=2 and r['team']==1),
                          label+': zombie inventory invalid')
                    check(any(r['phase']==3 and r['winner']==1 for r in player),label+': missing zombie victory')
            ended = [r for r in match if r['over']]
            if ended:
                tick=max(r['tick'] for r in ended)
                finals[name,game]=sorted((r['machine'],r['team'],r['kills'],r['deaths'],r['score'],r['winner'])
                                        for r in ended if r['tick']==tick)
            if match and (not variant.startswith('infection') or machine_index != 2):
                field = 'hits_dealt' if machine_index == 0 else 'hits_sent'
                check(max(r[field] for r in match)>min(r[field] for r in match),
                      label+': no new accepted/sent remote damage in this round')
    for game in range(len(variants)):
        check(bool(finals.get(('host',game))) and all(finals.get((name,game))==finals.get(('host',game))
              for name in ('client','client2')),f'game {game}: final statistics disagree')
    result = {'result':'FAIL' if errors else 'PASS','errors':errors,'rounds':rounds,
              'full_engines':3,'protocol_standins':0,'map':manifest['map'],'variants':variants}
    if write: (root/'result.json').write_text(json.dumps(result,indent=2))
    return result


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path); p.add_argument('--maps',type=Path); p.add_argument('--root',type=Path,required=True)
    p.add_argument('--map',default='bloodgulch'); p.add_argument('--variants',default='slayer,infection-combat-client,slayer')
    p.add_argument('--seconds',type=int,default=600); p.add_argument('--address-base',type=int,default=200)
    p.add_argument('--stage-timeout',type=int,default=180)
    p.add_argument('--soak',action='store_true'); p.add_argument('--verify-only',action='store_true')
    args=p.parse_args()
    result=verify(args.root) if args.verify_only else run(args)
    print(json.dumps(result)); raise SystemExit(result['result']!='PASS')
