#!/usr/bin/env python3
"""A2 copy (only changes: SPLIT_ROOT env overrides build/split; ROOT pinned). Whole-object admission audit for ONE unit (read-only).

    python scratch/tools/object_audit.py <unit> [ours.obj]

Compares January's split object (build/split/<unit>.obj) with ours
(build/base/<unit>.obj by default):

1. every January-owned section is matched by its OWNER symbol (the first
   non-section symbol defined in it; plain sections fall back to name+order)
   and compared with coff_compare.section_infos_equal (bytes, size,
   relocations with hardened destination resolution);
2. every January symbol: present in ours, same storage class (external 2 /
   static 3), same owning-section key, same value (offset);
3. candidate-only surplus: sections/symbols ours defines that January's
   object does not (these need the provider-link check and earn no credit).

.debug$* sections are ignored; .drectve is compared by bytes.
Exit 0 only if every January-owned section and symbol matches.
"""
import os
import sys

ROOT = 'C:/halo-worktrees/claude-compiler-application-20260925'
os.chdir(ROOT)
sys.path.insert(0, 'tools')
import coff_compare as cc  # noqa: E402


def owners(obj):
    """section number -> owner key."""
    first = {}
    for s in obj['symbols']:
        n = s['section']
        if n <= 0 or s['name'].startswith('.') or s['name'].startswith('$'):
            continue
        if s['storage'] not in (2, 3):
            continue
        first.setdefault(n, s['name'])
    keys = {}
    counts = {}
    for sec in obj['sections']:
        n = sec['index']
        if sec['name'].startswith('.debug'):
            continue
        if n in first:
            keys[n] = 'sym:' + first[n]
        else:
            c = counts.get(sec['name'], 0)
            counts[sec['name']] = c + 1
            keys[n] = 'sec:%s#%d' % (sec['name'], c)
    return keys


def symbol_rows(obj, keys):
    rows = {}
    for s in obj['symbols']:
        if s['name'].startswith('.') or s['name'].startswith('$') or s['section'] <= 0:
            continue
        if s['storage'] not in (2, 3):
            continue
        rows[s['name']] = (s['storage'], keys.get(s['section'], '?'), s['value'])
    return rows


def main():
    unit = sys.argv[1]
    ours_path = sys.argv[2] if len(sys.argv) > 2 else 'build/base/%s.obj' % unit
    t = cc.load(open(os.path.join(os.environ.get('SPLIT_ROOT', 'build/split'), '%s.obj' % unit), 'rb').read())
    o = cc.load(open(ours_path, 'rb').read())
    tk, ok = owners(t), owners(o)
    o_by_key = {v: k for k, v in ok.items()}
    # a section is the same section if ours holds the January owner symbol in ANY position
    o_sec_of = {s['name']: s['section'] for s in o['symbols'] if s['section'] > 0}
    for n, key in tk.items():
        if key.startswith('sym:') and key not in o_by_key and key[4:] in o_sec_of:
            o_by_key[key] = o_sec_of[key[4:]]
            ok[o_sec_of[key[4:]]] = key
    bad = 0
    print('== sections (January-owned)')
    for n, key in sorted(tk.items(), key=lambda kv: kv[1]):
        tsec = t['sections'][n - 1]
        on = o_by_key.get(key)
        if on is None:
            print('  MISSING  %-12s %-50s size %d' % (tsec['name'], key, tsec['size']))
            bad += 1
            continue
        osec = o['sections'][on - 1]
        try:
            ti = cc.section_info_by_number(t, n)
            oi = cc.section_info_by_number(o, on)
            eq = cc.section_infos_equal(ti, oi)
        except Exception as e:  # noqa: BLE001
            eq = False
            print('  ERROR    %s %s' % (key, e))
        flags_eq = (tsec['flags'] & ~0x00F00000) == (osec['flags'] & ~0x00F00000)
        align_t, align_o = (tsec['flags'] >> 20) & 0xF, (osec['flags'] >> 20) & 0xF
        state = 'ok' if eq and flags_eq and align_t == align_o else 'DIFF'
        if state != 'ok':
            bad += 1
        print('  %-4s %-8s %-52s size %5d/%-5d flags %s align %d/%d' % (
            state, tsec['name'], key[:52], tsec['size'], osec['size'],
            'eq' if flags_eq else '%08x/%08x' % (tsec['flags'], osec['flags']), align_t, align_o))
    print('== symbols (January)')
    ts, os_ = symbol_rows(t, tk), symbol_rows(o, ok)
    o_sec_num = {s['name']: s['section'] for s in o['symbols'] if s['section'] > 0}
    sym_bad = 0
    for name, (st, key, val) in sorted(ts.items()):
        if name not in os_:
            print('  MISSING  %s' % name)
            sym_bad += 1
            continue
        ost, okey, oval = os_[name]
        same_section = (key == okey) or (o_by_key.get(key) is not None and o_sec_num.get(name) == o_by_key.get(key))
        if (st, val) != (ost, oval) or not same_section:
            print('  DIFF     %-44s storage %d/%d key %s/%s value %d/%d' % (name, st, ost, key, okey, val, oval))
            sym_bad += 1
    print('  %d January symbols, %d differ' % (len(ts), sym_bad))
    bad += sym_bad
    print('== candidate-only surplus (no credit; needs provider link)')
    tkeys = set(tk.values())
    for n, key in sorted(ok.items(), key=lambda kv: kv[1]):
        if key not in tkeys:
            sec = o['sections'][n - 1]
            print('  +%-8s %-52s size %d' % (sec['name'], key[:52], sec['size']))
    print('OBJECT AUDIT: %s' % ('PASS' if not bad else 'FAIL (%d)' % bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
