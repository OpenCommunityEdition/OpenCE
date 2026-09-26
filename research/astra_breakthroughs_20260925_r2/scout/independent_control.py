"""Validate the size/use-count ordering prediction outside vehicles.c."""
import json
import frame_trace as F
import priority_trace as P

F.setup()
text=(F.HERE/'control.c').read_text()
needle='        horizontal.x = left.xy.x;\n        horizontal.y = left.xy.y;'
assert text.count(needle)==1
results={}
aggregate=text.replace(needle,'        horizontal = left.xy;')
for label,source in [('control_scalar',text),('control_aggregate',aggregate),
        ('control_const_interface',aggregate.replace('use2(pair *value)','use2(pair const *value)'))]:
    path=F.OUT/(label+'.c')
    path.write_text(source)
    result=P.run(label,str(path),'_frame_control')
    work=F.OUT/(label+'_frames')
    F.T.CHAINS='5 24;5 24 0;0 24;3 24'
    ev,obj=F.T.run_gated(str(path),str(work),F.UNIT,'_frame_control',raw_bps=[0x10814497],
        extra_cflags=['/FAsc','/Fa'+str(work/'listing.asm')])
    result['frames']=[dict(name=F.cstring(chs.get(2)),frame=chs.get(0)) for _,bp,regs,rm,chs in ev]
    result['frames_stock_check']=F.stock_check(work)
    result['source_sha256']=F.sha(path)
    results[label]=result
    print('FRAMES',label,result['frames'])
(F.HERE/'independent_control_results.json').write_text(json.dumps(results,indent=2))
