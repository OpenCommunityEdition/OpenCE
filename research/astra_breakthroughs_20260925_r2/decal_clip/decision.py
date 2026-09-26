"""Observe the bounded call-site decision which materializes the counter reload."""
import json
import common as C

C.T.CHAINS='4 24;2 24;5 24;6 24'
work=C.OUT / 'decision'
sites=[0x1072e92a,0x10756db4,0x10756dc0,0x10756f39,0x10758346]
C.T.run_gated(str(C.source()),str(work),C.UNIT,C.NAME,raw_bps=sites)
events=C.lt_events(work)
spill=next(e for e in events if e['bp']==1 and e['hit']==22)
node=spill['regs']['edi']
selected=[e for e in events if
    e['bp'] in (2,3) and e['regs']['esi']==node or
    e['bp']==4 and e['regs']['ecx']==node or
    e['bp']==5 and e['regs']['ebx']==node]
assert [e['bp'] for e in selected]==[2,3,4,5]
assert selected[1]['regs']['eax']==2
assert selected[2]['stack'][:4]==[node,selected[2]['regs']['esi'],2,1]
assert selected[2]['chains'][0][2]==0x10040002
C.equality(work)
result=dict(spill=spill,decision=selected,sites=sites)
(work / 'decision.json').write_text(json.dumps(result,indent=2)+'\n')
for e in selected:
    print('site',hex(sites[e['bp']-1]),'hit',e['hit'],'regs',{k:hex(v) for k,v in e['regs'].items()},'stack',[hex(v) for v in e['stack']])
