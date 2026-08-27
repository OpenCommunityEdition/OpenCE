"""Gate harness: compile a unit (optionally with find/replace edits) and report per-function status.

usage: python scratch/gate.py <unit-path-no-ext> [--edits edits.json] [--fn NAME ...] [--disas NAME]
Example: python scratch/gate.py source/hs/hs --fn _hs_find_script_by_name --disas _hs_find_script_by_name
"""
import sys, json, os, re, subprocess, argparse
sys.path.insert(0, 'tools')
import coff_compare as cc

ap = argparse.ArgumentParser()
ap.add_argument('unit')
ap.add_argument('--edits')
ap.add_argument('--fn', action='append', default=[])
ap.add_argument('--disas')
ap.add_argument('--all', action='store_true')
a = ap.parse_args()

unit = a.unit.replace('\\', '/')
src = unit + '.c'
bn = open('build.ninja').read()
# the "build <obj>: cl <src>" line may be wrapped with a $ continuation, so match
# on the object path alone rather than requiring the leading "build " on that line
key = 'build\\base\\' + unit.replace('/', '\\') + '.obj:'
i = bn.index(key)
j = bn.index('cflags = ', i)
k = bn.index('\nbuild ', j)
cf = bn[j + len('cflags = '):k]
cf = cf.replace('$\n', ' ').replace('$\r\n', ' ')
cf = re.sub(r'\s+', ' ', cf).strip()
toks = re.findall(r'/I"[^"]+"|\S+', cf)
toks = [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in toks]

code = open(src, encoding='latin-1').read()
if a.edits:
    for entry in json.load(open(a.edits)):
        find, repl = entry[0], entry[1]
        assert find in code, 'EDIT NOT FOUND: %r' % find[:80]
        code = code.replace(find, repl)
scratch_src = f'scratch/_gate_{os.getpid()}.c'
open(scratch_src, 'w', encoding='latin-1', newline='\n').write(code)
obj = f'scratch/_gate_{os.getpid()}.obj'
if os.path.exists(obj):
    os.remove(obj)
cl = r'C:\halo-worktrees\claude-finish-hs-20260816\xbox\bin\vc7\CL.Exe'
incdir = os.path.dirname(src)
cmd = [cl, '/nologo', '/c'] + toks + ['/I' + incdir, '/Fo' + obj, scratch_src]
r = subprocess.run(cmd, capture_output=True, text=True)
if not os.path.exists(obj) or r.returncode != 0:
    print('COMPILE FAILED')
    print(r.stdout[-4000:])
    print(r.stderr[-2000:])
    sys.exit(1)

target = cc.load(open('build/split/' + unit + '.obj', 'rb').read())
ours = cc.load(open(obj, 'rb').read())


def fn_syms(o):
    secs = o['sections']
    out = {}
    for s in o['symbols']:
        if (s['name'].startswith('_') and s['section'] > 0 and s['storage'] in (2, 3)
                and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text'):
            out.setdefault(s['name'], s)
    return out


tsyms = fn_syms(target)
osyms = fn_syms(ours)
want = a.fn if a.fn else sorted(tsyms)
nexact = nres = nunw = 0
for name in want:
    if name not in tsyms:
        print('NOT-IN-TARGET', name)
        continue
    ti = cc.section_info(target, name)
    if name not in osyms:
        nunw += 1
        if a.fn or a.all:
            print(f'UNWRITTEN {ti["size"]:5d}  {name}')
        continue
    oi = cc.section_info(ours, name)
    if cc.section_infos_equal(ti, oi):
        nexact += 1
        if a.fn:
            print(f'EXACT     {ti["size"]:5d}  {name}')
    else:
        nres += 1
        why = []
        if ti['size'] != oi['size']:
            why.append(f'size {oi["size"]}!={ti["size"]}')
        if ti['relocation_count'] != oi['relocation_count']:
            why.append(f'relocs {oi["relocation_count"]}!={ti["relocation_count"]}')
        if ti['normalized_sha256'] != oi['normalized_sha256']:
            why.append('sha')
        if not why:
            # size, relocation count and normalized bytes all match, so the only
            # thing left is relocation IDENTITY - usually a static that carries no
            # public symbol in the image, which csplit names as
            # "<nearest public symbol> + N". See tools/campaign/attest_scan.py.
            why.append('reloc-identity')
        print(f'residual  {ti["size"]:5d}  {name}  [{", ".join(why)}]')
print(f'== exact {nexact}  residual {nres}  unwritten {nunw}  (of {len(want)} listed)')

if a.disas:
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

    def dis(o, name):
        secs = o['sections']
        for s in o['symbols']:
            if s['name'] == name and s['section'] > 0 and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text':
                sec = secs[s['section'] - 1]
                data = cc._section_bytes(o, sec)
                rels = {}
                si = cc.section_info(o, name)
                for rl in si.get('relocations', []):
                    rels[rl['address']] = str(rl['target'][1]) if len(rl['target']) > 1 else '?'
                lines = []
                for insn in md.disasm(bytes(data), 0):
                    ann = ''
                    for off, symn in rels.items():
                        if isinstance(off, int) and insn.address <= off < insn.address + insn.size:
                            ann += f'   ; {symn}'
                    lines.append((f'{insn.address:4x}: {insn.mnemonic} {insn.op_str}', ann))
                return lines
        return [('<not found>', '')]

    lt = dis(target, a.disas)
    lo = dis(ours, a.disas)
    import itertools
    print(f'--- target ({len(lt)}) | ours ({len(lo)}) : {a.disas}')
    for x, y in itertools.zip_longest(lt, lo, fillvalue=('', '')):
        xa, xann = x
        ya, yann = y
        mark = ' ' if xa.split(':', 1)[-1] == ya.split(':', 1)[-1] else '|'
        print(f'{(xa + xann):60s}{mark} {ya}{yann}')
