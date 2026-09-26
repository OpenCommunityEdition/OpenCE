"""Observe fixed scout controls with private debugger/config copies.

No debugger FORCE operations or compiler file changes. Every run is replayed
with stock CL and required to match the entire object except COFF timestamp.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
OUT = ROOT / 'scratch/astra_scout_r2_20260925'
PRIOR = ROOT / 'research/astra_breakthroughs_20260925/decal_clip/dependencies'
SRC = ROOT / 'scratch/astra_scout_20260925'
UNIT = 'source/units/vehicles'
FN = '_update_alien_scout_physics'
OUT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(PRIOR))
from tools import coff_compare as cc
import trace_window as T

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def apply_unified(original, patch):
    """Strict in-memory unified patch; never writes the production file."""
    source=original.splitlines(keepends=True)
    result=[]
    cursor=0
    lines=patch.splitlines(keepends=True)
    i=0
    while i<len(lines):
        match=re.match(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@',lines[i])
        if not match:
            i+=1
            continue
        start=int(match[1])-1
        assert start>=cursor
        result.extend(source[cursor:start])
        cursor=start
        i+=1
        while i<len(lines) and not lines[i].startswith('@@ '):
            line=lines[i]
            if line.startswith((' ','-')):
                assert source[cursor]==line[1:],(cursor,line,source[cursor])
                cursor+=1
            if line.startswith((' ','+')): result.append(line[1:])
            i+=1
    result.extend(source[cursor:])
    return ''.join(result)

def setup(prepare_sources=True):
    assert sha(PRIOR/'manifest.json')=='804e84e66af21b637b30c2124e38ed66e3b19e02993b7a493c52b98878fde297'
    manifest = json.loads((PRIOR / 'manifest.json').read_text())
    for item in manifest['source_snapshots']:
        assert sha(PRIOR/item['name']) == item['snapshot_sha256']
    debugger = Path(manifest['debugger']['default_path'])
    assert sha(debugger) == manifest['debugger']['sha256']
    for name, digest in manifest['compiler_sha256'].items():
        assert sha(ROOT/'xbox/bin/vc7'/name) == digest
    seed = OUT/'dbg32c_seed.exe'
    shutil.copyfile(debugger,seed)
    T.WT = str(ROOT)
    T.CL = str(ROOT/'xbox/bin/vc7/CL.Exe')
    T.DEBUGGER = str(seed)
    if not prepare_sources:
        return
    sources = {'scalar_left':'held_D_forward_aggregate_only', 'aggregate_left':'held_aggregate_control'}
    expected = json.loads((ROOT/'research/astra_breakthroughs_20260925/scout/measurements.json').read_text())['audit']
    for name, origin in sources.items():
        digest = next(x['source_sha256'] for x in expected if x['name']==origin)
        original=(ROOT/'source/units/vehicles.c').read_text(encoding='latin1')
        patch=(ROOT/'research/astra_breakthroughs_20260925/scout'/(origin+'.patch')).read_text(encoding='latin1')
        (OUT/(name+'.c')).write_text(apply_unified(original,patch),encoding='latin1',newline='\n')
        assert sha(OUT/(name+'.c')) == digest
    (HERE/'input_manifest.json').write_text(json.dumps(dict(
        head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
        compiler=manifest['compiler_sha256'],debugger=manifest['debugger'],
        sources={n:dict(patch='research/astra_breakthroughs_20260925/scout/'+o+'.patch',sha256=sha(OUT/(n+'.c'))) for n,o in sources.items()},
        helpers={x['name']:x['snapshot_sha256'] for x in manifest['source_snapshots']}),indent=2))

def cstring(words):
    if not words: return None
    data=b''.join(x.to_bytes(4,'little') for x in words)
    return data.split(b'\0')[0].decode('latin1')

def stock_check(work):
    path=work/'trace.obj'
    observed=path.read_bytes()
    (work/'observed.obj').write_bytes(observed)
    command=(work/'dbg_cmd.u16').read_text(encoding='utf-16-le').rstrip('\0')
    cwd=(work/'dbg_dir.u16').read_text(encoding='utf-16-le').rstrip('\0')
    run=subprocess.run(command,cwd=cwd,capture_output=True,text=True,timeout=30)
    assert run.returncode==0,run.stdout+run.stderr
    stock=path.read_bytes()
    (work/'stock.obj').write_bytes(stock)
    a,b=bytearray(observed),bytearray(stock)
    a[4:8]=b[4:8]=b'\0'*4
    assert a==b,'Instrumentation changed full object beyond COFF timestamp'
    report=dict(command=command,cwd=cwd,raw_equal=observed==stock,
        full_object_timestamp_only=True,observed_sha256=hashlib.sha256(observed).hexdigest(),
        stock_sha256=hashlib.sha256(stock).hexdigest(),timestamp_masked_sha256=hashlib.sha256(a).hexdigest(),
        differing_offsets=[i for i,(x,y) in enumerate(zip(observed,stock)) if x!=y])
    (work/'stock_check.json').write_text(json.dumps(report,indent=2))
    return report

def run(label):
    work=OUT/(label+'_frame')
    work.mkdir(exist_ok=True)
    T.CHAINS='5 24;5 24 0;0 24;3 24'
    events,obj=T.run_gated(str(OUT/(label+'.c')),str(work),UNIT,FN,
        raw_bps=[0x10814497,0x10814408],extra_cflags=['/FAsc','/Fa'+str(work/'listing.asm')])
    rows=[]
    for _,bp,regs,record,chains in events:
        if bp==0:
            frame=chains.get(0)
            desc=chains.get(1)
            rows.append(dict(name=cstring(chains.get(2)),frame_ptr=regs['edi'],frame=frame,descriptor=desc,
                offset=(frame[9]-(1<<32) if frame[9]&(1<<31) else frame[9]) if frame else None))
    check=stock_check(work)
    result=dict(label=label,rows=rows,events=events,stock_check=check)
    (work/'frames.json').write_text(json.dumps(result,indent=2))
    print(label,'frame records',len(rows),'stock full object identical except stamp',check['full_object_timestamp_only'])
    for row in rows:
        if row['name'] and any(x in row['name'] for x in ('vehicle_left','desired_velocity','vehicle_horizontal','applied_alpha')):
            print(row)
    return result

if __name__=='__main__':
    setup()
    for label in sys.argv[1:] or ['scalar_left','aggregate_left']:
        run(label)
