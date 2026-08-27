"""Isolation-repro generator for units.c functions (the 197e30 method).

usage: python tools/campaign/mk_repro.py <out.c> <fn> [<fn2> ...] [--caller "C code..."]

Emits a minimal TU: units.c's whole prelude (includes/constants/macros/
structures/prototypes/globals, i.e. everything above the public-code marker),
with `static` DEGRADED to extern on prototypes of functions NOT included (so
referenced-but-undefined statics compile), followed by the requested function
definitions in original source order, then an optional scratch caller.

Statics whose definitions ARE included keep their staticness, so their custom
calling conventions survive; a scratch caller (or an included in-repro caller)
gives VC7 the call sites it needs to pick the convention.

Compile with tools/campaign/lab_compile.py and compare against the January
split section.
"""
import re, sys

args = sys.argv[1:]
caller = None
if '--caller' in args:
    i = args.index('--caller')
    caller = args[i + 1]
    args = args[:i] + args[i + 2:]
out, names = args[0], args[1:]

s = open('source/units/units.c', encoding='latin-1').read().replace('\r\n', '\n')
prelude = s[:s.index('/* ---------- public code */')]


def fn_def(name):
    m = re.search(r'\n((?:static )?[a-z_][a-z_ *]*\n?' + re.escape(name) + r'\(\n(?:[^)]|\n)*?\)\n\{)', s)
    assert m, name
    start = s.rfind('\n', 0, m.start(1)) + 1
    i = s.index('{', m.start(1))
    depth = 0
    while i < len(s):
        if s[i] == '{':
            depth += 1
        elif s[i] == '}':
            depth -= 1
            if depth == 0:
                return s.find('\n', 0, start), s[start:i + 1]
        i += 1


defs = sorted((fn_def(n) for n in names), key=lambda t: t[0])

# degrade static on prototypes of non-included functions
def degrade(m):
    proto = m.group(1)
    fname = re.search(r'([a-z_][a-z_0-9]*)\($', proto.split('(')[0] + '(')
    nm = proto.split('(')[0].split()[-1].split('\n')[-1]
    return '\n' + proto if nm in names else '\n' + proto

lines = prelude.split('\n')
out_lines = []
i = 0
while i < len(lines):
    ln = lines[i]
    m = re.match(r'static ([a-z_][a-z_ *]*)$', ln)
    m2 = re.match(r'static ([a-z_][a-z_ *]*[a-z_]) ?([a-z_0-9]+)\(', ln)
    if m2 and m2.group(2) not in names:
        out_lines.append(ln[len('static '):])
    elif m and i + 1 < len(lines) and re.match(r'[a-z_0-9]+\(', lines[i + 1]) and lines[i + 1].split('(')[0] not in names:
        out_lines.append(ln[len('static '):])
    else:
        out_lines.append(ln)
    i += 1
prelude = '\n'.join(out_lines)

body = prelude + '\n/* ---------- repro code */\n\n'
body += '\n\n'.join(d for _, d in defs)
if caller:
    body += '\n\n' + caller
body += '\n'
open(out, 'w', encoding='latin-1', newline='\n').write(body)
print('wrote', out, len(body), 'bytes;', len(defs), 'functions')
