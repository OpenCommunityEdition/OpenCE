"""Two complete engines: native objectives and natural victories, opt-in only.

The controller places the remote player at existing objectives. No objective
state or score is injected. A labelled fixture death checks native respawn;
weapon combat is covered separately. This is not a navigation/AI test.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time
import test_dual_combat as dual


def verify(root):
    manifest = json.loads((root / 'manifest.json').read_text())
    results = []
    for peer in manifest['instances']:
        path = Path(peer['work']) / 'session.log'
        log = path.read_text(errors='replace')
        assert not re.search(r'EXCEPTION|ASSERTION FAILED|!!! ASSERT', log), peer['name']
        samples = dual.samples(path)
        rows = [{k: int(v) for k, v in re.findall(r'(\w+)=(-?\d+)', line)}
                for line in log.splitlines() if 'native objective: game=' in line]
        for game, name in enumerate(manifest['variants'].split(',')):
            players = [r for r in samples if r['game'] == game]
            assert players and any(r['over'] for r in players), (peer['name'], name, 'unfinished')
            if name.startswith('infection'):
                assert any(r['winner'] == 1 and r['phase'] == 3 for r in players), name
                assert all(r['skull'] and r['weapons'] == 1 and r['grenades'] == [0, 0]
                           for r in players if r['alive'] and r['team'] == 1 and r['phase'] == 2), name
            elif name not in ('slayer', 'team_slayer'):
                match = [r for r in rows if r['game'] == game]
                assert match and all(not r['infection'] and r['reserve'] == 999 for r in match), name
                final = [r for r in match if r['over'] and r['machine'] == 1]
                assert final and max(r['score'] for r in final) > 0 and any(r['won'] == 1 for r in final), (name, final[-2:])
                assert any(r['machine'] == 0 and r['deaths'] >= 1 and not r['alive'] for r in players), 'no death'
                assert any(r['machine'] == 0 and r['deaths'] >= 1 and r['alive'] for r in players), 'no respawn'
                # Native Oddball slows its carrier; Race speeds up players
                # one/two laps behind. Neither is an Infection speed leak.
                native_speeds = {'oddball': (0.75, 1.0), 'race': (1.0, 1.1, 1.2)}.get(name, (1.0,))
                assert all(r['phase'] == -1 and r['speed'] in native_speeds for r in players), name
            if peer['name'] == 'host':
                assert f'inventory profile: PASS game={game}' in log, (name, 'disk variant reload')
            results.append({'peer': peer['name'], 'game': game, 'mode': name, 'result': 'PASS'})
    return {'result': 'PASS', 'full_engines': 2, 'protocol_bots': 0, 'rounds': results,
            'scope': 'Real objective contact/pickup, native score and victory, synchronized postgame, fixture death/respawn and saved variants. Teleported fixture placement; not AI or traversal.'}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--exe', type=Path, default=Path('build/windows/halo.exe'))
    parser.add_argument('--maps', type=Path, default=Path('assets/maps'))
    parser.add_argument('--map', default='bloodgulch')
    parser.add_argument('--variants', default='ctf,oddball,king,race')
    parser.add_argument('--seconds', type=int, default=420)
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    error = None
    if not args.verify_only:
        manifest = dual.prepare(args.exe.resolve(), args.maps, args.root, args.map, args.variants)
        # Inventory fixtures alternate to Blood Gulch on odd rounds.
        import shutil
        for peer in manifest['instances']:
            target = Path(peer['work'])/'maps/bloodgulch.map'
            if not target.exists(): shutil.copy2((args.maps / 'bloodgulch.map'), target)
        for n, peer in enumerate(manifest['instances']):
            peer['overrides'].update(HALO_TEST_INPUT='look:inventory-objectives', HALO_NETWORK_TEST_SCORE='1',
                                     HALO_NET_ADDRESS=f'127.0.0.{240+n}', HALO_NET_BROADCAST=f'127.0.0.{241-n}')
        processes, logs = [], []
        try:
            for peer in manifest['instances']:
                work = Path(peer['work']); log = (work/'session.log').open('wb'); logs.append(log)
                env = {k: v for k, v in os.environ.items() if not k.upper().startswith('HALO_')}
                env.update(peer['overrides'])
                p = subprocess.Popen([str(work/manifest.get('executable', 'halo.exe'))], cwd=work, env=env, stdout=log, stderr=subprocess.STDOUT,
                                     creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
                processes.append(p); peer['pid'] = p.pid
                time.sleep(2)
            (args.root/'manifest.json').write_text(json.dumps(manifest, indent=2))
            deadline = time.monotonic() + args.seconds
            completed = None
            while time.monotonic() < deadline:
                assert all(p.poll() is None for p in processes), ('early exit', [p.poll() for p in processes])
                for peer in manifest['instances']:
                    debug = Path(peer['work'])/'debug.txt'
                    assert not debug.exists() or 'EXCEPTION' not in debug.read_text(errors='replace'), (peer['name'], 'exception in debug.txt')
                done = all(any(r['game'] == len(args.variants.split(','))-1 and r['over']
                               for r in dual.samples(Path(i['work'])/'session.log')) for i in manifest['instances'])
                if done:
                    completed = completed or time.monotonic()
                    if time.monotonic()-completed > 4: break
                time.sleep(1)
        except AssertionError as exc:
            error = str(exc)
        finally:
            for p in processes:
                dual.stop_process(p)
            for log in logs: log.close()
    try:
        assert error is None, error
        result = verify(args.root)
    except AssertionError as exc:
        result = {'result': 'FAIL', 'error': str(exc)}
    output = 'native-objectives-rechecked.json' if args.verify_only else 'native-objectives-result.json'
    (args.root/output).write_text(json.dumps(result, indent=2))
    print(json.dumps(result))
    return int(result['result'] != 'PASS')


if __name__ == '__main__':
    raise SystemExit(main())
