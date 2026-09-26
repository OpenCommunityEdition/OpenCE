"""A3: list match_vassert / match_vwarn calls whose message argument is a plain string literal (possible
hand-supplied expression text), for the given C files. Read-only.

    python -B research/compiler_application_20260925/workers/A3/tools/vassert_census.py file.c [...]
"""
import re
import sys

CALL = re.compile(r'match_v(?:assert|warn)\s*\(')


def split_args(s):
    depth = 0
    out, cur = [], ''
    i = 0
    in_str = False
    while i < len(s):
        c = s[i]
        if in_str:
            cur += c
            if c == '\\':
                cur += s[i + 1]
                i += 2
                continue
            if c == '"':
                in_str = False
        elif c == '"':
            in_str = True
            cur += c
        elif c in '([{':
            depth += 1
            cur += c
        elif c in ')]}':
            if depth == 0:
                out.append(cur)
                return out, i
            depth -= 1
            cur += c
        elif c == ',' and depth == 0:
            out.append(cur)
            cur = ''
        else:
            cur += c
        i += 1
    return out, i


for path in sys.argv[1:]:
    t = open(path, encoding='latin-1').read()
    for m in CALL.finditer(t):
        args, end = split_args(t[m.end():])
        if len(args) < 4:
            continue
        msg = ' '.join(args[3].split())
        line = t.count('\n', 0, m.start()) + 1
        if re.fullmatch(r'"(?:[^"\\]|\\.)*"', msg):
            print('%s:%d  expr=%s  msg=%s' % (path.split('/')[-1], line, ' '.join(args[2].split())[:90], msg[:90]))
