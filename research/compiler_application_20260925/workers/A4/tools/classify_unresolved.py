"""Classify a link_probe unresolved list by January split definer.  python classify_unresolved.py <unresolved.txt> <out.json>"""
import glob, os, sys, collections, json
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
os.chdir(WT)
sys.path.insert(0, 'tools')
import coff_compare as cc
BS = chr(92)
unres = [l.strip() for l in open(sys.argv[1]) if l.strip()]
definer = {}
for p in glob.glob('build/split/**/*.obj', recursive=True):
    try:
        o = cc.load(open(p, 'rb').read())
    except Exception:
        continue
    rel = os.path.relpath(p, 'build/split').replace(BS, '/')[:-4]
    for s in o['symbols']:
        if s['section'] > 0 and s['storage'] == 2:
            definer.setdefault(s['name'], rel)
cfg = json.load(open('config/config.json'))
proj_of, status = {}, {}
for pr in cfg['projects']:
    for ob in pr['objects']:
        k = os.path.splitext(ob['name'])[0]
        proj_of[k] = pr['name']
        status[k] = ob['status']
cats = collections.Counter()
ex = collections.defaultdict(list)
for n in unres:
    d = definer.get(n)
    if d is None:
        c = 'no January split definer (vendor lib / import / CRT)'
    elif d == 'source/linker_common':
        c = 'January pooled COMMON (linker_common): absent tentative definition'
    else:
        c = 'January object in project=%s status=%s' % (proj_of.get(d, '?'), status.get(d, '?'))
    cats[c] += 1
    ex[c].append(n + ('' if d is None else '  <- ' + d))
for c, k in cats.most_common():
    print('%4d  %s' % (k, c))
    for e in ex[c][:8]:
        print('        ', e)
json.dump({c: ex[c] for c in ex}, open(sys.argv[2], 'w'), indent=1)
