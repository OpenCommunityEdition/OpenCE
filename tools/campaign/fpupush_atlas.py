"""Compile scratch/fpupush.c with hs.c's flags and report, per variant, whether
VC7 passed the float argument through the FPU (fld ... push ... fstp [esp]) or as
an integer copy (mov ... push).
"""
import re, subprocess, sys, os
sys.path.insert(0, 'tools')
import coff_compare as cc
import capstone

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

bn = re.sub(r'\s*\$\r?\n\s*', ' ', open('build.ninja').read())
key = 'source' + chr(92) + 'hs' + chr(92) + 'hs.obj:'
i = bn.index(key)
j = bn.index('cflags = ', i)
k = bn.index('build ', j)
cf = re.sub(r'\s+', ' ', bn[j + 9:k]).strip()
toks = [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in re.findall(r'/I"[^"]+"|\S+', cf)]
CL = r'C:\halo-worktrees\claude-finish-hs-20260816\xbox\bin\vc7\CL.Exe'

src = 'scratch/fpupush.c'
obj = 'scratch/fpupush.obj'
if os.path.exists(obj):
    os.remove(obj)
r = subprocess.run([CL, '/nologo', '/c'] + toks + ['/Fo' + obj, src], capture_output=True, text=True)
if not os.path.exists(obj):
    print('COMPILE FAILED')
    print(r.stdout[-3000:])
    sys.exit(1)

o = cc.load(open(obj, 'rb').read())
secs = o['sections']
names = []
for s in o['symbols']:
    n = s['name']
    if n.startswith('_v') and s['section'] > 0 and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text':
        if n not in names:
            names.append(n)

for n in sorted(names):
    si = cc.section_info(o, n)
    sec = secs[[s['name'] for s in o['symbols'] if s['name'] == n][0] and 0]
    data = None
    for s in o['symbols']:
        if s['name'] == n and s['section'] > 0:
            data = cc._section_bytes(o, secs[s['section'] - 1])
            break
    text = [f'{ins.mnemonic} {ins.op_str}' for ins in md.disasm(bytes(data), 0)]
    body = ' | '.join(text)
    fpu = 'fstp dword ptr [esp]' in body
    fpu_q = 'fstp qword ptr [esp]' in body
    verdict = 'FPU-PUSH dword  <<<' if fpu else ('fpu-qword(promoted)' if fpu_q else 'integer')
    print(f'{n:8s} {si["size"]:4d}B  {verdict}')
    if fpu:
        seq = [t for t in text if t.startswith(('fld', 'fstp', 'push', 'mov e', 'sub esp'))][:8]
        print(f'         {" ; ".join(seq)}')
