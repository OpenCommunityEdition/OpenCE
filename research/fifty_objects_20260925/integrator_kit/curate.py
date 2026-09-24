"""Copy curated worker evidence from ignored scratch/ into tracked research/fifty_objects_20260925/.

    python scratch/campaign/curate.py

Copies, per scratch/w/<slug>/: every *.md, *.patch and *.diff (outside split/ and config/ copies),
plus a diff against the tree for every .c candidate that wave results name in candidate_files; and every wave result/review record under scratch/campaign/wave*/units/.
Skips any single file over 400 KB (listed in SKIPPED.txt). .obj, split and symbols.json copies are
never copied. Idempotent: re-running overwrites with the current scratch contents.
"""
import glob
import json
import os
import shutil
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)
DEST = os.path.join('research', 'fifty_objects_20260925')
LIMIT = 400 * 1024
skipped = []
copied = 0


def want_dir(rel):
    parts = rel.replace('\\', '/').split('/')
    return not any(p in ('split', 'config', '__pycache__') or p.startswith('split_') for p in parts)


def copy(src, dst):
    global copied
    if os.path.getsize(src) > LIMIT:
        skipped.append('%s (%d bytes)' % (src, os.path.getsize(src)))
        return
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(src, dst)
    copied += 1


for slug in sorted(os.listdir(os.path.join('scratch', 'w'))):
    base = os.path.join('scratch', 'w', slug)
    if not os.path.isdir(base):
        continue
    for dirpath, dirnames, filenames in os.walk(base):
        rel = os.path.relpath(dirpath, base)
        if not want_dir(rel):
            dirnames[:] = []
            continue
        for f in filenames:
            ext = os.path.splitext(f)[1].lower()
            if ext in ('.md', '.patch', '.diff') or f.startswith('APPLY_ORDER') or f == 'SHA256SUMS.txt':
                copy(os.path.join(dirpath, f), os.path.join(DEST, 'w', slug, rel, f))

# named candidate sources are stored as diffs against the tree's version of the unit's .c
named = {}
for res in glob.glob(os.path.join('scratch', 'campaign', 'wave*', 'result.json')):
    data = json.load(open(res, encoding='utf-8'))
    for r in data.get('results', []):
        for u in (r or {}).get('units', []) or []:
            src = (u.get('unit') or '').split(' ')[0]
            src = src if src.endswith('.c') else src + '.c'
            for c in u.get('candidate_files', []) or []:
                if c.lower().endswith('.c'):
                    named[os.path.normpath(c)] = src
for c, src in sorted(named.items()):
    rel = os.path.relpath(c, os.path.join(ROOT, 'scratch', 'w'))
    if rel.startswith('..') or not os.path.exists(c):
        continue
    dst = os.path.join(DEST, 'w', rel[:-2] + '.vs_tree.diff')
    if os.path.exists(src):
        d = subprocess.run(['git', 'diff', '--no-index', '--ignore-cr-at-eol', src, c], capture_output=True, text=True,
                           encoding='utf-8', errors='replace').stdout
        if d and len(d) > 60000:
            skipped.append('%s (diff vs %s is %d bytes; wrong mapping or whole-file rewrite)' % (c, src, len(d)))
        elif d:
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            open(dst, 'w', encoding='utf-8', newline='\n').write(d)
            copied += 1
    else:
        copy(c, os.path.join(DEST, 'w', rel))

for rec in glob.glob(os.path.join('scratch', 'campaign', 'wave*', 'units', '*.md')):
    wave = rec.split(os.sep)[-3]
    copy(rec, os.path.join(DEST, 'results', wave, os.path.basename(rec)))

# the integrator kit that applies these packets
for f in ('batch_gate.py', 'admit.py', 'sym_edit.py', 'sym_ops_from_copy.py', 'rebaseline_parks.py', 'audit_battery.py', 'curate.py', 'WORKER_BRIEF.md'):
    copy(os.path.join('scratch', 'campaign', f), os.path.join(DEST, 'integrator_kit', f))
copy(os.path.join('scratch', 'patch_ninja.py'), os.path.join(DEST, 'integrator_kit', 'patch_ninja.py'))
with open(os.path.join(DEST, '.gitattributes'), 'w', newline='\n') as fh:
    fh.write('# evidence copies are kept byte-for-byte (patches must still apply) and are not style-checked\n'
             '* -text -whitespace\n')
with open(os.path.join(DEST, 'SKIPPED.txt'), 'w', encoding='utf-8', newline='\n') as fh:
    fh.write('Files over %d bytes left in scratch (not copied):\n' % LIMIT)
    for s in skipped:
        fh.write(s + '\n')
print('copied', copied, 'skipped', len(skipped))
