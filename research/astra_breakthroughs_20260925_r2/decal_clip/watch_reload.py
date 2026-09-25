"""Follow the materialized ECX reload through subsequent list/order changes."""
import json
import common as C

C.T.CHAINS='0 24;0 24 10;0 24 28;0 24 2c'
work=C.OUT / 'watch_reload'
C.T.run_gated(str(C.source()),str(work),C.UNIT,C.NAME,raw_bps=[0x10758346],
    dwp=[(1,14,0,4,4,3),(1,14,0,0,4,1),(1,14,0,0x10,4,1)])
C.equality(work)
events=C.watch_events(work)
(work / 'reload.json').write_text(json.dumps(events,indent=2)+'\n')
for e in events[:100]:
    print('WP',e['wp'],'hit',e['hit'],'eip',hex(e['eip']),'value',hex(e['value']),'opcode',hex(e['record'][1]) if e['record'] else None)
