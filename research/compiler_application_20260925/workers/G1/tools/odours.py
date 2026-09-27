"""G1 read-only census tool: compare the FP expression TREES of OUR source with the first-party /Od build.

usage: python -B odours.py <unit> <fn> <od_va_hex> [--source copy.c] [--raw]

1. Compiles OUR source (or a copy) with VC7 at /Od (the unit's flags with /O* replaced by /Od; scratch only) and
   symbolically executes the x87 code of <fn> (od_expr.symexec) -> our trees in source evaluation order.
2. Symbolically executes the first-party /Od function at <od_va> (SSE) -> its trees.
3. Reduces both to SHAPES (leaf identities abstracted: K(value) kept, every other leaf -> 'x', calls -> 'call') and
   aligns the two store/argument/compare sequences; prints aligned pairs whose shapes differ but whose operator
   multisets are equal (= a REGROUPING or operand-order difference), plus unmatched rows.
A /Od build does not reassociate: the printed trees are the parse trees of the two sources.
"""
import difflib
import importlib.util
import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, 'tools')
import coff_compare as cc  # noqa: E402
import od_expr as ox  # noqa: E402
import lst  # noqa: E402
import capstone  # noqa: E402


def our_od_rows(unit, fn, src):
    code = open(src, encoding='latin-1').read()
    tmp = 'scratch/_g1od_%d' % os.getpid()
    open(tmp + '.c', 'w', encoding='latin-1', newline='\n').write(code)
    cl = os.path.abspath(os.path.join('xbox', 'bin', 'vc7', 'CL.Exe'))
    fl = [t for t in lst.flags(unit) if not re.match(r'/O[a-z0-9-]*$', t)] + ['/Od']
    r = subprocess.run([cl, '/nologo', '/c'] + fl + ['/I' + os.path.dirname(unit + '.c'), '/Fo' + tmp + '.obj',
                        tmp + '.c'], capture_output=True, text=True)
    if r.returncode:
        sys.exit(r.stdout[-3000:])
    obj = cc.load(open(tmp + '.obj', 'rb').read())
    for f in (tmp + '.c', tmp + '.obj'):
        os.remove(f)
    sym = cc.symbol(obj, fn)
    sec = obj['sections'][sym['section'] - 1]
    raw = bytearray(obj['data'][sec['raw']:sec['raw'] + sec['size']])
    starts = sorted(s['value'] for s in obj['symbols'] if s['section'] == sym['section'] and s['type'] == 0x20)
    lo = sym['value']
    hi = min([v for v in starts if v > lo] or [sec['size']])
    import struct
    rel = {}
    for ri in range(sec['reloc_count']):
        roff = sec['reloc'] + ri * 10
        addr, tidx, rtype = struct.unpack_from('<LLH', obj['data'], roff)
        rel[addr] = obj['by_index'][tidx]['name']
        raw[addr:addr + 4] = b'\0\0\0\0'
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    insns = [i for i in md.disasm(bytes(raw), 0) if lo <= i.address < hi]
    imap = {}
    for ins in insns:
        for a in range(ins.address, ins.address + ins.size):
            if a in rel:
                imap[ins.address] = rel[a]
    ox.RELOC = imap
    rows = ox.symexec(insns, 0, False, reloc_at=lambda ins: ins.address)
    ox.RELOC = None
    return rows


LEAF = re.compile(r"K\([^)]*\)|G\([^)]*\)|ret\([^)]*\)|\{[^{}]*\}[-+](?:0x)?[0-9a-f]+|\*\{[^{}]*\}[-+](?:0x)?[0-9a-f]+|"
                  r"[vp][0-9a-f]+|g[0-9a-f]+|ARG@esp\+\w+|\[[^\]]*\]|bits\([^)]*\)|0\b")


def shape(expr):
    prev = None
    s = expr
    while prev != s:          # nested braces: reduce innermost first
        prev = s
        s = re.sub(r'\{[^{}]*\}', 'R', s)
    s = re.sub(r"K\(([^)]*)\)", r'K\1', s)
    s = re.sub(r"ret\([^)]*\)", 'call', s)
    s = re.sub(r"G\([^)]*\)|\*?R[-+](?:0x)?[0-9a-f]+|\bR\b|[vp][0-9a-f]+\b|g[0-9a-f]+\b|ARG@esp\+\w+|\[[^\]]*\]|bits\([^)]*\)",
               'x', s)
    s = re.sub(r'\(real\)|\(long\)|\(double\)|\(float\)', '', s)
    return s


def kind(row):
    body = row.split('  ', 2)[-1].strip() if '  ' in row else row
    m = re.match(r'(\S+) := (.*)$', body)
    if m:
        return ('ST', m.group(2))
    m = re.match(r'F?CMP (.*) \? (.*)$', body)
    if m:
        return ('CMP', m.group(1) + ' ? ' + m.group(2))
    if body.startswith('CALL'):
        return ('CALL', '')
    return ('?', body)


def ops(sh):
    return sorted(re.findall(r' [-+*/] ', sh)) + sorted(re.findall(r'K[-0-9.e]+', sh))


def main():
    args = sys.argv[1:]
    unit, fn, va = args[0], args[1], int(args[2], 16)
    src = args[args.index('--source') + 1] if '--source' in args else unit + '.c'
    ours = [r for r in our_od_rows(unit, fn, src) if kind(r)[0] in ('ST', 'CMP')]
    theirs = [r for r in ox.run(va, False) if kind(r)[0] in ('ST', 'CMP')]
    fp = lambda e: bool(re.search(r' [-+*/] |K\(', e))
    ours = [r for r in ours if fp(kind(r)[1])]
    theirs = [r for r in theirs if fp(kind(r)[1])]
    a = [shape(kind(r)[1]) for r in theirs]
    b = [shape(kind(r)[1]) for r in ours]
    if '--raw' in args:
        for r in theirs:
            print('OD  ', r)
        for r in ours:
            print('OUR ', r)
        return
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    print('## %s: /Od %d FP rows, ours(/Od) %d FP rows, equal-shape %d' % (
        fn, len(a), len(b), sum(i2 - i1 for t, i1, i2, j1, j2 in sm.get_opcodes() if t == 'equal')))
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == 'equal':
            continue
        for k in range(max(i2 - i1, j2 - j1)):
            ta = theirs[i1 + k] if i1 + k < i2 else ''
            tb = ours[j1 + k] if j1 + k < j2 else ''
            same_ops = ta and tb and ops(shape(kind(ta)[1])) == ops(shape(kind(tb)[1]))
            flag = 'REGROUP?' if same_ops else tag.upper()
            print('--- %s' % flag)
            if ta:
                print('   OD : %s' % ta)
            if tb:
                print('   OUR: %s' % tb)


if __name__ == '__main__':
    main()
