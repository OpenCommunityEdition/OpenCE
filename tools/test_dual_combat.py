"""Two full OpenCE processes: isolated LAN, real controller combat, telemetry.

Run as a script with --exe, --maps, --root (a NEW directory). --manual creates
two reusable launchers without starting them. No user processes are stopped.
"""
import argparse
import hashlib
import json
import math
import os
import re
import shlex
import statistics
from pathlib import Path
import shutil
import subprocess
import time


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stop_process(process):
    if process.poll() is None:
        process.terminate()
    try:
        process.wait(timeout=15)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=15)


def prepare(exe, maps, root, map_name, variants, manual=False, render=False, capture_every=0, resolution='800x600'):
    root.mkdir(parents=True, exist_ok=False)
    executable = 'halo.exe' if exe.suffix.lower() == '.exe' else 'halo'
    manifest = {'binary_sha256': sha(exe), 'map': map_name, 'variants': variants,
                'manual': manual, 'render': render, 'executable': executable, 'instances': []}
    for index, name in enumerate(('host', 'client')):
        work = root / name
        (work / 'maps').mkdir(parents=True)
        for filename in dict.fromkeys(('ui.map', 'carousel.map', map_name + '.map')):
            shutil.copy2(maps / filename, work / 'maps' / filename)
        shutil.copy2(exe, work / executable)
        library = 'SDL3.dll' if executable == 'halo.exe' else 'libSDL3.so.0'
        shutil.copy2(exe.with_name(library), work / library)
        assert sha(work / executable) == manifest['binary_sha256']
        (work / 'config.toml').write_text('[crash_reports]\nupload="no"\n[update]\nauto=false\n'
            '[discord]\napplication_id=""\n[network]\nonline=false\njoin_from_clipboard=false\n'
            '[display]\n' + ('' if manual else 'high_res_hud=false\nhigh_res_text=false\n'))
        overrides = dict(HALO_DATA_ROOT=str(work), HALO_SAVE_ROOT=str(work / 'saves'),
            HALO_NET_ADDRESS=f'127.0.0.{200+index}', HALO_NET_BROADCAST=f'127.0.0.{201-index}',
            HALO_NET_ONLINE='false', HALO_NET_JOIN_FROM_CLIPBOARD='false',
            HALO_NETWORK_TEST='' if manual else ('host:' + map_name + ':' + variants if index == 0 else 'join'),
            HALO_NETWORK_TEST_START='12', HALO_NETWORK_TEST_KILL='0', HALO_NETWORK_TEST_SHOOT='0',
            HALO_NETWORK_TEST_PICKUP='0', HALO_NETWORK_TEST_VEHICLE='0', HALO_NETWORK_TEST_SCORE='0',
            HALO_NETWORK_LOSS='0', HALO_NETWORK_LATENCY='0', HALO_NETWORK_CORRUPT='0', HALO_NETWORK_CORRUPT_STREAM='0',
            HALO_TEST_INPUT='' if manual else 'look:combat', HALO_MENU_OPEN='', HALO_EXIT_AFTER='0',
            HALO_NULL_RENDERER='0' if manual or render else '1', HALO_HIDDEN_WINDOW='0' if manual else '1')
        if manual or render:
            overrides.update(HALO_DISPLAY_MODE='windowed', HALO_WINDOW_SIZE=resolution)
            if render and capture_every:
                (work / 'frames').mkdir()
                overrides.update(HALO_SCREENSHOT_DIR=str(work / 'frames'), HALO_SCREENSHOT_EVERY=str(capture_every))
        command = '@echo off\r\nsetlocal\r\nfor /f "tokens=1 delims==" %%K in (\'set HALO_ 2^>nul\') do set "%%K="\r\n'
        command += ''.join(f'set "{k}={v}"\r\n' for k, v in overrides.items())
        command += 'cd /d "%~dp0"\r\n"%~dp0halo.exe" >> "%~dp0manual-session.log" 2>&1\r\n'
        if executable == 'halo.exe':
            (work / ('Play-' + name + '.cmd')).write_bytes(command.encode('utf-8'))
        else:
            command = '#!/bin/sh\ncd -- "$(dirname -- "$0")"\nexec env '
            command += ' '.join(shlex.quote(k + '=' + v) for k, v in overrides.items())
            command += ' ./halo >> manual-session.log 2>&1\n'
            launcher = work / ('Play-' + name + '.sh')
            launcher.write_text(command)
            launcher.chmod(0o755)
        manifest['instances'].append({'name': name, 'work': str(work), 'overrides': overrides})
    (root / 'manifest.json').write_text(json.dumps(manifest, indent=2))
    return manifest


def sockets(processes, root):
    if os.name != 'nt':
        return
    ids = ','.join(str(p.pid) for p in processes)
    command = ('$idsForTest=@(' + ids + '); @(' +
        'Get-NetTCPConnection -ErrorAction SilentlyContinue | Where-Object { $_.OwningProcess -in $idsForTest } | '
        'Select-Object @{N="Transport";E={"TCP"}},OwningProcess,LocalAddress,LocalPort,RemoteAddress,RemotePort,State; '
        'Get-NetUDPEndpoint -ErrorAction SilentlyContinue | Where-Object { $_.OwningProcess -in $idsForTest } | '
        'Select-Object @{N="Transport";E={"UDP"}},OwningProcess,LocalAddress,LocalPort) | ConvertTo-Json -Depth 3')
    result = subprocess.run(['powershell', '-NoProfile', '-Command', command], capture_output=True, timeout=15)
    (root / 'sockets.json').write_bytes(result.stdout)


def run(manifest, root, seconds):
    processes, handles = [], []
    flags = subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
    try:
        for instance in manifest['instances']:
            work = Path(instance['work'])
            env = os.environ.copy()
            # Keep unrelated process environment, but remove inherited Halo
            # debug/transport overrides before applying this fixture's values.
            env = {k: v for k, v in env.items() if not k.startswith('HALO_')}
            env.update(instance['overrides'])
            log = (work / 'session.log').open('wb')
            handles.append(log)
            process = subprocess.Popen([str(work / manifest.get('executable', 'halo.exe'))], cwd=work, env=env,
                                       stdout=log, stderr=subprocess.STDOUT, creationflags=flags)
            processes.append(process)
            instance['pid'] = process.pid
            time.sleep(2)
        (root / 'manifest.json').write_text(json.dumps(manifest, indent=2))
        started = time.monotonic()
        captured = False
        completed_at = None
        last_game = len(manifest['variants'].split(',')) - 1
        while time.monotonic() - started < seconds:
            if any(p.poll() is not None for p in processes):
                raise AssertionError('game process exited before the deadline: ' +
                    str([(i['name'], p.poll()) for i,p in zip(manifest['instances'], processes)]))
            if not captured and time.monotonic() - started > 25:
                sockets(processes, root)
                captured = True
            if captured:
                complete = True
                for instance in manifest['instances']:
                    latest = samples(Path(instance['work']) / 'session.log')[-2:]
                    complete &= len(latest) == 2 and {r['machine'] for r in latest} == {0, 1} and all(
                        r['game'] == last_game and r['over'] and r['alive'] and r['deaths'] == 1 for r in latest)
                if complete:
                    completed_at = completed_at or time.monotonic()
                    if time.monotonic() - completed_at >= 3:
                        break
                else:
                    completed_at = None
            time.sleep(.5)
    finally:
        for process in processes:
            stop_process(process)
        for log in handles:
            log.close()


def samples(path):
    result = []
    for line in path.read_text(errors='replace').splitlines():
        if 'combat sample: ' in line:
            # A running process may still be writing the final line.
            payload = line.split('combat sample: ', 1)[1]
            if payload.endswith('}'):
                result.append(json.loads(payload))
    return result


def lifecycle(rows, label):
    """Require live -> counted death/no unit -> a different, live attached unit."""
    before = next((r for r in rows if r['alive'] and r['deaths'] == 0), None)
    assert before, label + ': no initial live unit'
    dead = next((r for r in rows if r['tick'] > before['tick'] and not r['alive'] and r['deaths'] == 1), None)
    assert dead, label + ': no observed counted death'
    after = next((r for r in rows if r['tick'] > dead['tick'] and r['alive'] and r['attached']
                  and r['deaths'] == 1 and r['unit'] != before['unit']), None)
    assert after, label + ': no live respawn with a new unit'
    return before, dead, after


def verify(root):
    peers = [samples(root / name / 'session.log') for name in ('host', 'client')]
    result = {'result': 'PASS', 'samples': [len(p) for p in peers], 'peers': {}, 'rounds': []}
    manifest = json.loads((root / 'manifest.json').read_text())
    variants = manifest['variants'].split(',')
    assert len({i['pid'] for i in manifest['instances']}) == 2
    for instance in manifest['instances']:
        assert sha(Path(instance['work']) / manifest.get('executable', 'halo.exe')) == manifest['binary_sha256']
        for key in ('HALO_NETWORK_TEST_KILL', 'HALO_NETWORK_TEST_SHOOT', 'HALO_NETWORK_TEST_SCORE'):
            assert instance['overrides'][key] == '0', 'synthetic combat/scoring driver enabled'
    if os.name == 'nt':
        endpoints = json.loads((root / 'sockets.json').read_text())
        host, client = manifest['instances']
        assert any(e['OwningProcess'] == host['pid'] and e['LocalAddress'] == host['overrides']['HALO_NET_ADDRESS']
                   and e['LocalPort'] == 5150 for e in endpoints), 'host is not bound to its isolated address'
        assert any(e['OwningProcess'] == client['pid'] and e['LocalAddress'] == client['overrides']['HALO_NET_ADDRESS']
                   and e.get('RemoteAddress') == host['overrides']['HALO_NET_ADDRESS'] and e.get('RemotePort') == 5150
                   for e in endpoints), 'client connected to a different host'
    for name, rows in zip(('host', 'client'), peers):
        assert rows, f'{name}: no real-input combat telemetry (old build or inactive driver)'
        log = (root / name / 'session.log').read_text(errors='replace')
        assert 'ASSERTION FAILED' not in log and 'crash: exception' not in log
        assert 'infection test: damage' not in log and 'queued environmental death' not in log
        assert not re.search(r'network test: player \d+ shoots player', log)
        live = [r for r in rows if r['alive'] and r['tick'] > 150 and not r['over']]
        assert live and all(r['attached'] for r in live), f'{name}: invalid player/unit attachment'
        local = [r for r in live if r['local']]
        if manifest['instances'][0]['overrides']['HALO_TEST_INPUT'] == 'look:classic':
            # The fixture relocates units exactly at 1050, after movement.
            # Camera was sampled before that diagnostic relocation this tick.
            for r in [r for r in local if r['tick']==1050]:
                following = next((n for n in local if n['game']==r['game'] and n['tick']==1080),None)
                assert following and following['camera_cluster']>=0 and following['camera_distance']<3, 'camera failed to recover after fixture relocation'
            local = [r for r in local if r['tick']!=1050]
        assert local and all(r['camera_cluster'] >= 0 and math.isfinite(r['camera_distance'])
                             and r['camera_distance'] < 3 for r in local), f'{name}: camera outside live unit'
        pings = [r['ping'] for r in live if r['machine'] == 1 and r['ping'] >= 0]
        assert pings and max(pings) < 999, f'{name}: RTT missing or >= 999 ms on loopback'
        result['peers'][name] = {'rtt_ms_min_median_max': [min(pings), statistics.median(pings), max(pings)],
            'camera_distance_max': max(r['camera_distance'] for r in local),
            'camera_clusters': sorted({r['camera_cluster'] for r in local}),
            'bsp': sorted({r['bsp'] for r in live})}
        for game, variant in enumerate(variants):
            if variant not in ('slayer', 'team_slayer'):
                continue
            for machine in (0, 1):
                life = [r for r in rows if r['game'] == game and r['machine'] == machine]
                before, dead, after = lifecycle(life, f'{name}: Slayer {game} machine {machine}')
                if variant == 'team_slayer':
                    opponents = [r for r in rows if r['game'] == game and r['machine'] != machine and r['alive']]
                    assert opponents and before['team'] != opponents[0]['team'], 'Team Slayer opponents share a team'
                assert any(r['kills'] == r['deaths'] == 1 and r['alive'] for r in life), 'Slayer kill counters missing'
                assert any(a['unit'] == b['unit'] and a['weapon'] == b['weapon'] and a['ammo'] > b['ammo']
                           for a, b in zip(life, life[1:]) if a['alive'] and b['alive']), 'gun did not consume ammo'
                assert before['weapons'] == after['weapons'] == 2 and not after['skull'], 'Infection loadout leaked into Slayer'
                result['rounds'].append({'peer': name, 'game': game, 'mode': variant, 'machine': machine,
                    'death_respawn_ticks': [dead['tick'], after['tick']],
                    'nonlethal_damage_sampled': any(r['alive'] and (r['body'] < 1 or r['shield'] < 1) for r in life)})
        infection_rounds = []
        for game, variant in enumerate(variants):
            if variant == 'infection-combat-host': infection_rounds.append((game, 0))
            elif variant == 'infection-combat-client': infection_rounds.append((game, 1))
            elif variant.startswith('infection-classic-'):
                infection_rounds.append((game, int(variant.split('-')[4])))
        for game, alpha in infection_rounds:
            match = [r for r in rows if r['game'] == game]
            zombie = [r for r in match if r['machine'] == alpha and r['phase'] >= 2]
            human = [r for r in match if r['machine'] != alpha and r['phase'] >= 2]
            zbefore, zdead, zafter = lifecycle(zombie, f'{name}: game {game} alpha')
            hbefore, hdead, hafter = lifecycle(human, f'{name}: game {game} human')
            assert zbefore['team'] == 1 and hbefore['team'] == 0, 'wrong initial zombie'
            # Samples are one second apart: respawn and a close melee kill can share a sample.
            assert zdead['tick'] < zafter['tick'] <= hdead['tick'], 'human converted before alpha respawn/melee'
            assert hafter['team'] == 1 and hafter['skull'] and hafter['weapons'] == 1, 'human loadout remained after conversion'
            assert hbefore['weapons'] == 2 and not hbefore['skull'], 'human did not start with normal weapons'
            for r in zombie + human:
                if not r['alive']:
                    continue
                # The assigned colour is the base value. Output slot zero
                # can be scaled to black by the model's function channels;
                # a rendered frame is checked separately for visible armour.
                red, green, blue = r['base_color']
                assert (blue > red if r['team'] else red > blue), f'{name}: wrong assigned armour color'
                if r['team'] == 1:
                    assert r['skull'] and r['weapons'] == 1, 'zombie not holding only its skull'
            assert any(r['phase'] == 3 and r['winner'] == 1 for r in match), 'no zombie victory'
            if name == 'host':
                # In game 1 the remote human shoots; in game 2 the remote
                # zombie must submit its real melee hit to host authority.
                baseline = zdead if alpha == 1 else zbefore
                end = hafter if alpha == 1 else zafter
                assert end['hits_dealt'] > baseline['hits_dealt'], 'host did not accept remote combat damage'
            result['rounds'].append({'peer': name, 'game': game, 'alpha_machine': alpha,
                'alpha_death_respawn_ticks': [zdead['tick'], zafter['tick']],
                'human_death_respawn_ticks': [hdead['tick'], hafter['tick']], 'winner': 1})
    # Compare stable postgame statistics, not transient predicted frames.
    for game in range(len(variants)):
        finals = []
        for rows in peers:
            final = {}
            for machine in (0, 1):
                ended = [r for r in rows if r['game'] == game and r['machine'] == machine and r['over']]
                assert ended, 'missing postgame snapshot'
                last = ended[-1]
                final[machine] = {k: last[k] for k in ('kills', 'deaths', 'score', 'team', 'winner')}
            finals.append(final)
        assert finals[0] == finals[1], f'game {game}: peers disagree on postgame statistics'
    indexed = [{(r['game'], r['tick'], r['machine']): r for r in rows
                if r['alive'] and not r['over'] and r['tick'] > 150} for rows in peers]
    common = indexed[0].keys() & indexed[1].keys()
    assert common, 'no simultaneous live snapshots to compare'
    paired = [(indexed[0][key], indexed[1][key]) for key in common]
    result['synchronization'] = {
        'paired_live_snapshots': len(paired),
        'team_disagreements': sum(a['team'] != b['team'] for a, b in paired),
        'position_difference_max': round(max(math.dist(a['position'], b['position']) for a, b in paired), 3),
        'health_difference_max': round(max(abs(a['body'] - b['body']) for a, b in paired), 3),
        'postgame_statistics_agree': True,
    }
    result['binary_sha256'] = manifest['binary_sha256']
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--map', default='beavercreek')
    parser.add_argument('--variants', default='slayer,infection-combat-host,infection-combat-client,slayer')
    parser.add_argument('--seconds', type=int, default=180)
    parser.add_argument('--manual', action='store_true')
    parser.add_argument('--render', action='store_true', help='Render in hidden windows (normal menu windows with --manual)')
    parser.add_argument('--capture-every', type=int, default=0, help='With --render, save every Nth frame')
    parser.add_argument('--resolution', default='800x600', help='Rendered window size, e.g. 1280x720')
    args = parser.parse_args()
    manifest = prepare(args.exe, args.maps, args.root, args.map, args.variants, args.manual, args.render, args.capture_every, args.resolution)
    if args.manual:
        print('Manual launchers:', args.root)
        return
    run(manifest, args.root, args.seconds)
    result = verify(args.root)
    (args.root / 'result.json').write_text(json.dumps(result, indent=2))
    print(json.dumps(result))


if __name__ == '__main__':
    main()
