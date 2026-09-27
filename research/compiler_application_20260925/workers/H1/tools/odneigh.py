"""H1: list every /Od data address in [lo, hi) with its access widths and the referencing functions
(tagged with the __FILE__ string when the function carries one)."""
import odindex, sys, collections
BS = chr(92)
idx = odindex.load()
lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
by = collections.defaultdict(list)
for a, f in idx.items():
    for v, w, mn in f['data']:
        if lo <= v < hi:
            by[v].append((a, w))
for v in sorted(by):
    fs = sorted({a for a, w in by[v]})
    ws = sorted({w for a, w in by[v]})
    tag = []
    for a in fs[:3]:
        s = [x for x in idx[a]['strs'] if BS in x and x.endswith('.c')]
        tag.append('%x%s' % (a, ('(' + s[0] + ')') if s else ''))
    print('%x  w=%s  fns=%d %s' % (v, ws, len(fs), ' '.join(tag)))
