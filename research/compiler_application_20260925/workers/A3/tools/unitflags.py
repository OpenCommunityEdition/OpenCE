"""A3: print non-/I flags and the union of /I include paths for given units (from build.ninja). Read-only.

    python -B research/compiler_application_20260925/workers/A3/tools/unitflags.py source/hs/hs_runtime ...
"""
import re
import sys

bn = open('build.ninja').read()
incs = set()
for u in sys.argv[1:]:
    key = 'build\\base\\' + u.replace('/', '\\').replace(' ', '$ ') + '.obj:'
    i = bn.index(key)
    j = bn.index('cflags = ', i)
    k = bn.index('\nbuild ', j)
    cf = bn[j + len('cflags = '):k].replace('$\n', ' ').replace('$\r\n', ' ')
    toks = re.findall(r'/I"[^"]+"|\S+', cf)
    incs.update(t for t in toks if t.startswith('/I'))
    print('%-45s %s' % (u, ' '.join(t for t in toks if not t.startswith('/I'))))
print('INCLUDES:', ' '.join(sorted(incs)))
