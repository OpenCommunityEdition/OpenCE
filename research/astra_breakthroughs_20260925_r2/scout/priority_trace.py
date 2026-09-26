"""Observe size/use-count ordering, with unmodified compiler execution."""
import json
import sys
import frame_trace as F

def run(label,source=None,fn=None):
    source=source or str(F.OUT/(label+'.c'))
    fn=fn or F.FN
    work=F.OUT/(label+'_priority')
    work.mkdir(exist_ok=True)
    F.T.CHAINS='3 24;3 24 0 4;1 24;1 24 0 4'
    events,obj=F.T.run_gated(source,str(work),F.UNIT,fn,
        raw_bps=[0x107595c5,0x1075954a],extra_cflags=['/FAsc','/Fa'+str(work/'listing.asm')])
    rows=[]
    for _,bp,regs,record,chains in events:
        root=0 if bp==0 else 2
        rows.append(dict(kind='add' if bp==0 else 'rank',name=F.cstring(chains.get(root+1)),
            ptr=regs['edx'] if bp==0 else regs['ebx'],frame=chains.get(root),
            increment=regs['ebx'] if bp==0 else None,rank=regs['ebp'] if bp==1 else None))
    check=F.stock_check(work)
    result=dict(label=label,rows=rows,stock_check=check)
    (work/'priority.json').write_text(json.dumps(result,indent=2))
    for row in rows:
        if row['kind']=='rank' or row['name']=='_vehicle_left': print(row)
    return result

if __name__=='__main__':
    F.setup()
    for label in sys.argv[1:] or ['scalar_left','aggregate_left']: run(label)
