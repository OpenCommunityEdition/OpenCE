"""G1 read-only helper: compile a unit (or a scratch copy) with its real ninja flags plus /FAsc and print the listing
section of one function, i.e. OUR instructions interleaved with the source lines that produced them. Used to map
alndiff offsets to source statements. Asserts that the object is keyed-identical to a plain compile (/FAsc must not
change code).

usage: python -B lst.py <unit> <fn> [--source copy.c] [--out listing.txt]
"""
import os
import re
import subprocess
import sys

sys.path.insert(0, 'tools')
import coff_compare as cc  # noqa: E402


def flags(unit):
    bn = open('build.ninja').read()
    key = 'build\\base\\' + unit.replace('/', '\\').replace(' ', '$ ') + '.obj:'
    i = bn.index(key)
    j = bn.index('cflags = ', i)
    k = bn.index('\nbuild ', j)
    cf = bn[j + len('cflags = '):k].replace('$\n', ' ').replace('$\r\n', ' ')
    cf = re.sub(r'\s+', ' ', cf).strip()
    toks = re.findall(r'/I"[^"]+"|\S+', cf)
    return [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in toks]


def main():
    args = sys.argv[1:]
    unit, fn = args[0], args[1]
    src = unit + '.c'
    out = None
    if '--source' in args:
        src = args[args.index('--source') + 1]
    if '--out' in args:
        out = args[args.index('--out') + 1]
    code = open(src, encoding='latin-1').read()
    tmp = 'scratch/_g1lst_%d' % os.getpid()
    open(tmp + '.c', 'w', encoding='latin-1', newline='\n').write(code)
    cl = os.path.abspath(os.path.join('xbox', 'bin', 'vc7', 'CL.Exe'))
    base = [cl, '/nologo', '/c'] + flags(unit) + ['/I' + os.path.dirname(unit + '.c')]
    r1 = subprocess.run(base + ['/Fo' + tmp + '_a.obj', tmp + '.c'], capture_output=True, text=True)
    r2 = subprocess.run(base + ['/FAsc', '/Fa' + tmp + '.asm', '/Fo' + tmp + '_b.obj', tmp + '.c'], capture_output=True,
                        text=True)
    if r1.returncode or r2.returncode:
        print(r1.stdout[-2000:], r2.stdout[-2000:])
        sys.exit(1)
    a = cc.load(open(tmp + '_a.obj', 'rb').read())
    b = cc.load(open(tmp + '_b.obj', 'rb').read())
    same = cc.section_infos_equal(cc.section_info(a, fn), cc.section_info(b, fn))
    lst = open(tmp + '.asm', encoding='latin-1').read()
    m = re.search(r'^' + re.escape(fn) + r'\s+PROC NEAR.*?^' + re.escape(fn) + r'\s+ENDP', lst, re.S | re.M)
    text = ('; /FAsc listing of %s (object section identical to plain compile: %s)\n' % (fn, same)) + (m.group(0) if m else 'NOT FOUND')
    for f in (tmp + '.c', tmp + '_a.obj', tmp + '_b.obj', tmp + '.asm'):
        try:
            os.remove(f)
        except OSError:
            pass
    if out:
        open(out, 'w', encoding='utf-8', newline='\n').write(text + '\n')
        print('wrote', out, 'identical:', same)
    else:
        print(text)


if __name__ == '__main__':
    main()
