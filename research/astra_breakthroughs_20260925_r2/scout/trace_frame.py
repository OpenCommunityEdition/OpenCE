"""Generic observation-only VC7 frame count/rank/packing CLI.

No force or compiler-flag overrides are exposed. Output must stay under scratch.
Every trace is replayed with the identical stock command; only timestamp bytes
4..7 may differ in the complete object. This is measurement, not source credit.
"""
import argparse
import json
from pathlib import Path
import frame_trace as F

SITES=[0x107595c5,0x1075954a,0x1076b13b,0x1076b57e,0x1076b43a,0x10814497]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',required=True,type=Path,help='Existing C source; read only')
    parser.add_argument('--unit',required=True,help='Existing build.ninja unit, e.g. source/units/vehicles')
    parser.add_argument('--function',required=True,help='COFF function symbol, including leading underscore')
    parser.add_argument('--output',required=True,type=Path,help='Private output directory under repository scratch/')
    args=parser.parse_args()
    source=args.source.resolve()
    output=args.output.resolve()
    if not output.is_relative_to((F.ROOT/'scratch').resolve()):
        parser.error('--output must remain under repository scratch/')
    if not source.is_file(): parser.error('--source must be an existing file')
    if source.is_relative_to(output):
        parser.error('--source must not be inside --output')
    output.mkdir(parents=True,exist_ok=True)
    lockpath=output/'.frame_trace.lock'
    try:
        lock=lockpath.open('x')
    except FileExistsError:
        parser.error('--output is locked; use a unique private directory for concurrent runs')
    try:
        lock.write('Observation in progress; do not share this output directory.\n')
        lock.flush()
        run(args,source,output)
    finally:
        lock.close()
        lockpath.unlink()

def run(args,source,output):
    before=F.sha(source)
    F.setup(prepare_sources=False)
    compiler_before={name:F.sha(F.ROOT/'xbox/bin/vc7'/name) for name in ['CL.Exe','C1.Dll','C2.Dll']}
    output.mkdir(parents=True,exist_ok=True)
    F.T.CHAINS='3 24;1 24;5 24;5 24 0 4'
    events,obj=F.T.run_gated(str(source),str(output),args.unit,args.function,
        raw_bps=SITES,extra_cflags=['/FAsc','/Fa'+str(output/'listing.asm')])
    counts={}
    ranks=[]
    packing=[]
    finals={}
    for _,bp,regs,record,chains in events:
        if bp==0:
            ptr=regs['edx']
            counts[ptr]=counts.get(ptr,0)+regs['ebx']
        elif bp==1:
            frame=chains[1]
            ranks.append(dict(ptr=regs['ebx'],rank=regs['ebp'],size=frame[8],flags=hex(frame[1]),
                count=frame[14],internal_id=frame[7]))
        elif bp in [2,3,4]:
            packing.append(dict(kind={2:'fit',3:'expand',4:'new'}[bp],rank=regs['edi'],
                group=regs[{2:'ebx',3:'esi',4:'ebp'}[bp]]))
        else:
            frame=chains[2]
            value=frame[9]
            assert regs['edi'] not in finals,'Duplicate final frame identity; gate may be wrong'
            finals[regs['edi']]=dict(name=F.cstring(chains.get(3)),flags=hex(frame[1]),
                offset=value-(1<<32) if value&(1<<31) else value)
    assert [row['rank'] for row in ranks]==list(range(len(ranks))),'Noncontiguous/duplicate frame ranks; gate may be wrong'
    assert len({row['ptr'] for row in ranks})==len(ranks),'Duplicate ranked frame identity'
    for row in ranks:
        row['observed_addition_sum']=counts.get(row['ptr'],0)
        assert row['observed_addition_sum']==row['count'],'Reference addition/count disagreement; do not interpret this trace'
        row['final']=finals.get(row['ptr'])
    byrank={row['rank']:row for row in ranks}
    for step in packing:
        final=byrank[step['rank']]['final']
        step['name']=final['name'] if final else None
    receipt=F.stock_check(output)
    compiler_after={name:F.sha(F.ROOT/'xbox/bin/vc7'/name) for name in compiler_before}
    assert compiler_before==compiler_after,'Compiler disk content changed'
    assert F.sha(source)==before,'Input source changed during observation'
    section=F.cc.section_info(F.cc.load(output/'stock.obj'),args.function)
    report=dict(source=str(source),source_sha256=before,unit=args.unit,function=args.function,
        sites=[hex(x) for x in SITES],scope='observation only; no force; no source credit',
        ordering=ranks,packing=packing,section=section,stock_check=receipt,
        compiler_sha256=compiler_after,compiler_disk_hashes_unchanged=True,
        debugger_sha256=F.sha(output/'dbg32c.exe'),raw_log_sha256=F.sha(output/'dbg_result.txt'))
    (output/'frame_report.json').write_text(json.dumps(report,indent=2))
    print('Complete-object stock equality except timestamp:',receipt['full_object_timestamp_only'])
    print('Frame records:',len(ranks),'packing decisions:',len(packing))
    print('Report:',output/'frame_report.json')

if __name__=='__main__': main()
