"""Package pointer-independent observations, receipts and bounded disassembly."""
import difflib
import json
import re
import capstone
import pefile
import frame_trace as F

def read(path): return json.loads(path.read_text())
def signed(value): return value-(1<<32) if value&(1<<31) else value
def name(row):
    n=row['name'] or ('temporary:'+str(row['rank']))
    if n=='_vehicle_left': n+=':computed' if row['frame'][7]==260 else ':turning'
    return n

def main():
    result={'scope':'Research only; no source promotion or exactness credit','scout':{},'independent':{}}
    for label in ['scalar_left','aggregate_left']:
        priority=read(F.OUT/(label+'_priority')/'priority.json')
        ranks=[x for x in priority['rows'] if x['kind']=='rank']
        names={x['rank']:name(x) for x in ranks}
        packing=read(F.OUT/(label+'_packing')/'packing.json')
        graphs={}
        for key,rank in enumerate([23,33,34,36]):
            head=packing['graph'][str(key)][:3]
            tail=packing['graph_tail'][str(key)]
            assert head[0]==0 and tail[0]==32 and tail[1]==0
            members=[base+bit for base,bits in [(head[0],head[2]),(tail[0],tail[2])] for bit in range(32) if bits&(1<<bit)]
            graphs[names[rank]]=sorted(names[x] for x in members)
        construction=read(F.OUT/(label+'_construction')/'construction.json')
        computed=next(x for x in construction['joined'] if x['name']=='_vehicle_left' and x['frame'][7]==260)
        relevant=[x for x in ranks if name(x) in ['_applied_alpha','_vehicle_left:computed','_vehicle_horizontal_forward','_desired_velocity']]
        final=read(F.OUT/(label+'_frame')/'frames.json')['rows']
        offsets={x['name']:x['offset'] for x in final}
        result['scout'][label]=dict(
            ordering=[dict(name=name(x),size=x['frame'][8],flags=hex(x['frame'][1]),count=x['frame'][14],rank=x['rank']) for x in ranks],
            created_computed_left=dict(flags=hex(computed['frame'][1]),size=computed['frame'][8],type=hex(computed['frame'][4]),descriptor_flags=[hex(computed['descriptor'][i]) for i in [8,12,13,14,21]]),
            relevant_final_offsets={k:v for k,v in offsets.items() if any(n in k for n in ['vehicle_left$26624','applied_alpha','vehicle_horizontal_forward','desired_velocity'])},
            interference=graphs,packing=[dict(x,name=names[x['rank']]) for x in packing['rows']],
            addition_counts={name(x):sum(e['increment'] for e in priority['rows'] if e['kind']=='add' and e['ptr']==x['ptr']) for x in relevant})
    assert result['scout']['scalar_left']['interference']==result['scout']['aggregate_left']['interference']
    result['four_target_interference_sets_identical_by_symbol']=True
    controls=read(F.HERE/'independent_control_results.json')
    for label,data in controls.items():
        result['independent'][label]=dict(source_sha256=data['source_sha256'],
            section=F.cc.section_info(F.cc.load(F.OUT/(label+'_frames')/'stock.obj'),'_frame_control'),
            ordering=[dict(name=x['name'],size=x['frame'][8],flags=hex(x['frame'][1]),count=x['frame'][14],rank=x['rank']) for x in data['rows'] if x['kind']=='rank'],
            final=[dict(name=x['name'],flags=hex(x['frame'][1]),offset=signed(x['frame'][9])) for x in data['frames']])
    assert result['independent']['control_aggregate']['section']==result['independent']['control_const_interface']['section']
    result['negative_const_interface_function_exact']=True
    for label in ['scalar_left','aggregate_left']:
        listing=(F.OUT/(label+'_frame')/'listing.asm').read_text()
        result['scout'][label]['computed_left_assembly']=[s for s in listing.splitlines() if '_vehicle_left$26624' in s]
        watch=read(F.OUT/(label+'_watch')/'watch.json')['events']
        live=[]
        for line in watch:
            if 'writer_eip=0x107010e6' in line: break
            live.append(line)
        result['scout'][label]['live_watch_prefix']=live
    result['stock_checks']={}
    for path in sorted(F.OUT.glob('*/stock_check.json')):
        data=read(path)
        assert data['full_object_timestamp_only']
        result['stock_checks'][path.parent.name]=data
    for label in ['control_aggregate','control_const_interface']:
        source=(F.OUT/(label+'.c')).read_text()
        original=(F.HERE/'control.c').read_text()
        (F.HERE/(label+'.patch')).write_text(''.join(difflib.unified_diff(original.splitlines(True),source.splitlines(True),fromfile='a/control.c',tofile='b/control.c')))
    # Static disassembly only, bounded to the traced sorter and frame-packer code.
    pe=pefile.PE(str(F.ROOT/'xbox/bin/vc7/C2.Dll'))
    md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    ranges=[(0x10759534,0x107596aa),(0x10735ac4,0x10735afc),(0x1076ae00,0x1076b136),(0x1076b3ba,0x1076b636)]
    lines=['C2.Dll SHA256 '+F.sha(F.ROOT/'xbox/bin/vc7/C2.Dll')]
    for start,end in ranges:
        lines.append('\nBOUND %08x..%08x'%(start,end))
        lines.extend(('%08x %-8s %s'%(i.address,i.mnemonic,i.op_str)).rstrip() for i in md.disasm(pe.get_data(start-0x10700000,end-start),start))
    (F.HERE/'c2_sort_pack.asm.txt').write_text('\n'.join(lines)+'\n')
    (F.HERE/'measurements.json').write_text(json.dumps(result,indent=2))
    manifest={str(p.relative_to(F.OUT)):F.sha(p) for p in F.OUT.glob('*/dbg_result.txt')}
    (F.HERE/'trace_hashes.json').write_text(json.dumps(manifest,indent=2))
    generic_reports={}
    for label in ['scalar','aggregate']:
        generic=F.OUT/('generic_control_'+label)/'frame_report.json'
        if generic.exists(): generic_reports[label]=read(generic)
    if generic_reports:
        (F.HERE/'generic_cli_smoke.json').write_text(json.dumps(generic_reports,indent=2))
    print('Packaged',len(result['stock_checks']),'full-object stock checks; four target interference sets match.')

if __name__=='__main__': main()
