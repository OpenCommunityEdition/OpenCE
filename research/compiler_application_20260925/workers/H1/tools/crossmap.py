"""H1: map a January object's file statics to their addresses in the first-party /Od build (as data).
January function F -> /Od function G by shared string literals (G must contain every literal of F that is
distinctive, i.e. referenced by <= 3 /Od functions). Then each January data owner S referenced (directly, with a
known width) by mapped functions gets the /Od addresses referenced by ALL its G's with the same width and by NO
mapped G whose F does not reference S.
    python crossmap.py <split obj> [owner ...]
"""
import sys, collections
import odindex, janfn


def run(path, only=None, verbose=True, idx=None):
    idx = idx if idx is not None else odindex.load()
    str_users = collections.defaultdict(set)
    for a, f in idx.items():
        for s in f['strs']:
            str_users[s].add(a)
    o, jf, dsyms, secname = janfn.facts(path)
    fmap = {}
    for name, f in jf.items():
        strs = [s for s in f['strs'] if 0 < len(str_users.get(s, ())) <= 3 and len(s) >= 4]
        if not strs:
            continue
        cands = set.intersection(*[str_users[s] for s in strs])
        if len(cands) == 1:
            fmap[name] = cands.pop()
    # owners touched
    touched = collections.defaultdict(set)   # owner -> {(F, width)}
    for name, f in jf.items():
        for sn, off, own, w, mn in f['data']:
            if own and own[1] == 0:
                touched[own[0]].add((name, w))
    result = {}
    for owner, uses in touched.items():
        if only and owner not in only:
            continue
        cand = None
        for fname, w in uses:
            if fname not in fmap:
                continue
            g = idx[fmap[fname]]
            addrs = {v for v, ww, mn in g['data'] if (w == 0 and ww in (0,)) or (w != 0 and ww == w)}
            if w == 0:
                addrs = {v for v, ww, mn in g['data'] if ww == 0 or ww < 0}
            cand = addrs if cand is None else cand & addrs
        if cand is None:
            continue
        # exclude addresses used by mapped functions that do NOT touch this owner
        users = {f for f, w in uses}
        for fname, gaddr in fmap.items():
            if fname in users:
                continue
            g = idx[gaddr]
            cand -= {v for v, ww, mn in g['data']}
        result[owner] = sorted(cand)
    if verbose:
        print('mapped functions: %d of %d' % (len(fmap), len(jf)))
        for owner in sorted(result, key=lambda n: min((v for v, nn in sum(dsyms.values(), []) if nn == n), default=0)):
            jan = [(secname[s], v) for s, lst in dsyms.items() for v, n in lst if n == owner]
            print('  %-50s jan %-18s od %s' % (owner, jan, [hex(a) for a in result[owner]]))
    return fmap, result, dsyms, secname


if __name__ == '__main__':
    import exeindex
    args = sys.argv[1:]
    ix = None
    if args and args[0].startswith('--index='):
        ix = exeindex.load(args[0][8:]); args = args[1:]
    run(args[0], set(args[1:]) or None, idx=ix)
