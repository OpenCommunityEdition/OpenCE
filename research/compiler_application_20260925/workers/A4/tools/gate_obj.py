"""Per-function strict gate of an already-compiled object against a (possibly alternate) split root.
Same logic as tools/campaign/gate.py (fn_syms + coff_compare.section_infos_equal).
    python gate_obj.py <unit> <ours.obj> [split_root=build/split] [--quiet]"""
import os, sys
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
os.chdir(WT)
sys.path.insert(0, 'tools')
import coff_compare as cc
args = [x for x in sys.argv[1:] if not x.startswith('--')]
quiet = '--quiet' in sys.argv
unit, ours_p = args[0], args[1]
root = args[2] if len(args) > 2 else os.path.join('build', 'split')
target = cc.load(open(os.path.join(root, unit + '.obj'), 'rb').read())
ours = cc.load(open(ours_p, 'rb').read())
def fn_syms(o):
    secs = o['sections']
    out = {}
    for s in o['symbols']:
        if (s['type'] == 0x20 and s['section'] > 0 and s['storage'] in (2, 3)
                and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text'):
            out.setdefault(s['name'], s)
    return out
tsyms, osyms = fn_syms(target), fn_syms(ours)
ne = nr = nu = 0
for name in sorted(tsyms):
    ti = cc.section_info(target, name)
    if name not in osyms:
        nu += 1
        print('UNWRITTEN %5d  %s' % (ti['size'], name))
        continue
    oi = cc.section_info(ours, name)
    if cc.section_infos_equal(ti, oi):
        ne += 1
        if not quiet:
            print('EXACT     %5d  %s' % (ti['size'], name))
    else:
        nr += 1
        why = []
        if ti['size'] != oi['size']: why.append('size %d!=%d' % (oi['size'], ti['size']))
        if ti['relocation_count'] != oi['relocation_count']: why.append('relocs')
        if ti['normalized_sha256'] != oi['normalized_sha256']: why.append('sha')
        if not why: why.append('reloc-identity')
        print('residual  %5d  %s  [%s]' % (ti['size'], name, ', '.join(why)))
print('== exact %d  residual %d  unwritten %d  (of %d)  split=%s' % (ne, nr, nu, len(tsyms), root))
