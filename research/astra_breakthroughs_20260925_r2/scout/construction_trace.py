"""Join freshly created symbol/frame records to final named stack records."""
import json
import sys
import frame_trace as F

def run(label):
    work=F.OUT/(label+'_construction')
    work.mkdir(exist_ok=True)
    F.T.CHAINS='4 24;5 24;5 24 4;0 24'
    source=str(F.OUT/(label+'.c'))
    k,n=F.T.locate(source,F.UNIT,F.FN,str(work))
    events,obj=F.T.run(source,str(work),F.UNIT,gate=(k-1,k),
        raw_bps=[0x10715dc7,0x10814497],extra_cflags=['/FAsc','/Fa'+str(work/'listing.asm')])
    created=[]
    final=[]
    for event in events:
        if event[0]!='RAW': continue
        _,bp,regs,record,chains=event
        if bp==0:
            created.append(dict(hit=len(created)+1,ptr=regs['esi'],frame=chains.get(0),
                descriptor=chains.get(1),name=F.cstring(chains.get(2))))
        else:
            final.append(dict(ptr=regs['edi'],name=F.cstring(chains.get(3)),
                frame=chains.get(1),descriptor=chains.get(0)))
    named=[]
    for row in created:
        matches=[f for f in final if f['ptr']==row['ptr'] and f['descriptor'][10]==row['descriptor'][10]]
        if matches:
            row['final']=matches
            named.append(row)
    check=F.stock_check(work)
    result=dict(label=label,created=created,final=final,joined=named,stock_check=check)
    (work/'construction.json').write_text(json.dumps(result,indent=2))
    print(label,'created',len(created),'final',len(final),'joined',len(named),'stock',check['full_object_timestamp_only'])
    for row in named:
        if any(x in (row['name'] or '') for x in ('vehicle_left','desired_velocity','vehicle_horizontal','applied_alpha')):
            print(row)

if __name__=='__main__':
    F.setup()
    for label in sys.argv[1:] or ['scalar_left','aggregate_left']:
        run(label)
