"""Every candidate-only EXTERNAL definition (any section) vs January's selected definer; plus our COMDAT selection.
    python surplus_all.py <unit> <ours.obj> [split.obj]"""
import glob, os, sys, struct
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
os.chdir(WT)
sys.path.insert(0, 'tools')
import coff_compare as cc
unit, ours = sys.argv[1], sys.argv[2]
split = sys.argv[3] if len(sys.argv) > 3 else 'build/split/%s.obj' % unit
def comdat_sel(raw, secnum):
    # parse COFF: find the section symbol aux record for secnum and return Selection byte
    nsec = struct.unpack_from('<H', raw, 2)[0]
    symptr, nsym = struct.unpack_from('<II', raw, 8)
    i = 0
    while i < nsym:
        off = symptr + i * 18
        name, value, sec, typ, sclass, naux = struct.unpack_from('<8sIhHBB', raw, off)
        if sec == secnum and sclass == 3 and naux == 1 and value == 0:
            aux = raw[off + 18: off + 36]
            length, nrel, nline, chk, number, sel = struct.unpack_from('<IHHIHB', aux, 0)
            return sel
        i += 1 + naux
    return None
defs = {}
for p in glob.glob('build/split/**/*.obj', recursive=True):
    try:
        o = cc.load(open(p, 'rb').read())
    except Exception:
        continue
    for s in o['symbols']:
        if s['section'] > 0 and s['storage'] == 2:
            defs.setdefault(s['name'], []).append((p, o, s))
t = cc.load(open(split, 'rb').read())
tdef = {s['name'] for s in t['symbols'] if s['section'] > 0}
raw = open(ours, 'rb').read()
o = cc.load(raw)
bad = n = 0
for s in o['symbols']:
    if s['section'] <= 0 or s['storage'] != 2 or s['name'] in tdef or s['name'].startswith('$'):
        continue
    n += 1
    sec = o['sections'][s['section'] - 1]
    sel = comdat_sel(raw, s['section'])
    provs = defs.get(s['name'], [])
    if not provs:
        print('  %-44s %-6s sel=%s NO January provider' % (s['name'][:44], sec['name'], sel)); bad += 1; continue
    for p, po, ps in provs:
        eq = cc.section_infos_equal(cc.section_info_by_number(po, ps['section']), cc.section_info_by_number(o, s['section']))
        psec = po['sections'][ps['section'] - 1]
        feq = (psec['flags'] & ~0x1000) == (sec['flags'] & ~0x1000)
        if not eq:
            bad += 1
        print('  %-44s %-6s sel=%s %-9s flags(ex-COMDAT) %s  vs %s' % (s['name'][:44], sec['name'], sel, 'IDENTICAL' if eq else 'DIFFERENT', 'eq' if feq else '%08x/%08x' % (psec['flags'], sec['flags']), os.path.relpath(p, 'build/split')[:-4]))
print('%s: %d candidate-only external definitions, %d not identical' % (unit, n, bad))
