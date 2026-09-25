"""Prove reload operand/symbol/register identities and immediate predecessor."""
import json
import common as C

source=C.source()
C.T.CHAINS='6 24;0 24 28;5 24 1c;0 24 10'
work=C.OUT / 'materialize_operands'
sites=[0x1072e92a,0x10758346]
C.T.run_gated(str(source),str(work),C.UNIT,C.NAME,raw_bps=sites)
events=C.lt_events(work)
spill=next(e for e in events if e['bp']==1 and e['hit']==22)
node=spill['regs']['edi']
matches=[e for e in events if e['bp']==2 and e['regs']['ebx']==node]
assert len(matches)==1
reload=matches[0]
assert reload['chains'][1][6]==spill['chains'][0][6], 'reload is not counter symbol'
assert reload['chains'][2][2]!=0
receipt=C.equality(work)
result=dict(spill=spill,reload=reload,sites=sites)
(work / 'operands.json').write_text(json.dumps(result,indent=2)+'\n')
print('whole object equality',receipt['equal_ignoring_only_coff_timestamp'])
for i,v in reload['chains'].items(): print('chain',i,[hex(x) for x in v])
