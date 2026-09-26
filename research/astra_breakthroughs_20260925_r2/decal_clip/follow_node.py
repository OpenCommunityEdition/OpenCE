"""Follow the live opcode-0x21 node using observational hardware watchpoints."""
import json
import common as C

source=C.source()
C.T.CHAINS='6 24;6 18 1c;5 24;5 24 0'
work=C.OUT / 'locate_node'
C.T.run_gated(str(source),str(work),C.UNIT,C.NAME,raw_bps=[0x1072e92a])
events=C.lt_events(work)
found=[e for e in events if e['bp']==1 and e['chains'].get(1) and e['chains'][1][7]==99 and e['chains'][1][17]==0x1a0 and e['chains'][2][1]==0x21]
assert len(found)==2, len(found)
C.equality(work)
(work / 'node.json').write_text(json.dumps(found,indent=2)+'\n')
hit=found[0]['hit']
print('counter node located at bp1 hit',hit)
work=C.OUT / 'watch_node22'
C.T.run_gated(str(source),str(work),C.UNIT,C.NAME,raw_bps=[0x1072e92a],
    dwp=[(1,hit,5,4,4,3),(1,hit,5,0,4,1),(1,hit,5,0x28,4,1),(1,hit,5,0x2c,4,1)])
receipt=C.equality(work)
watch=C.watch_events(work)
(work / 'watch.json').write_text(json.dumps(watch,indent=2)+'\n')
print('whole-object timestamp-only equality',receipt['equal_ignoring_only_coff_timestamp'])
for e in watch[:100]:
    print('WP',e['wp'],'hit',e['hit'],'eip',hex(e['eip']),'value',hex(e['value']),'opcode',hex(e['record'][1]) if e['record'] else None)
