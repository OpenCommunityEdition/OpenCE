"""Bridge spill node -> new reload node -> actual encoder byte buffer and offset."""
import json
import struct
import common as C

C.T.CHAINS='1 24;2 24;5 24;1 24 2c'
work=C.OUT / 'emission'
sites=[0x1072e92a,0x10758346,0x10758283,0x10751347]
C.T.run_gated(str(C.source()),str(work),C.UNIT,C.NAME,raw_bps=sites)
events=C.lt_events(work)
spill=next(e for e in events if e['bp']==1 and e['hit']==22)
node=spill['regs']['edi']
assert spill['chains'][2][1]==0x21
original_edge_node=spill['chains'][2][4]
materialize=next(e for e in events if e['bp']==2 and e['regs']['ebx']==node)
reload_node=materialize['regs']['eax']
store_materialize=next(e for e in events if e['bp']==3 and e['regs']['ebp']==spill['regs']['ebp'])
store_node=store_materialize['regs']['eax']
emitted=[e for e in events if e['bp']==4 and e['regs']['ebx']==reload_node]
assert len(emitted)==1
reload=emitted[0]
edge_node=reload['chains'][0][4]
assert edge_node==original_edge_node, 'edge predecessor changed since spill'
edge=next(e for e in events if e['bp']==4 and e['regs']['ebx']==edge_node)
store=next(e for e in events if e['bp']==4 and e['regs']['ebx']==store_node)
tail=[e for e in events if e['bp']==4 and 0x4dd<=e['regs']['esi']<=0x4f8]
for e in tail:
    e['offset']=e['regs']['esi']
    e['encoded_hex']=b''.join(struct.pack('<I',v) for v in e['chains'][1])[:e['regs']['edx']].hex()
assert reload['offset']==0x4e5 and reload['encoded_hex']=='8b4dec'
assert edge['offset']==0x4e1 and edge['encoded_hex']=='8b749608'
assert store['offset']==0x4f5 and store['encoded_hex']=='894dec'
receipt=C.equality(work)
result=dict(spill=spill,materialization=materialize,reload_node=reload_node,
    store_materialization=store_materialize,store_node=store_node,
    edge_node=edge_node,original_edge_node=original_edge_node,tail=tail,sites=sites)
(work / 'emission.json').write_text(json.dumps(result,indent=2)+'\n')
print('whole-object timestamp-only equality',receipt['equal_ignoring_only_coff_timestamp'])
for e in tail: print(hex(e['offset']),e['encoded_hex'],'node',hex(e['regs']['ebx']),'opcode',hex(e['chains'][0][1]))
