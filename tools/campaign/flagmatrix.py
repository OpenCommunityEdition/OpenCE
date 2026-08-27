"""Flag matrix for one unit, using the same cflag extraction as gate.py.

The original scratch/flagmatrix.py predates the build.ninja $-continuation fix and
cannot find units whose build line wraps.

usage: python scratch/flagmatrix2.py <unit> [focus-fn]
"""
import subprocess, sys, re, os
sys.path.insert(0, 'tools')
import coff_compare as cc

UNIT = sys.argv[1]
FOCUS = sys.argv[2] if len(sys.argv) > 2 else None

bn = re.sub(r'\s*\$\r?\n\s*', ' ', open('build.ninja').read())
key = 'build build' + chr(92) + 'base' + chr(92) + UNIT.replace('/', chr(92)) + '.obj:'
i = bn.index(key)
j = bn.index('cflags = ', i)
k = bn.index('build ', j)
cf = re.sub(r'\s+', ' ', bn[j + 9:k]).strip()
toks = [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in re.findall(r'/I"[^"]+"|\S+', cf)]
CL = r'C:\halo-worktrees\claude-finish-hs-20260816\xbox\bin\vc7\CL.Exe'

target = cc.load(open(f'build/split/{UNIT}.obj', 'rb').read())
secs = target['sections']
fns = []
seen = set()
for s in target['symbols']:
    n = s['name']
    if (not n.startswith('_') or s['section'] <= 0 or s['storage'] not in (2, 3)
            or s['value'] != 0 or secs[s['section'] - 1]['name'] != '.text' or s['section'] in seen):
        continue
    seen.add(s['section'])
    fns.append(n)

MATRIX = [[], ['/Ob1'], ['/Ob0'], ['/Ob2'], ['/Oi'], ['/Ot'], ['/Os'], ['/Op'], ['/QIfist'],
          ['/Gf'], ['/GF'], ['/Ow'], ['/Oa'], ['/Ox'], ['/O1'], ['/Ob1', '/Oi']]
base = [t for t in toks if not t.startswith('/Fo')]
for extra in MATRIX:
    o = f'scratch/_fm_{os.getpid()}.obj'
    if os.path.exists(o):
        os.remove(o)
    subprocess.run([CL, '/nologo', '/c'] + base + extra + ['/Fo' + o, UNIT + '.c'],
                   capture_output=True, text=True)
    if not os.path.exists(o):
        print(f'{" ".join(extra) or "(baseline)":18s} COMPILE FAIL')
        continue
    ours = cc.load(open(o, 'rb').read())
    ex = 0
    focus = ''
    for n in fns:
        try:
            ti = cc.section_info(target, n)
            bi = cc.section_info(ours, n)
            if ti and bi and cc.section_infos_equal(ti, bi):
                ex += 1
            if n == FOCUS and ti and bi:
                focus = f'   {FOCUS}: {bi["size"]} vs {ti["size"]}, relocs {bi["relocation_count"]}/{ti["relocation_count"]}'
        except Exception:
            pass
    print(f'{" ".join(extra) or "(baseline)":18s} {ex}/{len(fns)} exact{focus}')
