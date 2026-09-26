"""G1 census tool (read-only): list every GROUPING parenthesis inside a function body of OUR source and classify it.

usage: python -B parscan.py <source.c> <fn> [<fn> ...]

Classes (C tree semantics; types are NOT known here, FP-ness is judged by hand afterwards):
  REGROUP   the group is the RIGHT operand of an operator of the SAME precedence class as its own top-level operator
            (a*(b/c), a+(b-c), a-(b+c) ...). The tree differs from the flat left-to-right spelling, so the first-party
            /Od evaluation order CAN confirm or refute the grouping (evidence-bearing site).
  LEFTRED   the group is the LEFT operand of a same-class operator ((a*b)*c, (a/b)*c): tree-identical to the flat
            spelling; /Od cannot discriminate (a change here is parenthesis-only: owner-held class).
  NEEDED    required by precedence ((a+b)*c): the grouping is the tree itself; /Od order confirms or refutes it.
  LEAF      parentheses around a single operand ((x), (a->b)): tree-identical; /Od cannot discriminate.
  WHOLE     parentheses around a whole operand with no adjacent arithmetic operator (x = (a*b);).
"""
import re
import sys

KEYWORDS = {'if', 'while', 'for', 'switch', 'return', 'sizeof', 'else', 'do', 'case'}
TYPEWORDS = {'real', 'float', 'double', 'long', 'short', 'int', 'char', 'unsigned', 'signed', 'byte', 'word', 'dword',
             'boolean', 'void', 'struct', 'const', 'volatile', 'enum', 'union', 'angle', 'fixed', 'string_id',
             'datum_index', 'size_t', 'long_fixed', 'short_fixed'}
PREC = {'*': 'mul', '/': 'mul', '%': 'mul', '+': 'add', '-': 'add'}
TOK = re.compile(r'0x[0-9a-fA-F]+|\d+\.\d*[fF]?|\d*\.\d+[fF]?|\d+[eE][-+]?\d+[fF]?|\d+[uUlL]*|[A-Za-z_]\w*|->|\+\+|--|'
                 r'<<=|>>=|<=|>=|==|!=|&&|\|\||<<|>>|[-+*/%&|^]=|.', re.S)


def strip(code):
    code = re.sub(r'/\*.*?\*/', lambda m: re.sub(r'[^\n]', ' ', m.group(0)), code, flags=re.S)
    code = re.sub(r'//[^\n]*', '', code)
    code = re.sub(r'"(\\.|[^"\\])*"', '""', code)
    code = re.sub(r"'(\\.|[^'\\])*'", "''", code)
    return code


def find_body(code, fn):
    for m in re.finditer(r'^[^\n;{}#]*\b' + re.escape(fn) + r'\s*\(', code, re.M):
        i = m.end() - 1
        depth = 0
        while True:
            c = code[i]
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    break
            i += 1
        j = i + 1
        while code[j] in ' \t\r\n':
            j += 1
        if code[j] != '{':
            continue
        depth = 0
        k = j
        while True:
            c = code[k]
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0:
                    return m.start(), k + 1
            k += 1
    return None


def tokens(code, start, end):
    out = []
    line = code.count('\n', 0, start) + 1
    pos = start
    for m in TOK.finditer(code, start, end):
        line += code.count('\n', pos, m.start())
        pos = m.start()
        t = m.group(0)
        if t.isspace():
            continue
        out.append((t, line))
    return out


def is_binop_prev(toks, i):
    """Is toks[i] a binary arithmetic operator (not unary)?"""
    t = toks[i][0]
    if t not in PREC:
        return False
    if i == 0:
        return False
    p = toks[i - 1][0]
    return p in (')', ']') or re.match(r'[A-Za-z_0-9.]', p) is not None and p not in KEYWORDS


def classify(toks):
    stack, out = [], []
    for i, (t, ln) in enumerate(toks):
        if t == '(':
            stack.append(i)
        elif t == ')' and stack:
            o = stack.pop()
            prev = toks[o - 1][0] if o else ''
            if re.match(r'[A-Za-z_]\w*$', prev) and prev not in KEYWORDS - {'return', 'case'}:
                if prev not in ('return', 'case'):
                    continue            # call / macro arguments
            if prev in (')', ']'):
                continue                # call through pointer
            if prev in KEYWORDS - {'return', 'case'}:
                continue                # control statement
            inner = [x[0] for x in toks[o + 1:i]]
            if inner and (inner[0] in TYPEWORDS or (inner[-1] == '*' and all(re.match(r'\w+$|\*', x) for x in inner))):
                continue                # cast
            depth, top = 0, set()
            for k in range(o + 1, i):
                x = toks[k][0]
                if x in '([':
                    depth += 1
                elif x in ')]':
                    depth -= 1
                elif depth == 0 and x in PREC and is_binop_prev(toks, k):
                    top.add(x)
                elif depth == 0 and x in ('<', '>', '<=', '>=', '==', '!=', '&&', '||', '?', '=', '&', '|', '^',
                                          '<<', '>>', ','):
                    top.add(x)
            # outer context
            pop = toks[o - 1][0] if o else ''
            nop = toks[i + 1][0] if i + 1 < len(toks) else ''
            p_bin = pop in PREC and o - 1 >= 1 and is_binop_prev(toks, o - 1)
            n_bin = nop in PREC
            icls = {PREC[x] for x in top if x in PREC}
            other = top - set(PREC)
            if not top:
                kind = 'LEAF'
            elif other:
                kind = 'OTHER'
            elif n_bin and PREC[nop] == 'mul' and (not p_bin or PREC[pop] == 'add') and icls == {'add'}:
                kind = 'NEEDED'         # left operand of a tighter operator
            elif n_bin and PREC[nop] == 'mul' and (not p_bin or PREC[pop] == 'add') and icls == {'mul'}:
                kind = 'LEFTRED'
            elif p_bin and PREC[pop] in icls and len(icls) == 1:
                kind = 'REGROUP'
            elif p_bin and PREC[pop] == 'mul' and icls == {'add'}:
                kind = 'NEEDED'
            elif n_bin and PREC[nop] == 'mul' and icls == {'add'}:
                kind = 'NEEDED'
            elif n_bin and PREC[nop] in icls and len(icls) == 1:
                kind = 'LEFTRED'
            elif not p_bin and not n_bin:
                kind = 'WHOLE'
            else:
                kind = 'MIXED'
            text = ' '.join(x[0] for x in toks[max(0, o - 3):min(len(toks), i + 4)])
            out.append((ln, kind, ''.join(sorted(top)), text))
    return out


def main():
    src = sys.argv[1]
    code = strip(open(src, encoding='latin-1').read())
    for fn in sys.argv[2:]:
        span = find_body(code, fn)
        if not span:
            print('## %s: NOT FOUND' % fn)
            continue
        toks = tokens(code, span[0], span[1])
        rows = classify(toks)
        counts = {}
        for r in rows:
            counts[r[1]] = counts.get(r[1], 0) + 1
        print('## %s  lines %d..%d  %s' % (fn, code.count('\n', 0, span[0]) + 1, code.count('\n', 0, span[1]) + 1,
                                            ' '.join('%s=%d' % kv for kv in sorted(counts.items()))))
        for ln, kind, top, text in sorted(rows):
            if kind in ('OTHER',):
                continue
            print('  %5d %-8s %-4s %s' % (ln, kind, top, text))


if __name__ == '__main__':
    main()
