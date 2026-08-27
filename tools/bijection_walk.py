"""Generalized lockstep walker for units residuals.

Normalizations before comparison:
 - branch/call targets -> ordinal labels (per-stream), so layout drift
   does not cascade
 - reloc-covered displacements already zeroed by capstone? no - we zero
   them by rewriting [*] any absolute dword displacement that matches a
   relocation address is replaced by 'RELOC'
Tiers:
 1 exact  2 register-bijection  3 bijection-extension EVENT  4 STRUCTURAL
Alignment: on structural mismatch, try to resync with lookahead 1-3 on
either side (records INSERT/DELETE events).
"""
import sys, re
sys.path.insert(0, 'tools')
import coff_compare as cc
import capstone

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

import argparse
_ap = argparse.ArgumentParser()
_ap.add_argument('--unit', default='source/units/units')
_ap.add_argument('functions', nargs='*')
_ap.add_argument('-v', action='store_true')
_ap.add_argument('--max-events', type=int, default=40,
                help='cap on printed events (0 = all). A low cap can hide a '
                     'systematic cause: render_actor looks like 3953 unrelated '
                     'events but is one frame-layout difference repeated.')
_ARGS = _ap.parse_args()
t_obj = cc.load(open(f'build/split/{_ARGS.unit}.obj', 'rb').read())
b_obj = cc.load(open(f'build/base/{_ARGS.unit}.obj', 'rb').read())

REGS = ['eax', 'ecx', 'edx', 'ebx', 'esi', 'edi']
SUB = {'al': 'eax', 'ah': 'eax', 'ax': 'eax', 'cl': 'ecx', 'ch': 'ecx', 'cx': 'ecx',
       'dl': 'edx', 'dh': 'edx', 'dx': 'edx', 'bl': 'ebx', 'bh': 'ebx', 'bx': 'ebx',
       'si': 'esi', 'di': 'edi'}
WIDTH = {'eax': {'eax': 'eax', '16': 'ax', 'lo': 'al', 'hi': 'ah'},
         'ecx': {'ecx': 'ecx', '16': 'cx', 'lo': 'cl', 'hi': 'ch'},
         'edx': {'edx': 'edx', '16': 'dx', 'lo': 'dl', 'hi': 'dh'},
         'ebx': {'ebx': 'ebx', '16': 'bx', 'lo': 'bl', 'hi': 'bh'},
         'esi': {'esi': 'esi', '16': 'si'},
         'edi': {'edi': 'edi', '16': 'di'}}


def prep(o, name):
    secs = o['sections']
    for s in o['symbols']:
        if s['name'] == name and s['section'] > 0 and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text':
            sec = secs[s['section'] - 1]
            data = cc._section_bytes(o, sec)
            si = cc.section_info(o, name)
            reladdrs = set()
            for r in si['relocations']:
                reladdrs.add(r['address'])
            insns = list(md.disasm(bytes(data), 0))
            out = []
            # collect branch targets for labeling
            targets = {}
            for i in insns:
                if (i.mnemonic.startswith('j') or i.mnemonic == 'call') and re.fullmatch(r'0x[0-9a-f]+', i.op_str):
                    a = int(i.op_str, 16)
                    if a not in targets:
                        targets[a] = f'L{len(targets)}'
            for i in insns:
                txt = f'{i.mnemonic} {i.op_str}'
                # reloc-covered immediate/displacement -> RELOC
                for ra in range(i.address, i.address + i.size):
                    if ra in reladdrs:
                        txt = re.sub(r'0x[0-9a-f]{4,8}', 'RELOC', txt)
                        txt = re.sub(r'\[(e[a-z]{2})( [\+\-] e?[a-z]{2}(\*\d)?)? [\+\-] \]', r'[\1\2+RELOC]', txt)
                        break
                if (i.mnemonic.startswith('j') or i.mnemonic == 'call') and re.fullmatch(r'0x[0-9a-f]+', i.op_str):
                    txt = f'{i.mnemonic} {targets[int(i.op_str, 16)]}'
                out.append((i.address, txt))
            return out
    return None


def canon(r):
    return SUB.get(r, r)


def apply_map(txt, mapping):
    def rep(m):
        r = m.group(0)
        base = canon(r)
        if base in mapping:
            nb = mapping[base]
            if r == base:
                return nb
            if r in ('ax', 'cx', 'dx', 'bx', 'si', 'di'):
                return WIDTH[nb].get('16', r)
            if r in ('al', 'cl', 'dl', 'bl'):
                return WIDTH[nb].get('lo', r)
            if r in ('ah', 'ch', 'dh', 'bh'):
                return WIDTH[nb].get('hi', r)
        return r
    return re.sub(r'\b(e?[abcd][xlh]|e?[sd]i|[abcd][lh])\b', rep, txt)


def regs_of(txt):
    seen = []
    for m in re.finditer(r'\b(e?[abcd][xlh]|e?[sd]i|[abcd][lh])\b', txt):
        r = canon(m.group(1))
        if r in REGS and r not in seen:
            seen.append(r)
    return seen


def shape(txt):
    return re.sub(r'\b(e?[abcd][xlh]|e?[sd]i|[abcd][lh])\b', 'R', txt)


def walk(fn):
    lt = prep(t_obj, fn)
    lo = prep(b_obj, fn)
    if lt is None or lo is None:
        return None
    i = j = 0
    mapping = {}
    events = []
    while i < len(lt) and j < len(lo):
        at, xt = lt[i]
        ao, xo = lo[j]
        if xt == xo or apply_map(xt, mapping) == xo:
            i += 1
            j += 1
            continue
        if shape(xt) == shape(xo):
            rt, ro = regs_of(xt), regs_of(xo)
            newmap = dict(mapping)
            ok = True
            for a, b2 in zip(rt, ro):
                if a in newmap and newmap[a] != b2:
                    pass  # override
                newmap[a] = b2
            events.append(('REG', at, xt, xo))
            mapping = {k: v for k, v in newmap.items() if k != v}
            i += 1
            j += 1
            continue
        # try resync
        resynced = False
        for skip in (1, 2, 3):
            if i + skip < len(lt) and lt[i + skip][1] == xo:
                for k in range(skip):
                    events.append(('T-EXTRA', lt[i + k][0], lt[i + k][1], ''))
                i += skip
                resynced = True
                break
            if j + skip < len(lo) and apply_map(xt, mapping) == lo[j + skip][1]:
                for k in range(skip):
                    events.append(('O-EXTRA', lo[j + k][0], '', lo[j + k][1]))
                j += skip
                resynced = True
                break
        if not resynced:
            events.append(('STRUCT', at, xt, xo))
            i += 1
            j += 1
    return events


for fn in _ARGS.functions:
    ev = walk(fn)
    if ev is None:
        print(f'{fn}: not found')
        continue
    from collections import Counter
    c = Counter(k for k, _, _, _ in ev)
    print(f'{fn}: {len(ev)} events  {dict(c)}')
    if _ARGS.v or len(ev) <= 12:
        shown = ev if _ARGS.max_events == 0 else ev[:_ARGS.max_events]
        for k, a, xt, xo in shown:
            print(f'   {k:8s} @{a:#x}  T:{xt:42s} O:{xo}')
        if len(shown) < len(ev):
            print(f'   ... {len(ev) - len(shown)} more events '
                  f'(--max-events 0 for all; the cap can hide a systematic cause)')
