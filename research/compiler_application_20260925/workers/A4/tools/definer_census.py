"""For a list of symbol names: every build/base definer, its COMDAT selection, and identity vs January's selected copy.
    python definer_census.py <names_file_or_comma_list> [--extra NAME=OBJ]"""
import glob, os, sys, struct, collections
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
os.chdir(WT)
sys.path.insert(0, 'tools')
import coff_compare as cc
arg = sys.argv[1]
names = [l.strip() for l in open(arg)] if os.path.exists(arg) else arg.split(',')
names = [n for n in names if n]
def sels(raw):
    symptr, nsym = struct.unpack_from('<II', raw, 8)
    out = {}
    i = 0
    while i < nsym:
        off = symptr + i * 18
        name, value, sec, typ, sclass, naux = struct.unpack_from('<8sIhHBB', raw, off)
        if sclass == 3 and naux == 1 and value == 0 and sec > 0:
            aux = raw[off + 18: off + 36]
            length, nrel, nline, chk, number, sel = struct.unpack_from('<IHHIHB', aux, 0)
            out.setdefault(sec, sel)
        i += 1 + naux
    return out
jan = {}
for p in glob.glob('build/split/**/*.obj', recursive=True):
    o = cc.load(open(p, 'rb').read())
    for s in o['symbols']:
        if s['name'] in names and s['section'] > 0 and s['storage'] == 2:
            jan[s['name']] = (p, o, s)
rows = collections.defaultdict(list)
for p in glob.glob('build/base/**/*.obj', recursive=True):
    raw = open(p, 'rb').read()
    try:
        o = cc.load(raw)
    except Exception:
        continue
    sel = None
    for s in o['symbols']:
        if s['name'] in names and s['section'] > 0 and s['storage'] == 2:
            if sel is None:
                sel = sels(raw)
            j = jan.get(s['name'])
            eq = None
            if j:
                eq = cc.section_infos_equal(cc.section_info_by_number(j[1], j[2]['section']), cc.section_info_by_number(o, s['section']))
            rows[s['name']].append((os.path.relpath(p, 'build/base')[:-4], sel.get(s['section']), eq))
for n in names:
    r = rows.get(n, [])
    c = collections.Counter((x[1], x[2]) for x in r)
    print('%-46s definers %3d  (selection, identical-to-January): %s' % (n[:46], len(r), dict(c)))
    for x in r:
        if x[1] != 2 or x[2] is not True:
            print('        NOTE', x)
