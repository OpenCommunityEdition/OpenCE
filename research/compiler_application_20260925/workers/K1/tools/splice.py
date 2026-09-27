"""Build a TU copy: take BASE file, replace the ai_communication_event body with the one from DONOR (or an
event-body file), optionally apply literal (old,new) replacement pairs from a python-literal list file.
usage: splice.py BASE OUT [--event DONOR_C] [--pairs PAIRS_PY ...]
Line endings: output uses BASE's CRLF convention."""
import sys, ast
args = sys.argv[1:]
base, out = args[0], args[1]
rest = args[2:]
t = open(base, newline='').read()
crlf = '\r\n' in t
t = t.replace('\r\n', '\n')
def ev_span(s):
    i = s.index('void ai_communication_event(')
    j = s.index('/* ---------- private code */', i)
    return i, j
k = 0
while k < len(rest):
    if rest[k] == '--event':
        d = open(rest[k + 1], newline='').read().replace('\r\n', '\n')
        i, j = ev_span(t); di, dj = ev_span(d)
        t = t[:i] + d[di:dj] + t[j:]
        k += 2
    elif rest[k] == '--pairs':
        pairs = ast.literal_eval(open(rest[k + 1]).read())
        for old, new in pairs:
            n = t.count(old)
            if n != 1:
                sys.exit('pair not unique (%d): %r' % (n, old[:80]))
            t = t.replace(old, new)
        k += 2
    else:
        sys.exit('bad arg ' + rest[k])
if crlf:
    t = t.replace('\n', '\r\n')
open(out, 'w', newline='').write(t)
print('wrote', out)
