"""G1 census helper (read-only): summarise tools/campaign/alndiff.py for many functions.

usage: python -B alnsum.py <unit> <fn> [<fn> ...]  [--obj ours.obj]
Prints per function: J/O instruction counts, differing blocks, blocks touching x87 ('f*' mnemonics) on either side,
and the first x87 block offsets.
"""
import re
import subprocess
import sys


def summarise(unit, fn, obj=None):
    cmd = ['python', '-B', 'tools/campaign/alndiff.py', unit, fn, '--max-lines', '0']
    if obj:
        cmd += ['--ours-object', obj]
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    head = out.splitlines()[0] if out else ''
    blocks, cur = [], None
    for line in out.splitlines()[1:]:
        if line.startswith('---'):
            cur = {'hdr': line, 'rows': []}
            blocks.append(cur)
        elif cur is not None and re.match(r'\s+[TO]\s', line):
            cur['rows'].append(line)
    def norm(r):
        r = re.sub(r'defined-noncode:\.[a-z]+:', 'symbol:', r)
        r = re.sub(r'rel\+0x[0-9a-f]+', 'rel', r)
        parts = r.split(None, 2)
        return parts[2] if len(parts) > 2 else ''
    real = []
    for b in blocks:
        t = [norm(r) for r in b['rows'] if r.split()[0] == 'T']
        o = [norm(r) for r in b['rows'] if r.split()[0] == 'O']
        if t != o:
            real.append(b)
    blocks = real
    x87 = []
    for b in blocks:
        mn = [r.split()[2] for r in b['rows'] if len(r.split()) > 2]
        if any(m.startswith('f') for m in mn):
            m = re.search(r'@T (0x[0-9a-f]+)', b['hdr'])
            x87.append(m.group(1) if m else '?')
    return head, len(blocks), x87


def main():
    args = sys.argv[1:]
    obj = None
    if '--obj' in args:
        i = args.index('--obj')
        obj = args[i + 1]
        del args[i:i + 2]
    unit, fns = args[0], args[1:]
    for fn in fns:
        head, nb, x87 = summarise(unit, fn, obj)
        print('%-48s %-40s blocks %4d  x87-blocks %4d  first: %s' % (fn, head.replace('instructions', 'ins'), nb,
                                                                       len(x87), ' '.join(x87[:8])))


if __name__ == '__main__':
    main()
