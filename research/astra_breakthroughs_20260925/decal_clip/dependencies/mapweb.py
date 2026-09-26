"""Map each coloured web to the instructions it occupies, by perturbation.

For decision k (reg R), force it to an allowed alternative R' and diff the function's
disassembly against the unforced build. Instructions that change only by R -> R'
(same position, same opcode) are the web's occurrences. Prints, per web, the
instruction offsets+text of the unforced build that mention R and flip.

  mapweb.py <src> <unit> <fn> [--k 0,1,..]
"""
import os, sys, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import trace as T, oracle as O, forcesearch as FS
import capstone
cc = O.cc

SUB = {'eax': ['eax', 'ax', 'al', 'ah'], 'ecx': ['ecx', 'cx', 'cl', 'ch'], 'edx': ['edx', 'dx', 'dl', 'dh'],
       'ebx': ['ebx', 'bx', 'bl', 'bh'], 'esi': ['esi', 'si'], 'edi': ['edi', 'di']}


def dis(objpath, fn):
    o = cc.load(open(objpath, 'rb').read())
    sym = cc.symbol(o, fn)
    sec = o['sections'][sym['section'] - 1]
    raw = bytes(cc._section_bytes(o, sec))
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    return [(i.address, '%s %s' % (i.mnemonic, i.op_str)) for i in md.disasm(raw, 0)]


def mentions(txt, reg):
    return any(re.search(r'\b%s\b' % s, txt) for s in SUB.get(reg, [reg]))


def map_webs(src, unit, fn, ks=None, work=None):
    work = os.path.abspath(work or os.path.join(T.HERE, 'work_map_' + fn))
    dec, obj, base, order = O.run_fn(src, unit, fn, work=work)
    import shutil
    base_obj = obj + '.base'
    shutil.copy(obj, base_obj)
    d0 = dis(base_obj, fn)
    res = {}
    for k, d in enumerate(dec):
        if ks is not None and k not in ks: continue
        alts = [r for r in (d['allowed'] or []) if r != d['reg']]
        # prefer an alternative that no web in this function uses (cleanest diff)
        used = {x['reg'] for x in dec}
        alts.sort(key=lambda r: (r in used, O.CLASS.index(r)))
        if not alts:
            res[k] = (d, None, [])
            continue
        r2 = alts[0]
        seg2, obj2 = T.run_gated(src, work, unit, fn, force=FS.force_list(base, [(k, r2)]))
        d1 = dis(obj2, fn)
        occ = []
        import difflib
        b0 = [FS.blind(t) for a, t in d0]; b1 = [FS.blind(t) for a, t in d1]
        sm = difflib.SequenceMatcher(None, b0, b1, autojunk=False)
        for op, i1, i2, j1, j2 in sm.get_opcodes():
            if op == 'equal':
                for x, y in zip(range(i1, i2), range(j1, j2)):
                    t0 = d0[x][1]; t1 = d1[y][1]
                    if t0 != t1 and mentions(t0, d['reg']) and mentions(t1, r2):
                        occ.append((d0[x][0], t0))
            else:
                occ.append(('STRUCT', '%s base[%x..] %s | forced %s' % (op, d0[i1][0] if i1 < len(d0) else -1,
                            '; '.join(t for a, t in d0[i1:i2])[:120], '; '.join(t for a, t in d1[j1:j2])[:120])))
        res[k] = (d, r2, occ)
    return dec, res, d0


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('src'); ap.add_argument('unit'); ap.add_argument('fn'); ap.add_argument('--k')
    a = ap.parse_args()
    ks = set(int(x) for x in a.k.split(',')) if a.k else None
    dec, res, d0 = map_webs(a.src, a.unit, a.fn, ks)
    for k in sorted(res):
        d, r2, occ = res[k]
        print('#%d id=%d %s pri=%d (probe ->%s) %d sites' % (k, d['id'], d['reg'], d['pri'], r2, len(occ)))
        for a0, t0 in occ:
            print('      %5s  %s' % (('%x' % a0) if isinstance(a0, int) else a0, t0))

