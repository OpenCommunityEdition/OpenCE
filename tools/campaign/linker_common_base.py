"""Build a comparison base for source/linker_common containing ONLY verified
COMMON records: symbols January pools into its COMMON block that OUR tree
independently emits as a COMMON symbol of the identical size.

Nothing is invented and nothing initialised is copied - a .bss record carries no
bytes, only a name and a size, and the size is the one our own objects request.
Sections whose symbol we do not emit, emit at a different size, or emit in a
real section rather than COMMON are DROPPED, so the result is an honest subset.
"""
import glob, sys
sys.path.insert(0, 'tools'); sys.path.insert(0, "tools/campaign")
import coff_compare as cc
import coffio

JAN = 'build/split/source/linker_common.obj'
OUT = sys.argv[1] if len(sys.argv) > 1 else 'scratch/probe/linker_common_base.obj'

# 1. our tree's COMMON symbols: name -> size
ours = {}
for p in glob.glob('build/base/**/*.obj', recursive=True):
    try: b = cc.load(open(p, 'rb').read())
    except Exception: continue
    for x in b['symbols']:
        if x.get('section') == 0 and x.get('value', 0) > 0:
            ours.setdefault(x['name'], x['value'])

o = coffio.load(JAN)
owner = {}
for sy in o.symbols:
    if sy['sec'] > 0 and not sy['name'].startswith('.'):
        owner.setdefault(sy['sec'], sy['name'])

keep, dropped = set(), []
for s in o.sections:
    i = s['index']
    n = owner.get(i)
    if s['name'] != '.bss' or n is None:
        dropped.append((n or s['name'], s['size'], 'not a named .bss record')); continue
    if n not in ours:
        dropped.append((n, s['size'], 'absent from our tree')); continue
    if ours[n] != s['size']:
        dropped.append((n, s['size'], 'our COMMON size %d' % ours[n])); continue
    keep.add(i)

remap, newsecs = {}, []
for s in o.sections:
    if s['index'] in keep:
        remap[s['index']] = len(newsecs) + 1
        t = dict(s); t['index'] = remap[s['index']]; newsecs.append(t)
newsyms = []
for sy in o.symbols:
    if sy['sec'] in remap:
        t = dict(sy); t['sec'] = remap[sy['sec']]; t['index'] = len(newsyms); newsyms.append(t)
o.sections, o.symbols = newsecs, newsyms
open(OUT, 'wb').write(coffio.build(o))

kept_bytes = sum(s['size'] for s in newsecs)
print("kept   %3d records  %9d bytes" % (len(newsecs), kept_bytes))
print("dropped %3d records %9d bytes" % (len(dropped), sum(d[1] for d in dropped)))
from collections import Counter
for reason, c in Counter(d[2].split(' ')[0] + ' ' + d[2].split(' ')[1] for d in dropped).most_common():
    print("   %-28s %d" % (reason, c))
print("wrote", OUT)
