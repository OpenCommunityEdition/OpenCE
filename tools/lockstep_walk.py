# -*- coding: utf-8 -*-
"""Full-function lockstep walk of render_actor with slot wildcarding.

Collects: every extra/missing instruction (with context), every shape
mismatch, cumulative byte-drift checkpoints, and the slot bijection epochs.
"""
import sys, re
sys.path.insert(0, r'C:\Users\isabe\AppData\Local\Temp\claude')
import ad
import coff_compare as cc

SLOT = re.compile(r'ebp - (0x[0-9a-f]+|\d+)')


def norm(x):
    return x.mnemonic + ' ' + SLOT.sub('ebp-S', x.op_str)


def load():
    out, err = ad.gate.build(ad.UNIT)
    ours = cc.load(out)
    ja = ad.secbytes(ad.TGT, '_ai_debug_render_actor')
    ob = ad.secbytes(ours, '_ai_debug_render_actor')
    ji = [k for k, v in ad.JO.items() if v == '_ai_debug_render_actor'][0]
    oo = {v: k for k, v in ad.owners(ours).items()}

    def masked(raw, o, num):
        b = bytearray(raw)
        for r in cc.section_info_by_number(o, num)['relocations']:
            for k in range(4):
                if r['address'] + k < len(b):
                    b[r['address'] + k] = 0
        return bytes(b)

    ia = list(ad.md.disasm(masked(ja, ad.TGT, ji), 0))
    ib = list(ad.md.disasm(masked(ob, ours, oo['_ai_debug_render_actor']), 0))
    return ia, ib


def walk(ia, ib, quiet=False):
    """Returns (jan_extra, our_extra, mismatches). Lists of (addr, text, ctx)."""
    jan_extra, our_extra, mismatches = [], [], []
    i = j = 0
    LOOK = 6
    while i < len(ia) and j < len(ib):
        x, y = ia[i], ib[j]
        nx, ny = norm(x), norm(y)
        if x.mnemonic == y.mnemonic and nx == ny:
            i += 1; j += 1
            continue
        # branch targets differ constantly under drift; treat same-mnemonic
        # branches as matched
        if x.mnemonic == y.mnemonic and x.mnemonic[0] == 'j':
            i += 1; j += 1
            continue
        # try resync: is ia[i] an extra (ours lacks it)?
        resynced = False
        for skip in range(1, LOOK):
            if i + skip < len(ia) and norm(ia[i + skip]) == ny:
                ok = all(k + skip < len(ia) and j + k < len(ib) and
                         (norm(ia[i + skip + k]) == norm(ib[j + k]) or
                          ia[i + skip + k].mnemonic == ib[j + k].mnemonic)
                         for k in range(min(4, len(ib) - j)))
                if ok:
                    for s in range(skip):
                        jan_extra.append((ia[i + s].address,
                                          '%s %s' % (ia[i + s].mnemonic, ia[i + s].op_str)))
                    i += skip
                    resynced = True
                    break
            if j + skip < len(ib) and norm(ib[j + skip]) == nx:
                ok = all(i + k < len(ia) and j + skip + k < len(ib) and
                         (norm(ia[i + k]) == norm(ib[j + skip + k]) or
                          ia[i + k].mnemonic == ib[j + skip + k].mnemonic)
                         for k in range(min(4, len(ia) - i)))
                if ok:
                    for s in range(skip):
                        our_extra.append((ib[j + s].address,
                                          '%s %s' % (ib[j + s].mnemonic, ib[j + s].op_str)))
                    j += skip
                    resynced = True
                    break
        if resynced:
            continue
        # transposition? x==ib[j+1] and y==ia[i+1]
        if (i + 1 < len(ia) and j + 1 < len(ib)
                and norm(ia[i + 1]) == ny and norm(ib[j + 1]) == nx):
            mismatches.append((x.address, 'TRANSPOSED: %s %s <-> %s %s'
                               % (x.mnemonic, x.op_str, y.mnemonic, y.op_str)))
            i += 2; j += 2
            continue
        mismatches.append((x.address, 'jan: %s %s | ours(%04x): %s %s'
                           % (x.mnemonic, x.op_str, y.address, y.mnemonic, y.op_str)))
        i += 1; j += 1
    return jan_extra, our_extra, mismatches


if __name__ == '__main__':
    ia, ib = load()
    print('jan %d instrs, ours %d (delta %d)' % (len(ia), len(ib), len(ia) - len(ib)))
    je, oe, mm = walk(ia, ib)
    print('\n== JANUARY-ONLY instructions (%d):' % len(je))
    for a, t in je:
        print('  %04x  %s' % (a, t))
    print('\n== OURS-ONLY instructions (%d):' % len(oe))
    for a, t in oe:
        print('  %04x  %s' % (a, t))
    print('\n== residual mismatches (%d):' % len(mm))
    for a, t in mm[:40]:
        print('  %04x  %s' % (a, t))
