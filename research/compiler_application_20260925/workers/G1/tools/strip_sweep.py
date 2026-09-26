"""G1 DIAGNOSTIC strip-test sweep (scratch copies only; never a candidate generator by itself).

usage: python -B strip_sweep.py <unit> <fn> [<fn> ...] [--out-dir DIR]

For every DECORATIVE parenthesis pair inside <fn> (parscan classes LEFTRED, and WHOLE/LEAF groups whose removal
cannot change the parse), writes a scratch copy with ONLY that pair removed, compiles it with the unit's real flags
(tools/campaign/gate.py --fn), and reports whether the function's section changed relative to the unmodified source
and whether the change moves it to / towards January (alndiff block count). A change means the decoration is
LOAD-BEARING (owner ruling 2026-09-26 item 4: no decorative parentheses) - it is reported, not adopted.
"""
import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, 'tools')
import parscan  # noqa: E402
import coff_compare as cc  # noqa: E402

SAFE_BEFORE = {'=', '(', ',', 'return', '{', ';', '?', ':', '+=', '-=', '*=', '/='}
BINPREC = {'*': 13, '/': 13, '%': 13, '+': 12, '-': 12, '<<': 11, '>>': 11, '<': 10, '>': 10, '<=': 10, '>=': 10,
           '==': 9, '!=': 9, '&': 8, '^': 7, '|': 6, '&&': 5, '||': 4}
SAFE_AFTER = {';', ')', ',', ':', '?'}


def strip_keep(code):
    """Length-preserving comment/string blanking (offsets stay valid in the original text)."""
    blank = lambda m: re.sub(r'[^\n]', ' ', m.group(0))
    code = re.sub(r'/\*.*?\*/', blank, code, flags=re.S)
    code = re.sub(r'//[^\n]*', blank, code)
    code = re.sub(r'"(\\.|[^"\\\n])*"', lambda m: '"' + 'x' * (len(m.group(0)) - 2) + '"', code)
    code = re.sub(r"'(\\.|[^'\\\n])*'", lambda m: "'" + 'x' * (len(m.group(0)) - 2) + "'", code)
    return code


def binop_at(toks, i):
    """toks[i] is a BINARY operator (its left neighbour is an operand), for every operator incl. '&' and '*'."""
    if i <= 0 or toks[i][0] not in BINPREC:
        return False
    p = toks[i - 1][0]
    return p in (')', ']') or (re.match(r'[A-Za-z_0-9.]', p) is not None and p not in parscan.KEYWORDS)


def groups(code, fn):
    """(open_offset, close_offset, class, text) for decorative groups in fn."""
    stripped = strip_keep(code)
    assert len(stripped) == len(code)
    span = parscan.find_body(stripped, fn)
    if not span:
        return []
    out = []
    # re-tokenise WITH offsets
    toks = []
    for m in parscan.TOK.finditer(stripped, span[0], span[1]):
        if not m.group(0).isspace():
            toks.append((m.group(0), m.start()))
    stack = []
    for i, (t, off) in enumerate(toks):
        if t == '(':
            stack.append(i)
        elif t == ')' and stack:
            o = stack.pop()
            prev = toks[o - 1][0] if o else ''
            nxt = toks[i + 1][0] if i + 1 < len(toks) else ''
            if re.match(r'[A-Za-z_]\w*$', prev) and prev not in ('return',):
                continue
            if prev in (')', ']'):
                continue
            inner = [x[0] for x in toks[o + 1:i]]
            if not inner:
                continue
            if inner[0] in parscan.TYPEWORDS or (inner[-1] == '*' and all(re.match(r'\w+$|\*', x) for x in inner)):
                continue
            flat = [(y[0], 0) for y in toks]
            depth, top = 0, set()
            for k in range(o + 1, i):
                x = toks[k][0]
                if x in '([':
                    depth += 1
                elif x in ')]':
                    depth -= 1
                elif depth == 0 and x in BINPREC and binop_at(flat, k):
                    top.add(x)
                elif depth == 0 and x in ('?', ':', '=', '+=', '-=', '*=', '/=', ','):
                    top.add('?')
            kind = None
            if '?' in top:
                kind = None
            elif not top:
                if prev != 'sizeof' and nxt not in ('++', '--', '.', '->', '[', '(') and \
                        not (inner[0] in ('*', '&', '-', '!', '~', '+') and prev in BINPREC) and \
                        (prev in BINPREC or prev in SAFE_BEFORE) and (nxt in BINPREC or nxt in SAFE_AFTER):
                    kind = 'LEAF'
            else:
                root = min(BINPREC[x] for x in top)
                pbin = prev in BINPREC and o >= 1 and binop_at(flat, o - 1)
                if prev in BINPREC and not pbin:
                    kind = None                      # unary operator before the group (-(a*b), *(p+i), &(...))
                elif root in (12, 13):               # arithmetic groups only (hidden-temp family)
                    p_ok = (not pbin) or root > BINPREC[prev]
                    n_ok = (nxt not in BINPREC) or root >= BINPREC[nxt]
                    ctx_ok = pbin or prev in SAFE_BEFORE
                    nctx_ok = nxt in BINPREC or nxt in SAFE_AFTER
                    if p_ok and n_ok and ctx_ok and nctx_ok:
                        kind = 'LEFTRED' if (nxt in BINPREC and not pbin) else ('RIGHTRED' if pbin else 'WHOLE')
            if kind:
                line = stripped.count('\n', 0, toks[o][1]) + 1
                out.append((toks[o][1], toks[i][1], kind, line, ' '.join(inner)[:70]))
    return out


def fn_info(obj_path, fn):
    o = cc.load(open(obj_path, 'rb').read())
    return cc.section_info(o, fn)


def blocks(unit, fn, obj):
    out = subprocess.run(['python', '-B', 'tools/campaign/alndiff.py', unit, fn, '--ours-object', obj, '--max-lines', '0'],
                         capture_output=True, text=True).stdout
    return out.count('\n---')


def main():
    args = sys.argv[1:]
    outdir = Path(args[args.index('--out-dir') + 1]) if '--out-dir' in args else Path('scratch/campaign/workers/G1/strip')
    if '--out-dir' in args:
        i = args.index('--out-dir')
        del args[i:i + 2]
    unit, fns = args[0], args[1:]
    outdir.mkdir(parents=True, exist_ok=True)
    code = open(unit + '.c', encoding='latin-1', newline='').read()
    tgt = cc.load(open('build/split/' + unit + '.obj', 'rb').read())
    for fn in fns:
        base_obj = str(outdir / ('base_%s.obj' % fn.strip('_')))
        subprocess.run(['python', '-B', 'tools/campaign/gate.py', unit, '--fn', '_' + fn, '--out', base_obj],
                       capture_output=True, text=True)
        bi = fn_info(base_obj, '_' + fn)
        bb = blocks(unit, '_' + fn, base_obj)
        ti = cc.section_info(tgt, '_' + fn)
        gs = groups(code, fn)
        print('## %s: %d decorative groups; base blocks %d' % (fn, len(gs), bb))
        for n, (a, b, kind, line, text) in enumerate(gs):
            var = code[:a] + code[a + 1:b] + code[b + 1:]
            src = outdir / ('%s_s%02d.c' % (fn, n))
            open(src, 'w', encoding='latin-1', newline='').write(var)
            obj = str(outdir / ('%s_s%02d.obj' % (fn, n)))
            r = subprocess.run(['python', '-B', 'tools/campaign/gate.py', unit, '--source', str(src), '--fn', '_' + fn,
                                '--out', obj], capture_output=True, text=True)
            if 'COMPILE FAILED' in r.stdout or not os.path.exists(obj):
                print('   s%02d L%-5d %-7s COMPILE FAILED  (%s)' % (n, line, kind, text))
                continue
            vi = fn_info(obj, '_' + fn)
            same = cc.section_infos_equal(bi, vi)
            exact = cc.section_infos_equal(ti, vi)
            vb = bb if same else blocks(unit, '_' + fn, obj)
            print('   s%02d L%-5d %-7s %s%s blocks %d  (%s)' % (n, line, kind, 'inert  ' if same else 'CHANGED',
                                                             ' EXACT' if exact else '', vb, text))


if __name__ == '__main__':
    main()
