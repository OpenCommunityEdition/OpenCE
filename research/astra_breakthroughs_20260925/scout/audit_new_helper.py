"""Bounded identity/coalescing audit for the sole newly emitted scout helper."""
from pathlib import Path
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'scratch/astra_scout_20260925'
REPORT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

def main():
    helper = '_point_from_line2d'
    candidate_path = OUT / 'held_aggregate_control.obj'
    target_path = ROOT / 'build/split/source/ai/action_vehicle.obj'
    provider_path = ROOT / 'build/base/source/ai/action_vehicle.obj'
    candidate, target, provider = map(cc.load, [candidate_path, target_path, provider_path])
    c, t, p = [cc.section_info(x, helper) for x in [candidate, target, provider]]
    commands = []
    for index, objects in enumerate([[candidate_path, provider_path], [provider_path, candidate_path]]):
        command = [str(ROOT / 'xbox/bin/vc7/Link.Exe'), '/NOLOGO', '/MACHINE:X86',
            '/SUBSYSTEM:CONSOLE', '/NODEFAULTLIB', '/ENTRY:probe_entry',
            '/OUT:' + str(OUT / ('provider_' + str(index) + '.exe'))] + list(map(str, objects))
        run = subprocess.run(command, capture_output=True, text=True, cwd=ROOT)
        output = run.stdout + run.stderr
        (OUT / ('provider_' + str(index) + '.log')).write_text(output)
        commands.append(dict(order=list(map(str, objects)), returncode=run.returncode,
            duplicate_errors=[x for x in output.splitlines() if 'LNK2005' in x or 'LNK1169' in x]))
    incoming = []
    for sec in candidate['sections']:
        if not sec['flags'] & cc.IMAGE_SCN_CNT_CODE:
            continue
        info = cc.section_info_by_number(candidate, sec['index'])
        for relocation in info['relocations']:
            if helper in json.dumps(relocation):
                incoming.append(dict(section=sec['index'], relocation=relocation))
    report = dict(helper=helper, size=c['size'], normalized_sha256=c['normalized_sha256'],
        exact_to_january_selected_provider=cc.section_infos_equal(c,t),
        exact_to_current_selected_provider=cc.section_infos_equal(c,p),
        incoming_runtime_relocations=incoming, link_attempts=commands,
        meaning='No duplicate definition in either order; unresolved externals expected. Not a whole-program link or admission.')
    (REPORT / 'new_helper_audit.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
