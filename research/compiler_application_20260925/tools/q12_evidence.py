"""Q12 literal evidence: COMDAT selection, complete contents, current providers and both-order link receipts.

usage (worktree root): python -B research/compiler_application_20260925/tools/q12_evidence.py <out_dir>

For each Q12 data section (four units) and every '??_C@' literal its relocations reference:
  - every January definer (all build/split objects defining the literal externally) and every current definer in
    our build (all build/base objects), each with section flags, alignment, COMDAT bit, the section-definition aux
    record (length, relocation count, checksum, number, selection), the symbols in the section and the COMPLETE
    section bytes;
  - checks: COMDAT with selection ANY everywhere; one literal symbol per section; no relocations; identical size,
    alignment and complete bytes across every definer on both sides;
  - for literals January's unit leaves UNDEF while ours defines them: link our unit object with our current base
    object of each January provider, in both input orders, with the pinned VC7 Link.Exe, and save the complete
    linker output as a receipt (PASS = no LNK2005 / LNK1169).
Writes literals.json, literals.txt, receipts/*.txt and prints a summary. Pure reader except for the temporary link
outputs (a temp directory, deleted)."""
import hashlib
import json
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
LINK = ROOT / 'xbox' / 'bin' / 'vc7' / 'Link.Exe'
SEL = {0: 'none', 1: 'NODUPLICATES', 2: 'ANY', 3: 'SAME_SIZE', 4: 'EXACT_MATCH', 5: 'ASSOCIATIVE', 6: 'LARGEST'}
UNITS = [
    ('source/bitmaps/bitmap_group', '_global_bitmap_reference'),
    ('source/ai/ai_communication', '_global_communication_priority_names'),
    ('source/ai/ai_debug', '_global_ai_debug_firing_position_color_count'),
    ('source/game/game_engine_king', '_king_engine'),
]


def rd(path):
    b = Path(path).read_bytes()
    nsec = struct.unpack_from('<H', b, 2)[0]
    symptr, nsym = struct.unpack_from('<II', b, 8)
    strtab = symptr + nsym * 18
    secs = []
    for i in range(nsec):
        o = 20 + i * 40
        size, raw, rel, _, nrel, _, flags = struct.unpack_from('<IIIIHHI', b, o + 16)
        secs.append(dict(n=i + 1, name=b[o:o + 8].rstrip(b'\0').decode('latin1'), size=size, raw=raw, rel=rel,
                         nrel=nrel, flags=flags))
    syms, byidx, i = [], {}, 0
    while i < nsym:
        e = b[symptr + i * 18: symptr + i * 18 + 18]
        if e[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', e, 4)[0]
            n = b[strtab + off: b.index(b'\0', strtab + off)].decode('latin1')
        else:
            n = e[:8].rstrip(b'\0').decode('latin1')
        val, sec, typ, cls, aux = struct.unpack_from('<IhHBB', e, 8)
        s = dict(i=i, name=n, value=val, sec=sec, cls=cls, aux=b[symptr + (i + 1) * 18: symptr + (i + 1 + aux) * 18])
        syms.append(s)
        byidx[i] = s
        i += 1 + aux
    return dict(b=b, secs=secs, syms=syms, byidx=byidx)


def definition(o, path, name):
    """Describe the external definition of `name` in object `o`, or None."""
    for s in o['syms']:
        if s['name'] == name and s['sec'] > 0 and s['cls'] == 2:
            sec = o['secs'][s['sec'] - 1]
            raw = o['b'][sec['raw']:sec['raw'] + sec['size']] if sec['raw'] else b'\0' * sec['size']
            aux = None
            for t in o['syms']:
                if t['sec'] == sec['n'] and t['cls'] == 3 and t['value'] == 0 and t['name'] == sec['name'] and t['aux']:
                    length, nr, nl, chk, num, sel = struct.unpack_from('<IHHIHB', t['aux'], 0)
                    aux = dict(length=length, nrel=nr, checksum='%08x' % chk, number=num, selection=sel,
                               selection_name=SEL.get(sel, '?'))
            return dict(object=Path(path).relative_to(ROOT).as_posix(),
                        object_sha256=hashlib.sha256(o['b']).hexdigest(),
                        section=sec['n'], section_name=sec['name'], flags='%08x' % sec['flags'],
                        comdat=bool(sec['flags'] & 0x1000), align_code=(sec['flags'] >> 20) & 0xF,
                        size=sec['size'], relocations=sec['nrel'], symbol_value=s['value'],
                        symbols_in_section=[t['name'] for t in o['syms'] if t['sec'] == sec['n']],
                        aux=aux, bytes_hex=raw.hex(), bytes_sha256=hashlib.sha256(raw).hexdigest())
    return None


INDEX = {}


def definers(tree, name):
    if tree not in INDEX:
        INDEX[tree] = [(p, p.read_bytes()) for p in sorted((ROOT / 'build' / tree).rglob('*.obj'))]
    out = []
    nb = name.encode('latin1')
    for p, data in INDEX[tree]:
        if nb in data:
            d = definition(rd(p), p, name)
            if d:
                out.append(d)
    return out


def section_literals(unit, owner):
    o = rd(ROOT / 'build' / 'split' / (unit + '.obj'))
    osec = next(o['secs'][s['sec'] - 1] for s in o['syms'] if s['name'] == owner and s['sec'] > 0)
    names = []
    for k in range(osec['nrel']):
        va, si, t = struct.unpack_from('<IIH', o['b'], osec['rel'] + k * 10)
        n = o['byidx'][si]['name']
        if n.startswith('??_C@') and n not in names:
            names.append(n)
    return o, names


def link(objs):
    with tempfile.TemporaryDirectory() as td:
        cmd = [str(LINK), '/NOLOGO', '/MACHINE:X86', '/SUBSYSTEM:CONSOLE', '/NODEFAULTLIB', '/ENTRY:probe_entry',
               '/OUT:' + os.path.join(td, 'probe.exe')] + [str(x) for x in objs]
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    return cmd, r.returncode, r.stdout + r.stderr


def main():
    out = Path(sys.argv[1])
    (out / 'receipts').mkdir(parents=True, exist_ok=True)
    report, lines, fails = [], [], 0
    for unit, owner in UNITS:
        jan_unit_obj, names = section_literals(unit, owner)
        ours_path = ROOT / 'build' / 'base' / (unit + '.obj')
        ours = rd(ours_path)
        lines.append('=== %s (%s .data): %d literal targets' % (unit, owner, len(names)))
        for name in names:
            jan_here = definition(jan_unit_obj, ROOT / 'build' / 'split' / (unit + '.obj'), name)
            ours_here = definition(ours, ours_path, name)
            jdefs = definers('split', name)
            odefs = definers('base', name)
            everyone = jdefs + odefs
            checks = {
                'ours_unit_defines': ours_here is not None,
                'january_has_exactly_one_definer': len(jdefs) == 1,
                'all_comdat_select_any': all(d['comdat'] and d['aux'] and d['aux']['selection'] == 2 for d in everyone),
                'one_literal_symbol_per_section': all(
                    [x for x in d['symbols_in_section'] if x != d['section_name']] == [name] for d in everyone),
                'no_relocations': all(d['relocations'] == 0 for d in everyone),
                'same_size': len({d['size'] for d in everyone}) == 1,
                'same_alignment': len({d['align_code'] for d in everyone}) == 1,
                'same_complete_bytes': len({d['bytes_sha256'] for d in everyone}) == 1,
                'aux_length_equals_size': all(d['aux'] and d['aux']['length'] == d['size'] for d in everyone),
            }
            row = dict(unit=unit, literal=name, january_unit='defined here' if jan_here else 'UNDEF',
                       january_definers=jdefs, current_definers_ours=odefs, checks=checks,
                       informational=dict(
                           aux_checksums=sorted({(d['object'].split('/')[1], d['aux']['checksum']) for d in everyone}),
                           aux_numbers=sorted({(d['object'].split('/')[1], d['aux']['number']) for d in everyone})))
            bad = [k for k, v in checks.items() if not v]
            fails += bool(bad)
            lines.append('%s %-40s January-unit %-12s Jan definers %d, our definers %d, size %d, bytes %s%s' % (
                'PASS' if not bad else 'FAIL', name, row['january_unit'], len(jdefs), len(odefs),
                everyone[0]['size'], bytes.fromhex(everyone[0]['bytes_hex'])[:24], '' if not bad else ' FAILED %s' % bad))
            if not jan_here and ours_here:
                row['link_receipts'] = []
                for jd in jdefs:
                    prov = ROOT / 'build' / 'base' / Path(jd['object']).relative_to('build/split')
                    prov_def = definition(rd(prov), prov, name) if prov.exists() else None
                    for order, objs in (('unit_first', [ours_path, prov]), ('provider_first', [prov, ours_path])):
                        cmd, rc, text = link(objs)
                        dup = [l for l in text.splitlines() if 'LNK2005' in l or 'LNK1169' in l]
                        verdict = 'PASS' if not dup else 'FAIL'
                        fails += verdict != 'PASS'
                        tag = '%s__%s__%s' % (unit.split('/')[-1], hashlib.sha1(name.encode()).hexdigest()[:8], order)
                        receipt = out / 'receipts' / (tag + '.txt')
                        receipt.write_text(
                            'literal: %s\nunit object: %s (sha256 %s)\nprovider (January selected: %s): %s (sha256 %s)\n'
                            'provider currently defines it: %s\norder: %s\ncommand: %s\nexit code: %d\n'
                            'LNK2005/LNK1169 lines: %d\nverdict: %s\n--- complete linker output ---\n%s' % (
                                name, ours_path.relative_to(ROOT).as_posix(),
                                hashlib.sha256(ours_path.read_bytes()).hexdigest(), jd['object'],
                                prov.relative_to(ROOT).as_posix(), hashlib.sha256(prov.read_bytes()).hexdigest(),
                                'yes, COMDAT %s, bytes sha256 %s' % (prov_def['aux']['selection_name'],
                                                                   prov_def['bytes_sha256']) if prov_def else 'NO',
                                order, ' '.join(os.path.relpath(c, ROOT) if os.path.isfile(c) else c for c in cmd),
                                rc, len(dup), verdict, text), encoding='utf-8', newline='\n')
                        row['link_receipts'].append(dict(order=order, provider=prov.relative_to(ROOT).as_posix(),
                                                         provider_defines=bool(prov_def), exit_code=rc,
                                                         duplicate_lines=dup, verdict=verdict,
                                                         receipt=receipt.relative_to(out).as_posix()))
                        lines.append('     link %-14s with %-44s rc %d  %s  (%s)' % (
                            order, prov.relative_to(ROOT / 'build' / 'base').as_posix(), rc, verdict,
                            receipt.relative_to(out).as_posix()))
            report.append(row)
    # negative control: the probe must report a duplicate when one exists (unit + a byte copy of itself: the
    # non-COMDAT .data owner is defined twice)
    for unit, owner in UNITS:
        src = ROOT / 'build' / 'base' / (unit + '.obj')
        with tempfile.TemporaryDirectory() as td:
            dup_copy = Path(td) / ('copy_' + src.name)
            dup_copy.write_bytes(src.read_bytes())
            cmd, rc, text = link([src, dup_copy])
        dup = [l for l in text.splitlines() if 'LNK2005' in l or 'LNK1169' in l]
        hit = any((' %s already defined' % owner) in l for l in dup)
        fails += not hit
        receipt = out / 'receipts' / ('CONTROL_%s__self_duplicate.txt' % unit.split('/')[-1])
        receipt.write_text('negative control: %s linked with a byte copy of itself\n'
                           'expected: LNK2005 naming %s\n'
                           'command: %s\nexit code: %d\nLNK2005/LNK1169 lines: %d\nowner reported: %s\nverdict: %s\n'
                           '--- complete linker output ---\n%s' % (
                               src.relative_to(ROOT).as_posix(), owner,
                               ' '.join(os.path.relpath(c, ROOT) if os.path.isfile(c) and str(c).startswith(str(ROOT))
                                        else str(c) for c in cmd),
                               rc, len(dup), hit, 'CONTROL DETECTED' if hit else 'CONTROL MISSED', text),
                           encoding='utf-8', newline='\n')
        lines.append('CONTROL %-32s self-duplicate link: rc %d, %d duplicate lines, owner %s reported: %s' % (
            unit, rc, len(dup), owner, hit))
    lines.append('RESULT %s (%d literal rows)' % ('PASS' if not fails else 'FAIL (%d)' % fails, len(report)))
    (out / 'literals.json').write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8', newline='\n')
    (out / 'literals.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    print('\n'.join(lines))
    return 1 if fails else 0


sys.exit(main())
