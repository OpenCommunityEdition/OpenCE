"""Archive compact nonbinary observations, reproducibility receipts, and raw excerpts."""
import json
import re
import subprocess
import common as C

labels=['locate_node','watch_node','watch_node22','materialize','materialize_operands',
        'predecessor','watch_reload','decision','emission']
evidence={}
receipts={}
raw=[]
for label in labels:
    work=C.OUT / label
    receipt=json.loads((work / 'equality.json').read_text())
    assert receipt['equal_ignoring_only_coff_timestamp'] and receipt['compiler_disk_hashes_unchanged']
    assert receipt['target']['normalized_sha256']==C.TARGET_SHA
    receipt['target']={k:v for k,v in receipt['target'].items() if k!='relocations'}
    receipt['config_sha256']={p.name:C.sha(p.read_bytes()) for p in work.glob('dbg_*') if p.is_file()}
    receipt['log_sha256']=C.sha((work / 'dbg_result.txt').read_bytes())
    receipts[label]=receipt
    if label in ('watch_node','watch_node22','watch_reload'):
        # Ignore recycled addresses after the record is first zeroed.
        es=C.watch_events(work)
        if label=='watch_node':
            evidence[label]={'excluded':True,'reason':'Initial wrong hit6 due to one-nibble logging; actual counter hit22. No conclusions drawn.', 'first_watch':es[0]}
        else:
            valid=[]
            for e in es:
                if not e['record'] or e['record'][1]==0: break
                valid.append(e)
            evidence[label]={'live_record_watch_prefix':valid, 'remaining_watch_events_excluded':len(es)-len(valid)}
        continue
    names={'locate_node':'node.json','materialize':'nodes.json','materialize_operands':'operands.json',
           'predecessor':'predecessor.json','decision':'decision.json','emission':'emission.json'}
    data=json.loads((work / names[label]).read_text())
    evidence[label]=data
    wanted=set()
    def walk(value):
        if isinstance(value,dict):
            if 'bp' in value and 'hit' in value: wanted.add((value['bp'],value['hit']))
            for v in value.values(): walk(v)
        elif isinstance(value,list):
            for v in value: walk(v)
    walk(data)
    counts={}
    raw.append('\n### '+label+' (full ordinal prefix added; following line is verbatim)\n')
    for line in (work / 'dbg_result.txt').read_text().splitlines():
        if not line.startswith('LT '): continue
        bp=int(line.split()[1][2:],16)
        counts[bp]=counts.get(bp,0)+1
        if (bp,counts[bp]) in wanted:
            raw.extend([f'full_ordinal bp{bp} hit{counts[bp]}',line])

emission=evidence['emission']
node_names={emission['spill']['regs']['edi']:'INC',emission['reload_node']:'COUNTER_RELOAD',emission['edge_node']:'EDGE_LOAD',emission['store_node']:'COUNTER_STORE'}
chronology=[]
for e in emission['tail']:
    chronology.append(dict(node=node_names.get(e['regs']['ebx'],f'OTHER_{e["offset"]:x}'),
        offset=hex(e['offset']),encoded_hex=e['encoded_hex'],opcode=hex(e['chains']['0'][1])))
manifest=dict(baseline_commit='012f7283230d13be8427ef7e93f73f3ef686111f',
    production_changes=False,source_controls=0,exact_credit=0,
    source_sha256=C.SOURCE_SHA,target_normalized_sha256=C.TARGET_SHA,
    dependency_manifest_path=str(C.PREVIOUS / 'dependencies/manifest.json'),
    dependency_manifest_sha256=C.DEPENDENCY_MANIFEST_SHA,
    source_patch_path=str(C.PREVIOUS / 'patches/canonical_to_S4.patch'),
    source_patch_sha256=C.sha((C.PREVIOUS / 'patches/canonical_to_S4.patch').read_bytes()),
    dependencies=C.MANIFEST,
    local_analysis_dependency_sha256={name:C.sha((C.ROOT / name).read_bytes()) for name in ['tools/coff_compare.py','build.ninja']},
    normalized_final_chronology=chronology,receipts=receipts,evidence=evidence)
(C.HERE / 'measurements.json').write_text(json.dumps(manifest,indent=2)+'\n')
(C.HERE / 'raw_evidence.txt').write_text('\n'.join(raw)+'\n')
blocks=[]
for name,va,count in [('predicate_and_call',0x10756cc1,15),('output_input_scan',0x10756db4,29),
        ('operand_eligibility',0x10756ed8,31),('reload_constructor',0x107582cc,55),
        ('store_constructor',0x10758233,45),('insert_before',0x10707c52,7),
        ('scheduler_list_rebuild',0x1074efd0,12),('actual_encoding',0x10751329,19)]:
    blocks.append(name+'\n'+'\n'.join(C.disasm(va,count)))
(C.HERE / 'c2_sites.txt').write_text('\n'.join(line.rstrip() for line in '\n\n'.join(blocks).splitlines())+'\n')
test=subprocess.run([__import__('sys').executable,str(C.HERE / 'test_parser.py')],capture_output=True,text=True)
assert test.returncode==0,test.stdout+test.stderr
(C.HERE / 'parser_tests.txt').write_text(test.stdout+test.stderr)
print('Archived',len(receipts),'whole-object receipts and',len(chronology),'encoder tail nodes')
