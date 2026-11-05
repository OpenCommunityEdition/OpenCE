"""Run a sustained, isolated Windows bot battle and summarize frame pacing.

One instance per invocation. Use sequential invocations for comparable timings.
Only the dedicated performance executable contains passive measurement hooks.
"""
import argparse
import csv
import ctypes
from ctypes import wintypes
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import time
import tomllib
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
LIGHTS = ['plasma_lighting', 'muzzle_lighting', 'jackal_shield_lighting',
          'covenant_shield_lighting', 'fuel_rod_lighting', 'ammo_counter_lighting',
          'needler_lighting', 'plasma_occlusion', 'shield_glow',
          'weapon_glow', 'vehicle_lighting', 'world_lighting']
QUALITY = ['anti_aliasing', 'anisotropic_filtering', 'shadow_resolution',
           'per_pixel_lighting', 'model_detail', 'hdr', 'hdr_output',
           'viewmodel_shield', 'shield_look', 'fov', 'viewmodel_fov']


def distribution(values, cap):
    ordered = sorted(values)
    if not ordered:
        raise ValueError('No frame samples')
    percentile = lambda p: ordered[max(0, math.ceil(len(ordered)*p)-1)]
    return dict(frames=len(values), average_fps=1000/statistics.mean(values),
                p50_ms=percentile(.5), p95_ms=percentile(.95), p99_ms=percentile(.99),
                worst_ms=ordered[-1], low_1pct_fps=1000/statistics.mean(ordered[-max(1, math.ceil(len(ordered)*.01)):]),
                over_1_5_budget_percent=100*sum(v>1500/cap for v in values)/len(values),
                over_50ms=sum(v>50 for v in values), over_100ms=sum(v>100 for v in values))


def summarize(rows, warmup, seconds, cap, captures=True):
    measured = [r for r in rows if warmup <= r['seconds'] < warmup+seconds and r['frame_ms']>0]
    # Older diagnostic binaries still mark capture slots when readback is off.
    # Such frames must remain in pacing results; there was no capture overhead.
    clean = [r for r in measured if not captures or not r['capture_excluded']]
    result = dict(raw=distribution([r['frame_ms'] for r in measured], cap),
                  pacing=distribution([r['frame_ms'] for r in clean], cap),
                  excluded_capture_frames=len(measured)-len(clean),
                  measured_span=measured[-1]['seconds']-measured[0]['seconds'],
                  selected_max=max(r['selected'] for r in measured),
                  selected_mean=statistics.mean(r['selected'] for r in measured),
                  selected_frame_percent=100*sum(r['selected']>0 for r in measured)/len(measured),
                  scene_lights_max=max(r['scene_lights'] for r in measured),
                  occlusion_updates_max=max(r['occlusion_updates'] for r in measured),
                  occlusion_mean_ms=statistics.mean(r.get('occlusion_ms',0) for r in clean),
                  occlusion_max_ms=max(r.get('occlusion_ms',0) for r in clean),
                  minutes=[])
    for start in range(0, seconds, 60):
        block=[r['frame_ms'] for r in clean if warmup+start <= r['seconds'] < warmup+min(start+60, seconds)]
        if block: result['minutes'].append(dict(start=start, **distribution(block, cap)))
    return result


def process_memory(handle):
    class Counters(ctypes.Structure):
        _fields_ = [('cb', wintypes.DWORD), ('faults', wintypes.DWORD)] + [
            (name, ctypes.c_size_t) for name in ('peak_rss','rss','peak_paged','paged','peak_nonpaged','nonpaged','pagefile','peak_pagefile','private')]
    counters= Counters(); counters.cb=ctypes.sizeof(counters)
    get=ctypes.WinDLL('psapi').GetProcessMemoryInfo
    get.argtypes=[wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD]
    if not get(wintypes.HANDLE(handle), ctypes.byref(counters), counters.cb):
        raise ctypes.WinError()
    return dict(rss=counters.rss, private=counters.private, faults=counters.faults)


def memory_window_mib(samples, start, end=float('inf')):
    values = [m['private'] for m in samples if start <= m['seconds'] < end]
    return statistics.mean(values)/2**20 if values else None


def safe(path, out):
    assert path.resolve().is_relative_to(out) and path.resolve()!=out
    for parent in [path, *path.parents]:
        if parent==out: break
        assert not parent.is_symlink() and not parent.is_junction(), parent
    return path


def cleanup(out):
    for path in out.rglob('*.map'): safe(path, out).unlink()
    saves=[p for p in (out/'saves').rglob('*') if p.is_file()]
    if saves:
        manifest=[dict(path=p.relative_to(out).as_posix(), sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in saves]
        with ZipFile(out/'test-saves.zip', 'x', ZIP_DEFLATED, compresslevel=9) as archive:
            for path, info in zip(saves, manifest): archive.write(path, info['path'])
            archive.writestr('manifest.json', json.dumps(manifest, indent=2))
        with ZipFile(out/'test-saves.zip') as archive:
            for info in manifest: assert hashlib.sha256(archive.read(info['path'])).hexdigest()==info['sha256']
        for path in saves: safe(path, out).unlink()
    from PIL import Image
    for path in (out/'screenshots').glob('*.bmp'):
        target=path.with_suffix('.png')
        with Image.open(path) as source:
            source.save(target)
            with Image.open(target) as saved: assert saved.size==source.size
        safe(path, out).unlink()
    for name in ['halo-performance.exe', 'SDL3.dll']:
        path=out/name
        if path.exists(): safe(path, out).unlink()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--settings', type=Path)
    parser.add_argument('--fps', type=int, choices=[30,60,120], required=True)
    parser.add_argument('--uncapped', action='store_true', help='Measure headroom without the limiter; --fps remains the reporting budget')
    parser.add_argument('--enabled', type=int, choices=[0,1], required=True)
    parser.add_argument('--seconds', type=int, default=300)
    parser.add_argument('--warmup', type=int, default=30)
    parser.add_argument('--resolution', default='2560x1440')
    parser.add_argument('--map', default='carousel', choices=['carousel','bloodgulch'])
    parser.add_argument('--bots', type=int, default=15)
    parser.add_argument('--label', required=True)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'build/lighting-performance-build')
    parser.add_argument('--dump-shaders', action='store_true', help='Retain GLSL instead of screenshots for cache diagnosis; not an FPS acceptance run')
    parser.add_argument('--no-captures', action='store_true', help='Disable screenshot allocations/readback for memory and stall diagnostics')
    args=parser.parse_args()
    assert re.fullmatch(r'[A-Za-z0-9_-]+', args.label)
    assert re.fullmatch(r'[1-9][0-9]*x[1-9][0-9]*', args.resolution)
    assert args.seconds>=30 and args.warmup>=0 and 1<=args.bots<=15
    out=(ROOT/'build/lighting-performance'/args.label).resolve()
    assert out.is_relative_to((ROOT/'build').resolve())
    out.mkdir(parents=True, exist_ok=False)
    (out/'maps').mkdir(); (out/'screenshots').mkdir()
    env={k:v for k,v in os.environ.items() if not k.startswith('HALO_')}
    env['PATH']=str(ROOT/'build/windows')+os.pathsep+env.get('PATH','')
    settings={}
    if args.settings:
        display=tomllib.loads(args.settings.read_text())['display']
        settings={k:display[k] for k in QUALITY if k in display}
    settings.update({k:bool(args.enabled) for k in LIGHTS})
    settings.update(overshield_effect=bool(args.enabled), shield_glow_intensity='default')
    (out/'config.toml').write_text('[display]\n'+''.join(f'{k} = {json.dumps(v)}\n' for k,v in settings.items()))
    env.update(HALO_DATA_ROOT=str(out), HALO_SAVE_ROOT=str(out/'saves'), HALO_HIDDEN_WINDOW='1',
               HALO_WINDOW_SIZE=args.resolution, HALO_RESOLUTION_SCALING='native', HALO_NO_AUDIO='1',
               HALO_MAX_FPS='-1' if args.uncapped else str(args.fps), HALO_NO_VSYNC='1', HALO_EXIT_AFTER=str(args.warmup+args.seconds+25),
               HALO_NET_ONLINE='0', HALO_NET_PUBLIC_LOBBY='0', HALO_NET_ALLOW_UPNP='0',
               HALO_NET_JOIN_FROM_CLIPBOARD='0',
               HALO_NET_ADDRESS=f'127.0.0.{81+args.enabled}', HALO_NET_BROADCAST=f'127.0.0.{81+args.enabled}',
               HALO_UPDATE_ANSWER='never', HALO_CRASH_REPORTS='no', HALO_NETWORK_TEST=f'local:{args.map}:slayer',
               HALO_NETWORK_TEST_START='5', HALO_NETWORK_TEST_SCORE='10000', HALO_BOTS=str(args.bots),
               HALO_BOT_SKILL='heroic', HALO_TEST_INPUT='bot:1', HALO_SCREENSHOT_DIR=str(out/'screenshots'))
    if args.dump_shaders:
        (out/'shaders').mkdir()
        env['HALO_GPU_DUMP_SHADERS']=str(out/'shaders')
        # Diagnose cache growth without periodic readback/large image allocations.
        env['HALO_SCREENSHOT_DIR']=''
    if args.no_captures:
        env['HALO_SCREENSHOT_DIR']=''
    build=args.build_dir.resolve()
    assert build.is_relative_to((ROOT/'build').resolve())
    shutil.copy2(build/'manifest.json', out/'build-manifest.json')
    (out/'settings.json').write_text(json.dumps(dict(resolution=args.resolution, fps=args.fps, uncapped=args.uncapped,
        frame_limit=int(env['HALO_MAX_FPS']),
        warmup=args.warmup, seconds=args.seconds, bots=args.bots, map=args.map,
        shader_dump=args.dump_shaders, captures=not (args.dump_shaders or args.no_captures), display=settings),indent=2))
    memory=[]; start=time.monotonic(); process=None
    try:
        for name in ['ui.map', args.map+'.map']:
            shutil.copy2(args.data_root/'maps'/name, out/'maps'/name)
        # Desktop configuration is next to the executable, not the data root.
        shutil.copy2(build/'halo-performance.exe', out/'halo-performance.exe')
        shutil.copy2(ROOT/'build/windows/SDL3.dll', out/'SDL3.dll')
        with (out/'stdout.log').open('wb') as stdout, (out/'stderr.log').open('wb') as stderr:
            process=subprocess.Popen([str(out/'halo-performance.exe')], cwd=out, env=env,
                stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
            try:
                while process.poll() is None:
                    if time.monotonic()-start>args.warmup+args.seconds+120:
                        raise TimeoutError('Game did not exit')
                    memory.append(dict(seconds=time.monotonic()-start, **process_memory(process._handle)))
                    (out/'memory.json').write_text(json.dumps(memory,indent=2))
                    time.sleep(5)
            finally:
                if process.poll() is None: process.terminate(); process.wait(10)
        rows=[]
        if (out/'frames.csv').exists():
            with (out/'frames.csv').open() as frames:
                rows=[{k:float(v) for k,v in row.items()} for row in csv.DictReader(frames)]
        report={}
        try:
            report=summarize(rows,args.warmup,args.seconds,args.fps,
                             captures=not (args.dump_shaders or args.no_captures))
        except ValueError as error:
            report['measurement_error']=str(error)
        log=(out/'stderr.log').read_text(errors='replace')
        report.update(exit=process.returncode, fps=args.fps, uncapped=args.uncapped, frame_limit=int(env['HALO_MAX_FPS']), enabled=args.enabled, shader_dump=args.dump_shaders,
                      captures=not (args.dump_shaders or args.no_captures),
                      unexpected_players=bool(re.search(r'\bplayer '+str(args.bots+1)+r':', log)),
                      bots_lines=[line for line in log.splitlines() if 'bots: tick' in line],
                      errors=re.findall(r'(?im)^.*(?:crash: exception|exception halt|assertion failed|GL_INVALID|shader.*(?:error|failed)).*$',log),
                      peak_private_mib=max((m['private']/2**20 for m in memory),default=None),
                      peak_rss_mib=max((m['rss']/2**20 for m in memory),default=None),
                      # Memory uses the runner clock. Bound both windows before
                      # the exit deadline; shutdown deallocation is not a plateau.
                      first_minute_private_mib=memory_window_mib(memory,args.warmup+15,min(args.warmup+75,args.warmup+args.seconds)),
                      last_minute_private_mib=memory_window_mib(memory,max(args.warmup+15,args.warmup+args.seconds-60),args.warmup+args.seconds))
        (out/'report.json').write_text(json.dumps(report,indent=2))
        print(json.dumps(dict(label=args.label, **report)),flush=True)
        assert report['exit']==0 and not report['errors'], 'Game failed; retained report and logs describe the failure'
        assert not report.get('measurement_error') and report['measured_span']>=args.seconds-.2, 'Incomplete measurement'
        assert not report['unexpected_players'], 'Unexpected player joined the isolated workload'
        assert bool(report['selected_max'])==bool(args.enabled), 'Light activation mismatch'
        if args.seconds>=120: assert len(report['bots_lines'])>=args.seconds//60-1, 'Insufficient combat telemetry'
    finally:
        cleanup(out)


if __name__=='__main__':
    main()
