"""Join the spilled counter node to nodes constructed around it at 107582cc."""
import json
import common as C

source=C.source()
C.T.CHAINS='6 18 1c;1 24;0 24;5 24'
work=C.OUT / 'materialize'
sites=[0x1072e92a,0x10758346,0x10758283]
C.T.run_gated(str(source),str(work),C.UNIT,C.NAME,raw_bps=sites)
events=C.lt_events(work)
found=[e for e in events if e['bp']==1 and e['chains'].get(0) and e['chains'][0][7]==99 and e['chains'][0][17]==0x1a0]
assert len(found)==2
node=found[0]['regs']['edi']
operands={e['regs']['ebp'] for e in found}
matches=[e for e in events if e['bp']>1 and (e['regs']['ebx']==node or e['regs']['ebp'] in operands)]
receipt=C.equality(work)
result=dict(node=node,spill_events=found,materializations=matches,sites=sites)
(work / 'nodes.json').write_text(json.dumps(result,indent=2)+'\n')
print('counter',hex(node),'materializations',len(matches),'whole object equality',receipt['equal_ignoring_only_coff_timestamp'])
for e in matches:
    print('site',hex(sites[e['bp']-1]),'hit',e['hit'],'regs',{k:hex(v) for k,v in e['regs'].items()})
    for i,vals in e['chains'].items(): print('chain',i,[hex(v) for v in vals[:12]])
