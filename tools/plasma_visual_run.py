"""Run the capture-only build with isolated data, retaining images and timing.

Example: python tools/plasma_visual_run.py --data-root C:/Games/GulchCE --fps 60 --enabled 1
Only this runner's copied maps and generated map caches are removed.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import statistics

parser = argparse.ArgumentParser()
parser.add_argument('--data-root', required=True, type=Path)
parser.add_argument('--capture-dir', type=Path, help='Directory containing halo-capture.exe and its build manifest')
parser.add_argument('--fps', required=True, type=int, choices=[30, 60, 120, 240])
parser.add_argument('--enabled', required=True, type=int, choices=[0, 1])
parser.add_argument('--label', default='')
parser.add_argument('--map', choices=['bloodgulch', 'carousel', 'a10', 'b40', 'c40', 'ce-deathisland', 'mcc-digsite', 'mcc-ruby-c40'])
parser.add_argument('--kind', choices=['rifle','pistol','charged','rocket','needle','ghost','banshee','banshee-fuel','shade','fuel','wraith','mp-needle','mp-banshee-fuel','mp-shade','ce-fuel','grenade'], default='rifle')
parser.add_argument('--muzzle', choices=['ar','handgun','shotgun','sniper','rifle','pistol','charged','needler','rocket','flamethrower','warthog','scorpion'])
parser.add_argument('--both-lighting', action='store_true')
parser.add_argument('--prong-lighting', action='store_true', help='Isolate the standalone plasma prong spill in a rifle/pistol combat capture')
parser.add_argument('--weapon-glow', action='store_true', help='Exercise authored sword and charge lights')
parser.add_argument('--emitter', choices=['sword-elite','jackal','major','fuel','hunter','stationary','ammo-fp','ammo-marine','ammo-player','needler-fp','needler-marine','needler-player','needler-grunt'])
parser.add_argument('--ambient', choices=['ghost','banshee','dropship','warthog','scorpion','overshield','camo','teleporter'])
parser.add_argument('--shield', choices=['normal', 'charge', 'fp', 'elite'])
parser.add_argument('--glow-intensity', choices=['default','light_show'], default='default')
parser.add_argument('--shield-look', default='classic', choices=['classic','enhanced','classic_with_sprinkles','classic_honeycomb2'])
parser.add_argument('--turn', action='store_true', help='Capture-only yaw/pitch sweep for light orientation checks')
parser.add_argument('--hide-viewmodel', action='store_true', help='Keep the local rifle from obscuring third-person receivers')
parser.add_argument('--face-camera', action='store_true', help='Face a held-weapon fixture toward the camera to inspect torso and hand spill')
parser.add_argument('--grounded', action='store_true', help='Place the held-weapon actor on the actual floor')
parser.add_argument('--crouch', action='store_true', help='Exercise a held-weapon actor crouching and standing again')
parser.add_argument('--exterior', action='store_true', help='View the vehicle fixture from outside to inspect chassis blocking')
parser.add_argument('--sparse-images', action='store_true', help='Retain one image every three simulation ticks')
parser.add_argument('--image-step', type=int, choices=range(1,31), default=3, help='Simulation ticks between sparse captures')
parser.add_argument('--counter-detail', action='store_true', help='View a held world-model AR display over its holder\'s shoulder')
parser.add_argument('--resolution', default='640x480', help='Capture window size, for example 1280x720')
parser.add_argument('--mixed', action='store_true')
parser.add_argument('--model', action='store_true')
parser.add_argument('--occlusion', type=int, choices=[0,1], default=0)
parser.add_argument('--probe', action='store_true')
parser.add_argument('--no-images', action='store_true', help='Measure without screenshot readback or disk writes')
parser.add_argument('--combat', action='store_true')
parser.add_argument('--door', action='store_true')
parser.add_argument('--toggle', action='store_true')
parser.add_argument('--span', type=float)
parser.add_argument('--burst', type=int, choices=range(1,33), default=1)
parser.add_argument('--specular-only', action='store_true')
parser.add_argument('--per-pixel', action='store_true')
parser.add_argument('--distance', type=float)
parser.add_argument('--height', type=float)
parser.add_argument('--drop', type=float)
parser.add_argument('--receiver-distance', type=float)
args = parser.parse_args()
if args.prong_lighting:
    if not args.combat or args.kind not in ('rifle', 'pistol') or args.weapon_glow or args.muzzle:
        parser.error('--prong-lighting requires rifle/pistol --combat, without other weapon-light options')
if args.toggle and (args.emitter or args.ambient or args.shield or args.muzzle):
    parser.error('--toggle requires a projectile capture; emitter, shield and muzzle fixtures use separate tick handlers')
if not re.fullmatch(r'[1-9][0-9]{2,3}x[1-9][0-9]{2,3}', args.resolution):
    parser.error('--resolution must be WIDTHxHEIGHT')
if args.counter_detail and args.emitter not in ('ammo-player', 'ammo-marine'):
    parser.error('--counter-detail requires a third-person AR holder')
if args.exterior and args.muzzle not in ('warthog', 'scorpion'):
    parser.error('--exterior requires --muzzle warthog or scorpion')
if args.crouch and not args.shield and args.emitter not in ('ammo-player', 'ammo-marine', 'needler-player', 'needler-marine'):
    parser.error('--crouch requires a third-person player or Marine emitter')
if args.map is None:
    # Blood Gulch has no machine door for the moving-occluder fixture.
    args.map = 'a10' if args.door else 'bloodgulch'
if args.muzzle:
    args.combat = True
root = Path(__file__).resolve().parents[1]
assert re.fullmatch(r'[a-zA-Z0-9_-]*', args.label)
out = root / f'build/plasma-visual/{args.fps}-{args.enabled}{"-" + args.label if args.label else ""}'
out.mkdir(parents=True, exist_ok=False)
(out / 'maps').mkdir()
(out / 'screenshots').mkdir()
capture_dir = args.capture_dir.resolve() if args.capture_dir else root/'build/plasma-visual'
manifest = capture_dir/'build-manifest.json'
if manifest.exists():
    shutil.copy2(manifest, out/'build-manifest.json')
shutil.copy2(args.data_root/'maps/ui.map',out/'maps/ui.map')
if args.map=='ce-deathisland':
    (out/'custom_maps').mkdir()
    # Accept the original CE filename as well as the earlier fixture's name.
    ce_map = 'deathisland_451254bb' if (args.data_root/'custom_maps/deathisland_451254bb.map').is_file() else 'deathisland'
    for name in [ce_map+'.map','bitmaps.map','sounds.map','loc.map']:
        shutil.copy2(args.data_root/'custom_maps'/name,out/'custom_maps'/name)
    level='custom_maps\\'+ce_map
elif args.map=='mcc-digsite':
    for name in ['1.map','bitmaps.map','sounds.map']:
        target=out/'mcc_maps'/'DigsiteMP'/name
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(args.data_root/'mcc_maps'/'DigsiteMP'/name,target)
    level=r'mcc_maps\DigsiteMP\1'
elif args.map=='mcc-ruby-c40':
    for name in ['RubyRebalanced/c40.map', '_shared/singleplayer/bitmaps.map']:
        target=out/'mcc_maps'/name
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(args.data_root/'mcc_maps'/name,target)
    level=r'mcc_maps\RubyRebalanced\c40'
else:
    shutil.copy2(args.data_root/'maps'/(args.map+'.map'),out/'maps'/(args.map+'.map'))
    level=f'levels\\{args.map}\\{args.map}' if args.map in ('a10','b40','c40') else f'levels\\test\\{args.map}\\{args.map}'
(out/'init.txt').write_text(f'map_name {level}\n')
if args.specular_only:
    with (out/'init.txt').open('a') as init:
        init.write('(set rasterizer_environment_diffuse_lights false)\n')

env = {k:v for k,v in os.environ.items() if not k.startswith('HALO_')}
env['PATH'] = str(root / 'build/windows') + os.pathsep + env.get('PATH', '')
env.update(HALO_DATA_ROOT=str(out), HALO_SAVE_ROOT=str(out/'saves'), HALO_HIDDEN_WINDOW='1',
           HALO_NO_AUDIO='1', HALO_MAX_FPS=str(args.fps), HALO_NO_VSYNC='1', HALO_EXIT_AFTER='25' if args.door else '13',
           HALO_NET_ONLINE='0', HALO_NET_PUBLIC_LOBBY='0', HALO_UPDATE_ANSWER='never',
           HALO_NET_JOIN_FROM_CLIPBOARD='0', HALO_NET_ALLOW_UPNP='0',
           HALO_CRASH_REPORTS='no', HALO_WINDOW_SIZE=args.resolution, HALO_PLASMA_LIGHTING=str(args.enabled),
           HALO_SCREENSHOT_DIR=str(out/'screenshots'), HALO_PLASMA_OCCLUSION=str(args.occlusion))
if args.map == 'mcc-digsite':
    # This multiplayer package's scenario script is not a player-spawn path.
    # Start an ordinary isolated local match, as the compatibility runner does,
    # rather than capturing its empty campaign-style map_name session.
    (out/'init.txt').write_text('')
    env.update(HALO_NETWORK_TEST='local:'+level+':slayer', HALO_NETWORK_TEST_START='3',
               HALO_NET_ADDRESS='127.0.0.86', HALO_NET_BROADCAST='127.0.0.86',
               HALO_EXIT_AFTER='35')
if args.map in ('b40', 'c40', 'mcc-ruby-c40'):
    env['HALO_COVENANT_CAPTURE_CAMPAIGN'] = '1'
    env['HALO_EXIT_AFTER'] = '70' if args.map in ('b40', 'mcc-ruby-c40') else '35'
if args.prong_lighting:
    env.update(HALO_PLASMA_PRONG_LIGHTING=str(args.enabled), HALO_PLASMA_LIGHTING='0')
if args.weapon_glow:
    env['HALO_WEAPON_GLOW'] = str(args.enabled)
if args.turn:
    env['HALO_LIGHTING_CAPTURE_TURN'] = '1'
if args.hide_viewmodel:
    env['HALO_VIEWMODEL_VISIBLE'] = '0'
if args.face_camera:
    env['HALO_EMITTER_CAPTURE_FACE_CAMERA'] = '1'
if args.grounded:
    env['HALO_EMITTER_CAPTURE_GROUNDED'] = '1'
if args.crouch:
    env['HALO_EMITTER_CAPTURE_CROUCH'] = '1'
if args.exterior and args.muzzle == 'warthog':
    env['HALO_WARTHOG_CAPTURE_EXTERIOR'] = '1'
if args.sparse_images:
    env['HALO_LIGHTING_CAPTURE_SPARSE'] = '1'
    env['HALO_LIGHTING_CAPTURE_IMAGE_STEP'] = str(args.image_step)
if args.counter_detail:
    env['HALO_EMITTER_CAPTURE_COUNTER_DETAIL'] = '1'
if args.emitter:
    env['HALO_EXIT_AFTER'] = '70' if args.map in ('b40', 'mcc-ruby-c40') else '35'
    env['HALO_EMITTER_CAPTURE'] = args.emitter
    env['HALO_PLASMA_LIGHTING'] = '0'
    env[{'jackal':'HALO_JACKAL_SHIELD_LIGHTING', 'major':'HALO_JACKAL_SHIELD_LIGHTING',
         'fuel':'HALO_FUEL_ROD_LIGHTING', 'hunter':'HALO_FUEL_ROD_LIGHTING', 'stationary':'HALO_COVENANT_SHIELD_LIGHTING',
         'ammo-fp':'HALO_AMMO_COUNTER_LIGHTING','ammo-marine':'HALO_AMMO_COUNTER_LIGHTING',
         'ammo-player':'HALO_AMMO_COUNTER_LIGHTING', 'needler-fp':'HALO_NEEDLER_LIGHTING',
         'needler-player':'HALO_NEEDLER_LIGHTING','needler-marine':'HALO_NEEDLER_LIGHTING',
         'needler-grunt':'HALO_NEEDLER_LIGHTING', 'sword-elite':'HALO_WEAPON_GLOW'}[args.emitter]] = str(args.enabled)
if args.ambient:
    env['HALO_AMBIENT_CAPTURE']=args.ambient
    env['HALO_PLASMA_LIGHTING']='0'
    env['HALO_VEHICLE_LIGHTING' if args.ambient in ('ghost','banshee','dropship','warthog','scorpion') else 'HALO_WORLD_LIGHTING']=str(args.enabled)
if args.shield:
    env.update(HALO_SHIELD_CAPTURE=args.shield, HALO_SHIELD_GLOW=str(args.enabled),
               HALO_OVERSHIELD_EFFECT=str(int(args.shield in ('charge','fp'))), HALO_VIEWMODEL_SHIELD='1',
               HALO_SHIELD_LOOK=args.shield_look, HALO_SHIELD_GLOW_INTENSITY=args.glow_intensity, HALO_PLASMA_LIGHTING='0')
if args.muzzle:
    muzzle_tags = {
        'ar': r'weapons\assault rifle\assault rifle', 'handgun': r'weapons\pistol\pistol',
        'shotgun': r'weapons\shotgun\shotgun', 'sniper': r'weapons\sniper rifle\sniper rifle',
        'rifle': r'weapons\plasma rifle\plasma rifle', 'pistol': r'weapons\plasma pistol\plasma pistol',
        'charged': r'weapons\plasma pistol\plasma pistol',
        'needler': r'weapons\needler\mp_needler' if args.map == 'ce-deathisland' else r'weapons\needler\needler',
        'rocket': r'weapons\rocket launcher\rocket launcher', 'flamethrower': r'weapons\flamethrower\flamethrower',
        'warthog': r'vehicles\warthog\warthog gun',
        'scorpion': r'vehicles\scorpion\scorpion cannon',
    }
    env['HALO_MUZZLE_CAPTURE_WEAPON'] = muzzle_tags[args.muzzle]
    env['HALO_MUZZLE_LIGHTING'] = str(args.enabled)
    env['HALO_PLASMA_LIGHTING'] = str(int(args.both_lighting))
    if args.muzzle in ('warthog', 'scorpion'):
        env['HALO_' + args.muzzle.upper() + '_CAPTURE'] = '1'
        # The vehicle fixture equips its own weapon. Use the combat fixture's
        # sustained bursts rather than the handheld muzzle fixture's tap input.
        env.pop('HALO_MUZZLE_CAPTURE_WEAPON')
    if args.muzzle == 'charged':
        env['HALO_MUZZLE_CAPTURE_CHARGED'] = '1'
        args.kind = 'charged'
for key in ['distance', 'height', 'receiver_distance', 'span', 'burst', 'drop']:
    value = getattr(args, key)
    if value is not None:
        env['HALO_PLASMA_CAPTURE_'+key.upper()] = str(value)
env['HALO_PLASMA_CAPTURE_KIND'] = args.kind
for key in ['combat','door','toggle','mixed']:
    if getattr(args,key):env['HALO_PLASMA_CAPTURE_'+key.upper()]='1'
if args.probe:
    env['HALO_PLASMA_CAPTURE_PROBE']='1'
if args.no_images:
    env['HALO_PLASMA_CAPTURE_NO_IMAGES']='1'
if args.model:
    env['HALO_PLASMA_CAPTURE_MODEL'] = '1'
if args.per_pixel:
    env['HALO_PER_PIXEL_LIGHTING'] = '1'
with (out/'stdout.log').open('wb') as so, (out/'stderr.log').open('wb') as se:
    capture_executable = capture_dir/'halo-capture.exe'
    process = subprocess.Popen([str(capture_executable)], cwd=out,
                               env=env, stdout=so, stderr=se, creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        process.wait(timeout=145 if args.map in ('b40', 'c40', 'mcc-ruby-c40') else 45)
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=10)
logs = '\n'.join(p.read_text(errors='replace') for p in out.glob('*.log'))
times = [int(x) for x in re.findall(r'PLASMA frame (\d+)', logs)]
deltas = [b-a for a,b in zip(times, times[1:]) if b>a]
report = dict(kind=args.kind, mixed=args.mixed, cap=args.fps, enabled=args.enabled, exit=process.returncode,
              screenshots=len(list((out/'screenshots').glob('*.bmp'))), frames=len(times),
              selected_frames=len(re.findall(r'selected [1-9]', logs)),
              average_fps=1000*(len(times)-1)/(times[-1]-times[0]) if len(times)>1 else None,
              median_ms=statistics.median(deltas) if deltas else None,
              shots=len(re.findall('PLASMA shot', logs)),
              model_draws=sum(int(x) for x in re.findall(r'PLASMA draws model (\d+)', logs)),
              specular_draws=sum(int(x) for x in re.findall(r'PLASMA draws model \d+ specular (\d+)', logs)),
              max_selected=max([int(x) for x in re.findall(r'selected (\d+)',logs)] or [0]), burst=args.burst, combat=args.combat, door=args.door, toggle=args.toggle, occlusion=args.occlusion, probe=args.probe, occlusion_updates=len(re.findall('PLASMA occlusion extent',logs)), map=args.map, model=args.model, per_pixel=args.per_pixel, specular_only=args.specular_only)
report['authored_light_flags'] = sorted({int(x, 16) for x in re.findall(r'PLASMA enabled [01] light flags ([0-9a-f]+)', logs)})
report['glow_template_flags'] = sorted({int(x, 16) for x in re.findall(r'PLASMA glow template [^\r\n]+ flags ([0-9a-f]+)', logs)})
report['no_images'] = args.no_images
active_frames = [(int(t), int(n)) for t, n in re.findall(r'PLASMA frame (\d+)[^\r\n]+ selected (\d+)', logs)]
active_deltas = [b[0]-a[0] for a, b in zip(active_frames, active_frames[1:]) if a[1] and b[1] and b[0] > a[0]]
report['active_average_fps'] = 1000 / statistics.mean(active_deltas) if active_deltas else None
report['active_p95_ms'] = sorted(active_deltas)[int((len(active_deltas)-1)*.95)] if active_deltas else None
report['homing_shots'] = len(re.findall('PLASMA homing', logs))
report['selected_tags'] = sorted(set(re.findall(r'PLASMA prepare [^\r\n]+ tag ([^\r\n]+)', logs)))
report['muzzle'] = args.muzzle
report['shield'] = args.shield
report['shield_look'] = args.shield_look
report['glow_intensity'] = args.glow_intensity
report['shield_created'] = bool(re.search(r'SHIELD created (?!-1 )', logs))
report['ambient'] = args.ambient
report['ambient_created'] = bool(re.search(r'AMBIENT created (?!-1 )', logs))
report['emitter'] = args.emitter
report['hide_viewmodel'] = args.hide_viewmodel
report['face_camera'] = args.face_camera
report['grounded'] = args.grounded
report['crouch'] = args.crouch
report['exterior'] = args.exterior
report['holder_crouch_samples'] = [float(x) for x in re.findall(r'EMITTER holder tick \d+ crouch ([0-9.]+)', logs)]
report['turn'] = args.turn
report['counter_detail'] = args.counter_detail
report['resolution'] = args.resolution
report['emitter_samples'] = len(re.findall('EMITTER sample', logs))
report['emitter_created'] = bool(re.search(r'EMITTER created (?!-1 )', logs))
report['both_lighting'] = args.both_lighting
report['muzzle_submissions'] = len(re.findall('MUZZLE submit', logs))
report['muzzle_fade_samples'] = sum(0 < float(x) < .99 for x in re.findall(r'MUZZLE sample[^\r\n]+ strength ([0-9.]+)', logs))
(out/'report.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report), flush=True)
for path in out.rglob('*.map'):
    assert path.resolve().is_relative_to(out.resolve())
    path.unlink()
if process.returncode or (report['shots'] != 3 and not args.combat and not args.emitter and not args.shield and not args.ambient) or (not args.no_images and not report['screenshots']):
    raise SystemExit('Capture incomplete; inspect report.json and logs')
if args.ambient and not report['ambient_created']:
    raise SystemExit('Ambient fixture missing; this is not a passing capture')
if args.shield and not report['shield_created']:
    raise SystemExit('Shield fixture did not create its actor')
if args.emitter and (not report['emitter_created'] or (args.emitter != 'sword-elite' and bool(report['emitter_samples']) != bool(args.enabled))):
    raise SystemExit('Emitter creation or activation failed')
if args.emitter == 'sword-elite' and args.enabled and r'weapons\energy sword\energy sword' not in report['selected_tags']:
    raise SystemExit('Sword attachment never entered enhanced lighting')
if args.crouch and not args.shield and report['holder_crouch_samples'] != [1.0, 1.0, 0.0]:
    raise SystemExit('Holder did not crouch and return to standing')
if bool(report['selected_frames']) != bool(args.enabled or (args.muzzle and args.both_lighting)):
    raise SystemExit('Lighting selection did not match the requested mode')
if report['average_fps'] is None or report['average_fps'] < args.fps * .95:
    raise SystemExit('Capture did not sustain the requested FPS within 5 percent')

if args.enabled and args.model and not report['model_draws']:
    raise SystemExit('Capture never reached a selected model draw')
if args.enabled and args.specular_only:
    flags = report['authored_light_flags'] or report['glow_template_flags']
    # Bit 1 is _light_definition_no_specular_bit in object_lights.c.
    if flags and all(value & 2 for value in flags):
        if report['specular_draws']:
            raise SystemExit('Capture ignored the authored no-specular flag')
    elif not report['specular_draws']:
        raise SystemExit('Capture never reached a selected specular draw')

if args.enabled and args.occlusion and not report['occlusion_updates']:
    raise SystemExit('Capture did not update any occlusion fields')

if args.combat and 'PLASMA combat equipped' not in logs:
    raise SystemExit('Combat fixture did not equip the requested weapon')
if args.probe and args.enabled and report['max_selected'] < (min(args.burst, 8) if args.kind in ('needle', 'mp-needle', 'wraith') else args.burst):
    raise SystemExit('Stress capture did not select the requested simultaneous light count')
if args.door:
    positions = [float(x) for x in re.findall(r'PLASMA door position ([0-9.]+)', logs)]
    if not positions or min(positions) > .1 or max(positions) < .9:
        raise SystemExit('Door did not complete both open and closed states')
if args.toggle:
    intervals = re.split(r'PLASMA toggle ([01])', logs)
    if len(intervals) < 5:
        raise SystemExit('Toggle capture did not exercise both states')
    for i in range(1, len(intervals)-1, 2):
        if intervals[i] == '0' and 'PLASMA occlusion extent' in intervals[i+1]:
            raise SystemExit('Occlusion updated while disabled')

expected_tags = {
    'grenade': r'weapons\plasma grenade\plasma grenade',
    'rifle': r'weapons\plasma rifle\bolt',
    'pistol': r'weapons\plasma pistol\bolt',
    'charged': r'weapons\plasma rifle\charged bolt',
    'rocket': r'weapons\rocket launcher\rocket',
    'needle': 'weapons\\needler\\needle',
    'ghost': 'vehicles\\ghost\\ghost bolt',
    'banshee': 'vehicles\\banshee\\banshee bolt',
    'banshee-fuel': 'vehicles\\banshee\\banshee fuel rod',
    'shade': 'vehicles\\c gun turret\\c gun turret',
    'fuel': 'weapons\\fuel rod gun\\fuel rod',
    'wraith': 'vehicles\\wraith\\wraith mortar',
    'mp-needle': 'weapons\\needler\\mp_needle',
    'mp-banshee-fuel': 'vehicles\\banshee\\mp_banshee fuel rod',
    'mp-shade': 'vehicles\\c gun turret\\mp gun turret',
    'ce-fuel': 'weapons\\plasma_cannon\\plasma_cannon',
}
if args.enabled and not args.prong_lighting and not args.muzzle and not args.emitter and not args.shield and not args.ambient:
    expected = {expected_tags[args.kind]}
    if args.mixed:
        expected = {expected_tags[kind] for kind in ('rifle', 'pistol', 'charged', 'rocket') if args.kind == 'rocket' or kind != 'rocket'}
    if not expected.issubset(report['selected_tags']):
        raise SystemExit('Capture did not select every requested projectile kind')
if not args.enabled and not (args.muzzle and args.both_lighting and args.occlusion) and report['occlusion_updates']:
    raise SystemExit('Disabled lighting still updated occlusion')
if args.kind == 'charged' and args.model and not args.probe and not args.combat and report['homing_shots'] != 3:
    raise SystemExit('Charged-shot capture did not exercise the homing target')

if args.muzzle and args.enabled and (not report['muzzle_submissions'] or not report['muzzle_fade_samples']):
    raise SystemExit('Muzzle capture did not exercise submission and smooth fade')
if args.muzzle and not args.enabled and report['muzzle_submissions']:
    raise SystemExit('Disabled muzzle feature still submitted lights')

if args.muzzle == "warthog" and args.enabled and r"MUZZLE profile vehicles\warthog\warthog gun" not in logs:
    raise SystemExit("Capture did not fire the Warthog turret")
if args.muzzle == 'scorpion' and args.enabled:
    if r'MUZZLE profile vehicles\scorpion\scorpion cannon' not in logs or set(re.findall(r'MUZZLE sample[^\r\n]+ trigger ([01])', logs)) != {'0', '1'}:
        raise SystemExit('Capture did not fire both Scorpion guns')

if args.prong_lighting and args.enabled:
    expected = 'weapons\\plasma '+args.kind+'\\plasma '+args.kind
    if expected not in report['selected_tags']:
        raise SystemExit('Prong capture never selected the intended weapon attachment')
