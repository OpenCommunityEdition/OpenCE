"""Compile a small probe .c with the campaign flags and disassemble one function.

usage: python scratch/probelab.py <probe.c> <_func> [--obj out.obj]
"""
import re
import subprocess
import sys

sys.path.insert(0, 'tools')
import coff_compare as cc
import capstone

CL = r'C:\halo-worktrees\claude-finish-hs-20260816\xbox\bin\vc7\CL.Exe'


def cflags():
    bn = open('build.ninja').read()
    key = 'build\\base\\source\\physics\\breakable_surfaces.obj:'
    i = bn.index(key)
    j = bn.index('cflags = ', i)
    k = bn.index('\nbuild ', j)
    cf = re.sub(r'\s+', ' ', bn[j + 9:k].replace('$\n', ' ')).strip()
    toks = re.findall(r'/I"[^"]+"|\S+', cf)
    return [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in toks]


def build(src, obj):
    cmd = [CL, '/nologo', '/c'] + cflags() + ['/Fo' + obj, src]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return r


def disas(obj, fn):
    o = cc.load(open(obj, 'rb').read())
    si = cc.section_info(o, fn)
    secs = o['sections']
    for s in o['symbols']:
        if s['name'] == fn and s['section'] > 0 and s['value'] == 0:
            data = cc._section_bytes(o, secs[s['section'] - 1])
            md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
            out = []
            for ins in md.disasm(bytes(data), 0):
                out.append('%4x: %-8s %s' % (ins.address, ins.mnemonic, ins.op_str))
            return si, out
    return si, []


if __name__ == '__main__':
    src = sys.argv[1]
    fn = sys.argv[2]
    obj = sys.argv[4] if len(sys.argv) > 4 else 'scratch/_probe.obj'
    r = build(src, obj)
    if r.returncode != 0:
        print('COMPILE FAILED')
        print(r.stdout[-3000:])
        sys.exit(1)
    si, lines = disas(obj, fn)
    print('size=%d relocs=%d' % (si['size'], si['relocation_count']))
    for l in lines:
        print(l)
