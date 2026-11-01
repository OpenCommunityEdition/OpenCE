"""Run one hidden, isolated renderer probe; retain timings and verified saves.

Data must be a disposable copy under this checkout's build directory. Fixed
steps provide the same input cues per tick; compare simulation logs before
claiming identical combat. Neither this tool nor its cache seed controls the
driver's separate shader cache.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import zipfile


def compare_player_traces(reference, current, first=30, last=1200):
    def samples(log):
        # Machine slots differ between builds with different bot capacities.
        # Positions, aim, health, inventory, animation and scores remain exact.
        return {int(match[1]): re.sub(r' m\d+(?= |$)', ' m<slot>', match[2])
                for match in re.finditer(r'network test: tick (\d+) (player 0:.*?) \| items', log)
                if first <= int(match[1]) <= last}
    left, right = samples(reference), samples(current)
    common = left.keys() & right.keys()
    identical = sum(left[tick] == right[tick] for tick in common)
    return dict(compared=len(common), identical=identical,
                missing=len(left.keys() ^ right.keys()),
                matched=bool(common) and identical == len(common) and left.keys() == right.keys())


def summarize(rows, first=600, last=1200):
    samples = [dict(row, frame_ms=(row['ns']-before['ns'])/1e6)
               for before, row in zip(rows, rows[1:])
               if row['playing'] and before['playing'] and first <= row['tick'] <= last]
    if not samples:
        return {}
    values = sorted(r['frame_ms'] for r in samples)
    return dict(frames=len(values), mean_ms=statistics.mean(values), p99_ms=values[int(len(values)*.99)],
                worst_ms=max(values), over16=sum(v>16.667 for v in values), over50=sum(v>50 for v in values),
                worst_frames=sorted(samples, key=lambda r:r['frame_ms'], reverse=True)[:12],
                totals={k:sum(r[k] for r in samples) for k in rows[0] if k.endswith(('_ns', '_calls'))})


def run(args):
    root = Path(__file__).resolve().parents[1]
    if not re.fullmatch(r'[1-9][0-9]*x[1-9][0-9]*', args.resolution):
        raise ValueError('Resolution must be WIDTHxHEIGHT')
    out = args.output.resolve(); data = args.data.resolve(); binary = args.binary.resolve()
    assert out.is_relative_to(root/'build') and data.is_relative_to(root/'build')
    out.mkdir(parents=True, exist_ok=False)
    debug = data/'debug.txt'
    debug_offset = debug.stat().st_size if debug.exists() else 0
    cache = out/'saves/shader_cache'; cache.mkdir(parents=True)
    if args.seed:
        with zipfile.ZipFile(args.seed) as archive:
            for name in archive.namelist():
                if name.startswith('saves/shader_cache/') and name.endswith('.bin'):
                    target = out/name
                    assert target.resolve().is_relative_to(cache)
                    target.write_bytes(archive.read(name))
    for name in ['halo.exe', 'SDL3.dll']:
        shutil.copy2(binary.parent/name, out/name)
    display = dict(shader_prepare=args.prepare, mcc_texture_prepare_mb=128 if args.prepare else 0,
                   texture_prepare_mb=128 if args.prepare else 0,
                   shield_look='classic_honeycomb2' if args.honeycomb else 'classic',
                   per_pixel_lighting=False, high_res_hud=False, viewmodel_shield=args.honeycomb,
                   first_person_legs=False, anti_aliasing='off', shadow_resolution=128,
                   anisotropic_filtering=1, model_detail='stock')
    if args.quality:
        display.update(per_pixel_lighting=True, anti_aliasing='msaa8x', shadow_resolution=1024,
                       anisotropic_filtering=16, model_detail='highest', first_person_legs=True,
                       viewmodel_shield=True, overshield_effect=True)
    (out/'config.toml').write_text('[display]\n'+''.join(f'{k} = {json.dumps(v)}\n' for k,v in display.items()))
    env = {k:v for k,v in os.environ.items() if not k.startswith('HALO_')}
    env.update(HALO_DATA_ROOT=str(data), HALO_SAVE_ROOT=str(out/'saves'), HALO_HIDDEN_WINDOW='1',
               HALO_DISPLAY_MODE='windowed', HALO_WINDOW_SIZE=args.resolution, HALO_MAX_FPS='120', HALO_NO_VSYNC='1',
               HALO_NO_AUDIO='1', HALO_EXIT_AFTER=str(args.seconds), HALO_FIXED_STEP='1',
               HALO_NET_ONLINE='0', HALO_NET_PUBLIC_LOBBY='0', HALO_NET_ALLOW_UPNP='0',
               HALO_NET_JOIN_FROM_CLIPBOARD='0', HALO_DISCORD='0', HALO_NET_ADDRESS='127.0.0.1',
               HALO_NET_BROADCAST='127.0.0.1', HALO_UPDATE_ANSWER='never', HALO_CRASH_REPORTS='no',
               HALO_NETWORK_TEST='local:'+args.map.replace('/', '\\')+':slayer',
               HALO_NETWORK_TEST_START='5', HALO_NETWORK_TEST_SCORE='10000', HALO_BOTS='7',
               HALO_BOT_SKILL='heroic', HALO_TEST_INPUT='bot:1', HALO_GPU_STATS='1')
    for setting in ['PLASMA_LIGHTING','MUZZLE_LIGHTING','JACKAL_SHIELD_LIGHTING','COVENANT_SHIELD_LIGHTING',
                    'FUEL_ROD_LIGHTING','AMMO_COUNTER_LIGHTING','NEEDLER_LIGHTING','PLASMA_OCCLUSION','SHIELD_GLOW']:
        env['HALO_'+setting] = '0'
    if args.lighting:
        env['HALO_SHIELD_GLOW'] = '1'
    (out/'settings.json').write_text(json.dumps(dict(display=display, environment={k:v for k,v in env.items() if k.startswith('HALO_')},
        executable_sha256=hashlib.sha256(binary.read_bytes()).hexdigest()), indent=2))
    process = None
    try:
        with (out/'game.log').open('wb') as log:
            process = subprocess.Popen([str(out/'halo.exe')], cwd=out, env=env, stdout=log, stderr=log,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            (out/'pid.txt').write_text(str(process.pid))
            process.wait(timeout=args.seconds+90)
        report = dict(exit=process.returncode)
        if args.compare:
            report['player_trace_comparison'] = compare_player_traces(
                args.compare.read_text(errors='replace'), (out/'game.log').read_text(errors='replace'))
        if (out/'frames.csv').exists():
            with (out/'frames.csv').open() as file:
                rows = [{k:int(v) for k,v in row.items()} for row in csv.DictReader(file)]
            report['settled'] = summarize(rows)
            report['gameplay'] = summarize(rows, first=1, last=1200)
            report['max_tick'] = max((row['tick'] for row in rows if row['playing']), default=-1)
        (out/'report.json').write_text(json.dumps(report, indent=2))
        print(json.dumps(dict(exit=report['exit'], **{k:v for k,v in report.get('settled', {}).items()
                                                    if k not in ('worst_frames', 'totals')})), flush=True)
    finally:
        if process and process.poll() is None:
            process.terminate(); process.wait(timeout=10)
        if debug.exists():
            with debug.open('rb') as log:
                log.seek(debug_offset)
                (out/'debug.txt').write_bytes(log.read())
        if (data/'gamestate.txt').exists():
            shutil.copy2(data/'gamestate.txt', out/'gamestate.txt')
        saves = [p for p in (out/'saves').rglob('*') if p.is_file() and not (p.suffix=='.map' and p.name.startswith('cache'))]
        manifest = [dict(path=p.relative_to(out).as_posix(), sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in saves]
        with zipfile.ZipFile(out/'saves.zip', 'x', zipfile.ZIP_DEFLATED) as archive:
            for path, item in zip(saves, manifest): archive.write(path, item['path'])
            archive.writestr('manifest.json', json.dumps(manifest, indent=2))
        with zipfile.ZipFile(out/'saves.zip') as archive:
            for item in manifest: assert hashlib.sha256(archive.read(item['path'])).hexdigest() == item['sha256']
        for path in saves+list((out/'saves').rglob('cache*.map'))+[out/'halo.exe', out/'SDL3.dll']:
            assert path.resolve().is_relative_to(out) and not path.is_symlink()
            assert all(not p.is_junction() for p in [path, *path.parents] if p.is_relative_to(out))
            path.unlink()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--map', default='custom_maps/deathisland')
    parser.add_argument('--seconds', type=int, default=60)
    parser.add_argument('--resolution', default='1920x1080')
    parser.add_argument('--seed', type=Path)
    parser.add_argument('--compare', type=Path, help='Reference game.log; compare sampled player state before interpreting timings')
    parser.add_argument('--prepare', action='store_true')
    parser.add_argument('--honeycomb', action='store_true')
    parser.add_argument('--quality', action='store_true', help='Match the reported high-quality display settings')
    parser.add_argument('--lighting', action='store_true', help='Enable Shield Glow, as in the reported settings')
    run(parser.parse_args())
