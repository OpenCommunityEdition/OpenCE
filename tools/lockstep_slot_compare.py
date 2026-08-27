# -*- coding: utf-8 -*-
"""Lockstep-compare render_actor: January vs ours, modulo frame-slot renaming.

Walks both disassemblies in parallel. Instructions must match mnemonic and
operand shape except [ebp - X] displacements, which build a bijection
jan_slot <-> our_slot. Reports: first shape mismatch, first bijection
conflict, first length drift, and the slot map so far at each event.
"""
import sys, re
sys.path.insert(0, r'C:\Users\isabe\AppData\Local\Temp\claude')
import ad
import coff_compare as cc

out, err = ad.gate.build(ad.UNIT)
ours = cc.load(out)
ja = ad.secbytes(ad.TGT, '_ai_debug_render_actor')
ob = ad.secbytes(ours, '_ai_debug_render_actor')

# mask relocation slots on both sides before disasm (house rules)
ji = [k for k, v in ad.JO.items() if v == '_ai_debug_render_actor'][0]
oo = {v: k for k, v in ad.owners(ours).items()}


def masked(raw, o, num):
    b = bytearray(raw)
    for r in cc.section_info_by_number(o, num)['relocations']:
        for k in range(4):
            if r['address'] + k < len(b):
                b[r['address'] + k] = 0
    return bytes(b)


ja_m = masked(ja, ad.TGT, ji)
ob_m = masked(ob, ours, oo['_ai_debug_render_actor'])

ia = list(ad.md.disasm(ja_m, 0))
ib = list(ad.md.disasm(ob_m, 0))
print('instruction counts: jan %d ours %d' % (len(ia), len(ib)))

SLOT = re.compile(r'ebp - (0x[0-9a-f]+|\d+)')

j2o = {}
o2j = {}
events = 0
k = 0
n = min(len(ia), len(ib))
while k < n and events < 12:
    x, y = ia[k], ib[k]
    xs = SLOT.findall(x.op_str)
    ys = SLOT.findall(y.op_str)
    xn = SLOT.sub('ebp-S', x.op_str)
    yn = SLOT.sub('ebp-S', y.op_str)
    if x.mnemonic != y.mnemonic or xn != yn or len(xs) != len(ys):
        print('SHAPE MISMATCH at #%d  jan %04x: %s %s   |   ours %04x: %s %s'
              % (k, x.address, x.mnemonic, x.op_str, y.address, y.mnemonic, y.op_str))
        events += 1
        # try to resync: skip the extra instruction on the longer side
        # heuristic: if jan has an instruction ours lacks, skip jan
        if k + 1 < len(ia) and SLOT.sub('ebp-S', ia[k + 1].op_str) == yn and ia[k + 1].mnemonic == y.mnemonic:
            print('   -> resync: JANUARY has an extra instruction here')
            ia.pop(k)
            n = min(len(ia), len(ib))
            continue
        if k + 1 < len(ib) and SLOT.sub('ebp-S', ib[k + 1].op_str) == xn and ib[k + 1].mnemonic == x.mnemonic:
            print('   -> resync: OURS has an extra instruction here')
            ib.pop(k)
            n = min(len(ia), len(ib))
            continue
        k += 1
        continue
    for a, b in zip(xs, ys):
        av, bv = int(a, 0), int(b, 0)
        if av in j2o and j2o[av] != bv:
            print('BIJECTION CONFLICT at #%d jan %04x: jan slot -%#x mapped to -%#x, now -%#x'
                  % (k, x.address, av, j2o[av], bv))
            print('   jan: %s %s | ours: %s %s' % (x.mnemonic, x.op_str, y.mnemonic, y.op_str))
            events += 1
        if bv in o2j and o2j[bv] != av:
            pass  # symmetric, reported above
        j2o[av] = bv
        o2j[bv] = av
    k += 1

print('walked %d instructions; %d slot pairs' % (k, len(j2o)))
print('slot map (jan -> ours), sorted:')
for a in sorted(j2o):
    m = ' *' if j2o[a] != a else ''
    print('  -%#05x -> -%#05x%s' % (a, j2o[a], m))
