"""Reproducible scout-only source-shape controls; generated copies stay in scratch.

No canonical input is modified. Historical count-shift leveling helper is excluded.
"""
from pathlib import Path
import hashlib
import difflib
from collections import Counter
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'scratch/astra_scout_20260925'
REPORT = Path(__file__).resolve().parent
DONOR = Path('C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review_vehicles/rv_od_leveling.c')
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc
from tools.campaign.alndiff import disassemble_function

FN = '_update_alien_scout_physics'
START = 'static void update_alien_scout_physics('
END = 'static void update_alien_fighter_physics('

def body(s):
    return s[s.index(START):s.index(END)]

def replace(s, a, b):
    assert s.count(a) == 1, (s.count(a), a)
    return s.replace(a, b)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def strip_aggregate_views(s):
    s = replace(s,
        'real_vector2d vehicle_forward_horizontal = *(real_vector2d const *)&vehicle->object.forward;',
        'real_vector2d vehicle_forward_horizontal;')
    anchor = '\t\t\tcross_product3d(&vehicle->object.up, &vehicle->object.forward, &vehicle_left);\n\t\t\tvehicle_left_horizontal ='
    s = replace(s, anchor,
        '\t\t\tvehicle_forward_horizontal.i = vehicle->object.forward.i;\n'
        '\t\t\tvehicle_forward_horizontal.j = vehicle->object.forward.j;\n' + anchor)
    return replace(s,
        'vehicle_left_horizontal = *(real_vector2d const *)&vehicle_left;',
        'vehicle_left_horizontal.i = vehicle_left.i;\n'
        '\t\t\tvehicle_left_horizontal.j = vehicle_left.j;')

def restore_input_copies(s):
    # The canonical source has exactly these real, initialized copies. This
    # control separates copy ownership from the remainder of the /Od topology.
    s = replace(s, '\t\t\t\treal_vector2d alpha_bonus;',
        '\t\t\t\treal_vector2d alpha_bonus;\n'
        '\t\t\t\treal_vector2d up_copy;\n'
        '\t\t\t\treal_vector2d angular_velocity_copy;\n\n'
        '\t\t\t\tup_copy.i = vehicle->object.up.i;\n'
        '\t\t\t\tup_copy.j = vehicle->object.up.j;\n'
        '\t\t\t\tangular_velocity_copy.i = vehicle->object.angular_velocity.i;\n'
        '\t\t\t\tangular_velocity_copy.j = vehicle->object.angular_velocity.j;')
    assert s.count('(real_vector2d const *)&vehicle->object.up') == 2
    assert s.count('(real_vector2d const *)&vehicle->object.angular_velocity') == 2
    return s.replace('(real_vector2d const *)&vehicle->object.up', '&up_copy').replace(
        '(real_vector2d const *)&vehicle->object.angular_velocity', '&angular_velocity_copy')

def direct_views_only(s):
    s = replace(s, '\t\t\t\treal_vector2d up;\n\t\t\t\treal_vector2d angular_velocity;\n', '')
    s = replace(s,
        '\t\t\t\tup.i = object_up->i;\n\t\t\t\tup.j = object_up->j;\n'
        '\t\t\t\tangular_velocity.i = object_angular_velocity->i;\n'
        '\t\t\t\tangular_velocity.j = object_angular_velocity->j;\n', '')
    assert s.count('dot_product2d(&up,') == 2
    assert s.count('dot_product2d(&angular_velocity,') == 2
    return s.replace('dot_product2d(&up,', 'dot_product2d((real_vector2d const *)object_up,').replace(
        'dot_product2d(&angular_velocity,', 'dot_product2d((real_vector2d const *)object_angular_velocity,')

def measure(name, source):
    cpath = OUT / (name + '.c')
    objpath = OUT / (name + '.obj')
    cpath.write_text(source, encoding='latin-1', newline='\n')
    original = (ROOT / 'source/units/vehicles.c').read_text(encoding='latin-1')
    (REPORT / (name + '.patch')).write_text(''.join(difflib.unified_diff(
        original.splitlines(keepends=True), source.splitlines(keepends=True),
        fromfile='a/source/units/vehicles.c', tofile='b/source/units/vehicles.c')), encoding='latin-1')
    command = [sys.executable, 'tools/campaign/gate.py', 'source/units/vehicles',
        '--source', str(cpath), '--all', '--out', str(objpath),
        '--cflag', '/FAsc', '--cflag', '/Fa' + str(OUT / (name + '.listing.asm'))]
    run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    (OUT / (name + '.gate.txt')).write_text(run.stdout + run.stderr)
    (REPORT / (name + '.gate.txt')).write_text(run.stdout + run.stderr)
    if run.returncode:
        raise RuntimeError(run.stdout + run.stderr)
    regions = subprocess.run([sys.executable, 'tools/campaign/real_regions.py',
        'source/units/vehicles', FN, '--ours-object', str(objpath), '--show'],
        cwd=ROOT, capture_output=True, text=True, check=True).stdout
    (OUT / (name + '.regions.txt')).write_text(regions)
    obj = cc.load(objpath)
    info = cc.section_info(obj, FN)
    instructions = disassemble_function(obj, FN)
    (OUT / (name + '.asm')).write_text('\n'.join(x.rendered for x in instructions))
    rows = [x for x in run.stdout.splitlines() if x.startswith(('EXACT', 'residual', 'unwritten'))]
    result = dict(name=name, source_sha256=sha(cpath), object_sha256=sha(objpath),
        rows=rows, section=info, instructions=len(instructions),
        prologue=[x.rendered for x in instructions[:7]], regions_summary=regions.splitlines()[:3])
    print(name, rows[[FN in x for x in rows].index(True)], result['prologue'], flush=True)
    return result

def code_owners(obj):
    return {s['name']: cc.section_info_by_number(obj, s['section']) for s in obj['symbols']
        if s['section'] > 0 and s['type'] == 0x20 and s['storage'] in (2, 3)
        and s['value'] == 0 and obj['sections'][s['section']-1]['name'] == '.text'}

def noncode(obj):
    rows = []
    for sec in obj['sections']:
        if sec['name'].startswith('.debug') or sec['flags'] & cc.IMAGE_SCN_CNT_CODE:
            continue
        info = cc.section_info_by_number(obj, sec['index'])
        rows.append(json.dumps(dict(name=sec['name'], size=info['size'],
            hash=info['normalized_sha256'], relocs=info['relocations']), sort_keys=True))
    return Counter(rows)

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    canonical = ROOT / 'source/units/vehicles.c'
    baseline = canonical.read_text(encoding='latin-1')
    before = sha(canonical)
    baseline_body = body(baseline)
    prior = body(DONOR.read_text(encoding='latin-1'))
    assert prior.count('point_from_line2d(') == 2
    assert 'applied_alpha.i += leveling_scale*alpha_desire.i*' in prior
    ab = strip_aggregate_views(prior)
    variants = {
        'baseline': baseline_body,
        'A_direct_views': direct_views_only(baseline_body),
        'B_genuine_topology_with_copies': restore_input_copies(ab),
        'AB_genuine_topology_direct_views': ab,
        'held_aggregate_control': prior,
    }
    # A second, orthogonal 2x2 closes the specific four-byte frame question.
    # These are policy-held diagnostics, not proposed source changes.
    forward_scalar = replace(prior,
        'real_vector2d vehicle_forward_horizontal = *(real_vector2d const *)&vehicle->object.forward;',
        'real_vector2d vehicle_forward_horizontal;')
    anchor = '\t\t\tcross_product3d(&vehicle->object.up, &vehicle->object.forward, &vehicle_left);\n\t\t\tvehicle_left_horizontal ='
    forward_scalar = replace(forward_scalar, anchor,
        '\t\t\tvehicle_forward_horizontal.i = vehicle->object.forward.i;\n'
        '\t\t\tvehicle_forward_horizontal.j = vehicle->object.forward.j;\n' + anchor)
    left_scalar = replace(prior,
        'vehicle_left_horizontal = *(real_vector2d const *)&vehicle_left;',
        'vehicle_left_horizontal.i = vehicle_left.i;\n'
        '\t\t\tvehicle_left_horizontal.j = vehicle_left.j;')
    variants.update(held_C_left_aggregate_only=forward_scalar,
        held_D_forward_aggregate_only=left_scalar)
    results = []
    for name, fn in variants.items():
        results.append(measure(name, replace(baseline, baseline_body, fn)))
    target = cc.load(ROOT / 'build/split/source/units/vehicles.obj')
    payload = dict(head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
        canonical_before=before, canonical_after=sha(canonical), donor_path=str(DONOR),
        donor_sha256=sha(DONOR), target=cc.section_info(target, FN), results=results)
    (OUT / 'results.json').write_text(json.dumps(payload, indent=2))
    baseline_obj = cc.load(OUT / 'baseline.obj')
    baseline_code = code_owners(baseline_obj)
    audit = []
    for result in results:
        name = result['name']
        obj = cc.load(OUT / (name + '.obj'))
        owners = code_owners(obj)
        changed = [n for n in baseline_code if n in owners and
            not cc.section_infos_equal(baseline_code[n], owners[n])]
        scan = subprocess.run([sys.executable, 'tools/fake_match_scan.py', str(OUT / (name + '.c'))],
            capture_output=True, text=True, cwd=ROOT)
        (OUT / (name + '.fake_scan.txt')).write_text(scan.stdout + scan.stderr)
        audit.append(dict(name=name, source_sha256=result['source_sha256'],
            object_sha256=result['object_sha256'], size=result['section']['size'],
            normalized_sha256=result['section']['normalized_sha256'],
            relocation_count=result['section']['relocation_count'],
            instructions=result['instructions'], frame=result['prologue'][2],
            regions=result['regions_summary'][1], changed_code_owners=changed,
            new_code_owners=sorted(set(owners)-set(baseline_code)),
            lost_code_owners=sorted(set(baseline_code)-set(owners)),
            exact_rows=[x for x in result['rows'] if x.startswith('EXACT')],
            noncode_unchanged=noncode(obj)==noncode(baseline_obj),
            fake_scan=scan.stdout.strip()))
    (REPORT / 'measurements.json').write_text(json.dumps(dict(head=payload['head'],
        canonical_sha256=before, canonical_after=sha(canonical), donor_sha256=payload['donor_sha256'],
        audit=audit), indent=2))
    assert sha(canonical) == before

if __name__ == '__main__':
    main()
