"""G1 read-only helper: map OUR function offsets to the source line(s) whose code they are, from a lst.py /FAsc
listing (the listing header line '; NNN : text' preceding each instruction group).

usage: python -B where.py <listing.txt> <unit> <fn> [--obj ours.obj]
Runs alndiff (ours vs January) and prints every differing block with the source line of its first OUR instruction.
"""
import re
import subprocess
import sys


def offmap(path):
    m = {}
    cur = ''
    for line in open(path, encoding='utf-8').read().splitlines():
        h = re.match(r';\s+(\d+)\s+:\s?(.*)$', line)
        if h:
            cur = 'L%s %s' % (h.group(1), h.group(2).strip()[:70])
            continue
        o = re.match(r'\s+([0-9a-f]{5})\s', line)
        if o:
            m[int(o.group(1), 16)] = cur
    return m


def main():
    lst, unit, fn = sys.argv[1], sys.argv[2], sys.argv[3]
    obj = sys.argv[sys.argv.index('--obj') + 1] if '--obj' in sys.argv else None
    m = offmap(lst)
    keys = sorted(m)
    cmd = ['python', '-B', 'tools/campaign/alndiff.py', unit, fn, '--max-lines', '0']
    if obj:
        cmd += ['--ours-object', obj]
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    for line in out.splitlines():
        b = re.match(r'--- (\w+) .*@T (\S+) @O (0x[0-9a-f]+|end)', line)
        if not b or b.group(3) == 'end':
            continue
        oo = int(b.group(3), 16)
        k = max([x for x in keys if x <= oo] or [0])
        print('%-8s T %-6s O 0x%-5x %s' % (b.group(1), b.group(2), oo, m.get(k, '?')))


if __name__ == '__main__':
    main()
