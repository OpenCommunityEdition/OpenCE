"""Observe first-fit/expansion choices and four targeted interference sets."""
import json
import sys
import frame_trace as F

def run(label):
    work=F.OUT/(label+'_packing')
    work.mkdir(exist_ok=True)
    F.T.CHAINS='2 12 5c 0;2 12 84 0;2 12 88 0;2 12 90 0'
    events,obj=F.T.run_gated(str(F.OUT/(label+'.c')),str(work),F.UNIT,F.FN,
        raw_bps=[0x1076b56a,0x1076b13b,0x1076b57e,0x1076b43a],
        extra_cflags=['/FAsc','/Fa'+str(work/'listing.asm')])
    graph=None
    rows=[]
    for _,bp,regs,record,chains in events:
        if bp==0:
            if graph is None: graph=chains
            continue
        rows.append(dict(kind=['graph','fit','expand','new'][bp],rank=regs['edi'],
            group=regs[['edx','ebx','esi','ebp'][bp]]))
    check=F.stock_check(work)
    result=dict(label=label,rows=rows,graph=graph,stock_check=check)
    tailwork=F.OUT/(label+'_graph_tail')
    tailwork.mkdir(exist_ok=True)
    F.T.CHAINS='2 3 5c 0 4;2 3 84 0 4;2 3 88 0 4;2 3 90 0 4'
    tail_events,tail_obj=F.T.run_gated(str(F.OUT/(label+'.c')),str(tailwork),F.UNIT,F.FN,
        raw_bps=[0x1076b56a],extra_cflags=['/FAsc','/Fa'+str(tailwork/'listing.asm')])
    result['graph_tail']=tail_events[0][4]
    result['tail_stock_check']=F.stock_check(tailwork)
    (work/'packing.json').write_text(json.dumps(result,indent=2))
    print(label,rows,'graph',graph)

if __name__=='__main__':
    F.setup()
    for label in sys.argv[1:] or ['scalar_left','aggregate_left']: run(label)
