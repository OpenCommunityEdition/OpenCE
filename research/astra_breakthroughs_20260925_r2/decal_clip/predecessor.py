"""Inspect operands of the node immediately preceding the newly inserted reload."""
import json
import common as C

C.T.CHAINS='0 24 10 28;0 24 10 2c;0 24 10 28 18;0 24 10 28 1c'
work=C.OUT / 'predecessor'
C.T.run_gated(str(C.source()),str(work),C.UNIT,C.NAME,raw_bps=[0x10758346])
e=next(e for e in C.lt_events(work) if e['bp']==1 and e['hit']==14)
C.equality(work)
(work / 'predecessor.json').write_text(json.dumps(e,indent=2)+'\n')
for i,v in e['chains'].items(): print('chain',i,[hex(x) for x in v])
