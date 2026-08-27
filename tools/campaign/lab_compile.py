"""Compile a repro TU with units.c's flags and diff a function vs the January split.

usage: python tools/campaign/lab_compile.py <repro.c> <fn-symbol> [--full]
Prints size + aligned-diff-line count vs build/split/source/units/units.obj;
--full dumps the differing lines.
"""
import re, subprocess, sys

sys.path.insert(0, 'tools')
import coff_compare as cc
import capstone

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

src, sym = sys.argv[1], sys.argv[2]
full = '--full' in sys.argv

bn = re.sub(r'\s*\$\r?\n\s*', ' ', open('build.ninja').read())
key = 'build build' + chr(92) + 'base' + chr(92) + 'source' + chr(92) + 'units' + chr(92) + 'units.obj:'
i = bn.index(key)
j = bn.index('cflags = ', i)
k = bn.index('\nbuild ', j)
cf = re.sub(r'\s+', ' ', bn[j + 9:k]).strip()
toks = [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in re.findall(r'/I"[^"]+"|\S+', cf)]
CL = 'C:' + chr(92) + chr(92).join(['halo-worktrees', 'claude-untried-objects', 'xbox', 'bin', 'vc7', 'CL.Exe'])

obj = src.rsplit('.', 1)[0] + '.obj'
r = subprocess.run([CL, '/nologo', '/c'] + toks + ['/Isource/units', '/Fo' + obj, src], capture_output=True, text=True)
if r.returncode:
    print('COMPILE FAILED')
    print(r.stdout[-3000:])
    sys.exit(1)


def dis(path, name):
    o = cc.load(open(path, 'rb').read())
    secs = o['sections']
    for smb in o['symbols']:
        if smb['name'] == name and smb['section'] > 0 and smb['value'] == 0 and secs[smb['section'] - 1]['name'] == '.text':
            data = cc._section_bytes(o, secs[smb['section'] - 1])
            return [(insn.address, insn.mnemonic + ' ' + insn.op_str) for insn in md.disasm(bytes(data), 0)], secs[smb['section'] - 1]['size']
    return None, None


t, tsize = dis('build/split/source/units/units.obj', sym)
o, osize = dis(obj, sym)
if o is None:
    print('symbol %s not found in %s' % (sym, obj))
    sys.exit(1)
import itertools
diffs = []
for (a, b) in itertools.zip_longest(t, o, fillvalue=(0, '')):
    if a[1] != b[1]:
        diffs.append('%4x: %-44s | %4x: %s' % (a[0], a[1], b[0], b[1]))
print('%s: target %d, lab %d bytes, %d diff lines' % (sym, tsize, osize, len(diffs)))
if full:
    print('\n'.join(diffs[:60]))
