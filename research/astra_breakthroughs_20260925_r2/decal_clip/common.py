"""Pinned, isolated R2 observation harness. All generated files stay in R2 scratch."""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import re
import struct
import subprocess
import sys

sys.dont_write_bytecode = True

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PREVIOUS = ROOT / 'research/astra_breakthroughs_20260925/decal_clip'
OUT = ROOT / 'scratch/astra_clip_r2_20260925'
UNIT = 'source/effects/decals'
NAME = '_decal_clip_to_surface'
SOURCE_SHA = '0719c1308c49175fa5616e566f78c405f3536a8d60c20f8d5a6da31a006d9e9e'
DEPENDENCY_MANIFEST_SHA = '804e84e66af21b637b30c2124e38ed66e3b19e02993b7a493c52b98878fde297'
TARGET_SHA = '1778c1f9b5a08eb6dd00df321dbd4b6a6192d0207d067312eaa116084913abaf'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def verify():
    manifest_bytes=(PREVIOUS / 'dependencies/manifest.json').read_bytes()
    assert sha(manifest_bytes)==DEPENDENCY_MANIFEST_SHA, 'Dependency manifest drift'
    manifest = json.loads(manifest_bytes)
    for item in manifest['source_snapshots']:
        p = PREVIOUS / 'dependencies' / item['name']
        assert sha(p.read_bytes()) == item['snapshot_sha256'], str(p)
    for name, expected in manifest['compiler_sha256'].items():
        assert sha((ROOT / 'xbox/bin/vc7' / name).read_bytes()) == expected, name
    debugger = Path(os.environ.get('HALO_C2_DEBUGGER', manifest['debugger']['default_path']))
    assert sha(debugger.read_bytes()) == manifest['debugger']['sha256'], str(debugger)
    return manifest, debugger

MANIFEST, DEBUGGER = verify()
sys.path.insert(0, str(PREVIOUS / 'dependencies'))
spec = importlib.util.spec_from_file_location('clip_r2_trace', PREVIOUS / 'dependencies/trace.py')
T = importlib.util.module_from_spec(spec)
spec.loader.exec_module(T)
T.WT, T.CL, T.DEBUGGER = str(ROOT), str(ROOT / 'xbox/bin/vc7/CL.Exe'), str(DEBUGGER)
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc
import objfns
objfns.WT=str(ROOT)
sys.path=[p for p in sys.path if p != MANIFEST['origin_directory'].split('\\scratch')[0]]
assert Path(cc.__file__).resolve()==ROOT / 'tools/coff_compare.py'
assert Path(objfns.__file__).resolve()==PREVIOUS / 'dependencies/objfns.py'

def source():
    """Rebuild the LF-normalized S4 TU from tracked canonical source and R1 patch."""
    old = (ROOT / 'source/effects/decals.c').read_text().splitlines(keepends=True)
    patch = (PREVIOUS / 'patches/canonical_to_S4.patch').read_text().splitlines(keepends=True)
    result, cursor, i = [], 0, 0
    while i < len(patch):
        m = re.match(r'@@ -(\d+)(?:,\d+)? \+\d+(?:,\d+)? @@', patch[i])
        if not m:
            i += 1
            continue
        start = int(m[1]) - 1
        result.extend(old[cursor:start]); cursor = start; i += 1
        while i < len(patch) and not patch[i].startswith('@@'):
            line = patch[i]; i += 1
            if line.startswith((' ', '-')):
                assert old[cursor] == line[1:], (cursor, old[cursor], line)
                cursor += 1
            if line.startswith((' ', '+')):
                result.append(line[1:])
    result.extend(old[cursor:])
    data = ''.join(result).encode()
    assert sha(data) == SOURCE_SHA, sha(data)
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / 'baseline.c'
    path.write_bytes(data)
    return path

def equality(work):
    instrumented = (work / 'trace.obj').read_bytes()
    (work / 'instrumented.obj').write_bytes(instrumented)
    command = (work / 'dbg_cmd.u16').read_text(encoding='utf-16-le').rstrip('\0')
    cwd = (work / 'dbg_dir.u16').read_text(encoding='utf-16-le').rstrip('\0')
    before = {n: sha((ROOT / 'xbox/bin/vc7' / n).read_bytes()) for n in MANIFEST['compiler_sha256']}
    p = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    assert p.returncode == 0, p.stdout + p.stderr
    stock = (work / 'trace.obj').read_bytes()
    (work / 'stock.obj').write_bytes(stock)
    a, b = bytearray(instrumented), bytearray(stock)
    a[4:8] = b[4:8] = b'\0'*4
    after = {n: sha((ROOT / 'xbox/bin/vc7' / n).read_bytes()) for n in before}
    receipt = dict(command=command, cwd=cwd, source_sha256=SOURCE_SHA,
        instrumented_sha256=sha(instrumented), stock_sha256=sha(stock),
        instrumented_timestamp_masked_sha256=sha(a), stock_timestamp_masked_sha256=sha(b),
        raw_equal=instrumented == stock, equal_ignoring_only_coff_timestamp=a == b,
        instrumented_length=len(instrumented), stock_length=len(stock),
        raw_differing_offsets=[i for i,(x,y) in enumerate(zip(instrumented,stock)) if x != y],
        compiler_sha256=after, compiler_disk_hashes_unchanged=before == after,
        debugger_sha256=sha((work / 'dbg32c.exe').read_bytes()),
        target=cc.section_info(cc.load(stock), NAME))
    (work / 'equality.json').write_text(json.dumps(receipt, indent=2)+'\n')
    assert a == b and before == after
    assert receipt['target']['normalized_sha256']==TARGET_SHA
    assert receipt['target']['size']==1776
    return receipt

def lt_events(work):
    events, hits = [], {}
    for line in (work / 'dbg_result.txt').read_text().splitlines():
        if not line.startswith('LT '): continue
        words = line.split()
        bp=int(words[1][2:],16)
        hits[bp]=hits.get(bp,0)+1
        assert hits[bp] % 16 == int(words[2][1:],16), 'unexpected missing/duplicated LT record'
        event = dict(bp=bp, hit=hits[bp], printed_hit_low_nibble=int(words[2][1:],16),
            regs={k:int(v,16) for k,v in re.findall(r'(e\w+)=(0x[0-9a-f]+)',line)}, chains={})
        stack=re.search(r'stk:((?: 0x[0-9a-f]+)+)',line)
        event['stack']=[int(x,16) for x in stack[1].split()] if stack else []
        for match in re.finditer(r'chain(\d):((?: 0x[0-9a-f]+)+)',line):
            event['chains'][int(match[1])] = [int(x,16) for x in match[2].split()]
        events.append(event)
    return events

def watch_events(work):
    text = (work / 'dbg_result.txt').read_text()
    events=[]
    hits={}
    for m in re.finditer(r'WP(\d) #([0-9a-f]+) @(0x[0-9a-f]+) now=(0x[0-9a-f]+) writer_eip=(0x[0-9a-f]+)\n([\s\S]*?)(?=\n(?:LT|WP|DWP|GATE|done)|\Z)',text):
        tail=m[6]
        rec=re.search(r'rec@(0x[0-9a-f]+):((?: 0x[0-9a-f]+)+)',tail)
        stk=re.search(r'stk:((?: 0x[0-9a-f]+)+)',tail)
        wp=int(m[1]); hits[wp]=hits.get(wp,0)+1
        assert hits[wp] % 16 == int(m[2],16), 'unexpected missing/duplicated WP record'
        events.append(dict(wp=wp,hit=hits[wp],printed_hit_low_nibble=int(m[2],16),address=int(m[3],16),value=int(m[4],16),
            eip=int(m[5],16),regs={k:int(v,16) for k,v in re.findall(r'(e\w+)=(0x[0-9a-f]+)',tail)},
            base=int(rec[1],16) if rec else None,record=[int(x,16) for x in rec[2].split()] if rec else None,
            stack=[int(x,16) for x in stk[1].split()] if stk else None))
    return events

def disasm(va, count=40):
    import capstone
    data=(ROOT / 'xbox/bin/vc7/C2.Dll').read_bytes()
    pe=struct.unpack_from('<I',data,0x3c)[0]
    section=pe+24+struct.unpack_from('<H',data,pe+20)[0]
    offset=None
    for _ in range(struct.unpack_from('<H',data,pe+6)[0]):
        vsz,rva,rawsz,raw=struct.unpack_from('<4I',data,section+8)
        if 0x10700000+rva <= va < 0x10700000+rva+vsz:
            offset=raw+va-0x10700000-rva; break
        section+=40
    assert offset is not None
    md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    return [f'{x.address:#010x}: {x.mnemonic:8s} {x.op_str}' for x in list(md.disasm(data[offset:offset+count*10],va))[:count]]

def call_candidates(target):
    """Byte-scan E8 sites; callers must independently validate instruction boundaries."""
    data=(ROOT / 'xbox/bin/vc7/C2.Dll').read_bytes()
    pe=struct.unpack_from('<I',data,0x3c)[0]
    section=pe+24+struct.unpack_from('<H',data,pe+20)[0]
    sites=[]
    for _ in range(struct.unpack_from('<H',data,pe+6)[0]):
        vsz,rva,rawsz,raw=struct.unpack_from('<4I',data,section+8)
        for i in range(raw,raw+rawsz-5):
            va=0x10700000+rva+i-raw
            if data[i]==0xe8 and (va+5+struct.unpack_from('<i',data,i+1)[0]) & 0xffffffff == target:
                sites.append(va)
        section+=40
    return sites

if __name__ == '__main__':
    if len(sys.argv)>1: print('\n'.join(disasm(int(sys.argv[1],0),int(sys.argv[2]) if len(sys.argv)>2 else 40)))
    else: print(source())
