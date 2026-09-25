"""Compiler-in-the-loop counterfactuals for one function's colouring.

  forcesearch.py <src> <unit> <fn> [--pairs] [--only k,k,...] [--try k:reg,k:reg ...]

For each decision k and each ALLOWED alternative register r, force the chooser's
result to r inside the real C2.Dll (dbg32c FORCE), recompile, and measure the
function against January:
  EXACT (coff_compare.section_infos_equal), size, differing instructions
  (register-aware diff of the real code), and differing instructions register-blind.
A single forced decision that yields strict EXACT proves the whole residual is
that one allocator decision.
"""
import os, sys, difflib, itertools, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import trace as T, oracle as O
import capstone
cc = O.cc
WT = T.WT
REGRE = re.compile(r'\b(e?[abcd]x|[abcd][lh]|e?[sd]i|e?[sb]p)\b')


def fn_insns(objpath, fn):
    o = cc.load(open(objpath, 'rb').read())
    sym = cc.symbol(o, fn)
    sec = o['sections'][sym['section'] - 1]
    raw = bytearray(cc._section_bytes(o, sec))
    info = cc.section_info_by_number(o, sym['section'])
    for r in info['relocations']:
        raw[r['address']:r['address'] + 4] = b'\0\0\0\0'
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    out = []
    for i in md.disasm(bytes(raw), 0):
        out.append('%s %s' % (i.mnemonic, i.op_str))
    # strip trailing padding
    while out and (out[-1].startswith('nop') or out[-1].startswith('int3') or out[-1].startswith('lea ') and False):
        out.pop()
    return out, info, bytes(raw)


def blind(s):
    s = re.sub(r'0x[0-9a-f]+', 'N', s)
    return REGRE.sub('R', s)


def ndiff(a, b):
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    return sum(max(i2 - i1, j2 - j1) for op, i1, i2, j1, j2 in sm.get_opcodes() if op != 'equal')


_J = {}


def january(unit, fn):
    if (unit, fn) not in _J:
        _J[(unit, fn)] = fn_insns(os.path.join(WT, 'build', 'split', unit + '.obj'), fn)
    return _J[(unit, fn)]


def measure(objpath, unit, fn):
    ji, jinfo, jraw = january(unit, fn)
    oi, oinfo, oraw = fn_insns(objpath, fn)
    ex = cc.section_infos_equal(jinfo, oinfo)
    return dict(exact=ex, size=oinfo['size'], jsize=jinfo['size'],
                d=ndiff(ji, oi), dblind=ndiff([blind(x) for x in ji], [blind(x) for x in oi]),
                ninsn=len(oi), jninsn=len(ji))


def force_list(base, pairs):
    f = []
    for k, r in pairs:
        hit = base + k + 1
        v = O.IDX[r]
        f.append(('CHOOSE', hit, 0, 1, 0, v))
        f.append(('CHOOSE', hit, 1, 6, 0x10, v))
    return f


def run_forced(src, unit, fn, base, pairs, work):
    seg, obj = T.run_gated(src, work, unit, fn, force=force_list(base, pairs))
    return measure(obj, unit, fn), obj


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('src'); ap.add_argument('unit'); ap.add_argument('fn')
    ap.add_argument('--only'); ap.add_argument('--try', dest='tries', nargs='*', default=[])
    ap.add_argument('--pairs', action='store_true')
    a = ap.parse_args()
    work = os.path.join(T.HERE, 'work_force' + a.fn)
    dec, obj, base, order = O.run_fn(a.src, a.unit, a.fn, work=work)
    m0 = measure(obj, a.unit, a.fn)
    print('FLOOR', m0)
    if a.tries:
        pairs = [(int(x.split(':')[0]), x.split(':')[1]) for x in a.tries]
        m, _ = run_forced(a.src, a.unit, a.fn, base, pairs, work)
        print('TRY', pairs, m)
        sys.exit(0)
    only = set(int(x) for x in a.only.split(',')) if a.only else None
    singles = []
    for k, d in enumerate(dec):
        if only is not None and k not in only: continue
        for r in d['allowed'] or []:
            if r == d['reg']: continue
            m, _ = run_forced(a.src, a.unit, a.fn, base, [(k, r)], work)
            singles.append(((k, r), m))
            print('k=%-3d id=%-3d %s->%s  exact=%s size=%d d=%d blind=%d' % (k, d['id'], d['reg'], r, m['exact'], m['size'], m['d'], m['dblind']), flush=True)
    singles.sort(key=lambda t: (not t[1]['exact'], t[1]['size'] != t[1]['jsize'], t[1]['d']))
    print('BEST singles:')
    for p, m in singles[:10]:
        print('  ', p, m)
    if a.pairs:
        cand = [p for p, m in singles if m['d'] < m0['d']][:12]
        res = []
        for x, y in itertools.combinations(cand, 2):
            if x[0] == y[0]: continue
            m, _ = run_forced(a.src, a.unit, a.fn, base, sorted([x, y]), work)
            res.append(((x, y), m))
            print('pair', x, y, 'exact=%s size=%d d=%d blind=%d' % (m['exact'], m['size'], m['d'], m['dblind']), flush=True)
        res.sort(key=lambda t: (not t[1]['exact'], t[1]['d']))
        print('BEST pairs:')
        for p, m in res[:10]:
            print('  ', p, m)

